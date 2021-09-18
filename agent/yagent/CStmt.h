#pragma once

#define USE_STMT	1

typedef std::function<void (std::string & name, std::string & query, sqlite3_stmt * p)>	PStmtCallback;
typedef std::function<void (int nErrorCode, const char * pErrorMessage)>				PQueryCallback;

typedef struct _STMT {
	_STMT() 
		:
		pDB(NULL), pStmt(NULL)	
	{	
	}
	std::string		name;
	std::string		query;
	CDB				*pDB;
	sqlite3_stmt	*pStmt;
} _STMT, *_PSTMT;

typedef struct STMT 
	:	public	_STMT
{
	STMT(IN _PSTMT p) 
	{
		name	= p->name;
		query	= p->query;
		pDB		= p->pDB;
		pStmt	= p->pStmt;
	}
	~STMT() {
		if( pStmt ) {
			sqlite3_reset(pStmt);
			sqlite3_finalize(pStmt);
		}
	}
} STMT;

typedef std::shared_ptr<STMT>			STMTPtr;
typedef std::map<std::string, STMTPtr>	STMTMap;

class CStmt
{
public:
	CStmt() 
		:	m_log(L"stmt.log")


	
	{
		m_log.Log(NULL);
		m_log.Log("%s %s", __DATE__, __TIME__);
	}
	~CStmt() {
	
	}
	static	void	BindString(Json::Value & doc, PCWSTR pValue) {
		doc["type"]		= "text";
		doc["value"]	= __utf8(pValue);
	}
	static	void	BindUInt64(Json::Value & doc, uint64_t nValue) {
		doc["type"]		= "uint64";
		doc["value"]	= nValue;
	}
	static	void	BindInt64(Json::Value & doc, uint64_t nValue) {
		doc["type"]		= "int64";
		doc["value"]	= nValue;
	}
	static	void	BindInt(Json::Value & doc, int nValue) {
		doc["type"]		= "int";
		doc["value"]	= nValue;
	}
	/*
		input:
		(1) query stmt name
		(2) 인자 JSON

		{
			"name":"xxxx",
			"bind": [
				{"type":"int","value":"value"},
				{"type":"int64","value":value},
				{"type":"text","value":"value"}
			]
		}

		output:
		(1) 결과 JSON
		{
			"name":"xxxx",
			"column":[


			],
			"row":[


			],
			"error":{
				"code":xxx,
				"msg":""
			},
			"exception":""
		}

	*/
	void			SetError(
		Json::Value &doc, 
		int			nCode, 
		std::function<void (int nErrorCode, const char * pErrorMessage)>	pCallback,
		PCSTR		pFmt, ...
	) {
		//if( doc )	doc["error"]["code"]	= nCode;
		if (NULL == pFmt)	return;

		va_list		argptr;
		char		szBuf[4096] = "";
		va_start(argptr, pFmt);
		int		nLength = lstrlenA(szBuf);
		::StringCbVPrintfA(szBuf + nLength, sizeof(szBuf) - nLength, pFmt, argptr);
		va_end(argptr);
		//doc["error"]["message"]	= szBuf;
		if( pCallback )	pCallback(nCode, szBuf);
		m_log.Log("%-32s %s", __func__, szBuf);
	} 
	unsigned int	Query(
		const Json::Value	& req, 
		Json::Value			& res,
		bool				bDebugLog		= false,
		PQueryCallback		pErrorCallback	= NULL
	) {	
		//	sqlite3 오류 코드 > 0 0인 경우 성공
		//	sqlite3 이외의 오류인 경우는 음수를 사용할 것.
		//	-1	exception
		//	-2	not found
		UINT		nCount	= 0;	
		STMTPtr		ptr		= nullptr;
		try {
			if( false == req.isMember("name") ) {
				SetError(res, -2, pErrorCallback, "[name] is not found.");
				return 0;
			}
			if( req.isMember("bind") && !req["bind"].empty() && !req["bind"].isArray() ) {
				SetError(res, -2, pErrorCallback, "[bind] is not array.");
				return 0;
			}
			const	Json::Value		&name	= req["name"];
			const	Json::Value		&bind	= req["bind"];

			ptr	= Get(name.asCString());
			if( nullptr == ptr ) {
				SetError(res, -2, pErrorCallback, "[%s] is not found.", name.asCString());
				return 0;
			}	
			res["db"]["path"]	= ptr->pDB->Path();
			res["db"]["stmt"]	= ptr->name;
			res["db"]["handle"]	= (Json::Value::UInt64)ptr->pDB->Handle();
			if( bDebugLog )
				m_log.Log("%-32s %p", ptr->name.c_str(), ptr->pDB->Handle());

			res["sql"]	= ptr->query;
			int		nIndex	= 0;
			for( auto & t : bind ) {
				if( t.isObject() ) {				
					const		Json::Value	&type	= t["type"];
					const		Json::Value	&value	= t["value"];		
					uint64_t	nValue64;

					try {									
						if( !type.asString().compare("int") ) {							
							sqlite3_bind_int(ptr->pStmt, ++nIndex, value.asInt());
						}
						else if( !type.asString().compare("int64") ) {
							sqlite3_bind_int64(ptr->pStmt, ++nIndex, nValue64 = value.asLargestInt());
						}
						else if( !type.asString().compare("uint64") ) {
							sqlite3_bind_int64(ptr->pStmt, ++nIndex, nValue64 = value.asLargestUInt());
						}
						else if( !type.asString().compare("text")) {
							sqlite3_bind_text(ptr->pStmt, ++nIndex, value.asCString(), -1, SQLITE_TRANSIENT);
						}
					}
					catch( std::exception & e ) {
						SetError(res, -1, pErrorCallback, "bind[%s]:%s", type.asCString(), e.what());
						JsonUtil::Json2String(t, [&](std::string &str) {
							m_log.Log(str.c_str());
						});
					}
				}		
			}
			int			nStatus			= 0;
			int			nColumnCount	= sqlite3_column_count(ptr->pStmt);
			const char	*pColumnName	= NULL;
			const char	*pColumnType	= NULL;
			Json::Value	&column			= res["column"];
			for( auto i = 0 ; i < nColumnCount ; i++ ) {
				Json::Value		c;
				pColumnName		= sqlite3_column_name(ptr->pStmt, i);			
				c["name"]	= pColumnName? pColumnName : Json::Value::null;
				pColumnType		= sqlite3_column_decltype(ptr->pStmt, i);
				c["type"]	= pColumnType? pColumnType : Json::Value::null;
				column.append(c);
			}

			Json::Value		&row	= res["row"];
			int				nRow	= 0;
			for( nRow = 0 ; nRow < 1000 ; nRow++ ) {
				Json::Value		col;
				nStatus	= sqlite3_step(ptr->pStmt);
				if( SQLITE_ROW == nStatus ) {
					col.clear();
					for( unsigned i = 0 ; i < column.size() ; i++ ) {
						try {
							auto	&t	= column[i];
							const	Json::Value	&type		= t["type"];
							const	Json::Value	&name		= t["name"];

							const	unsigned char	*pValue		= NULL;
							const	int				nValue		= 0;
							const	int64_t			nValue64	= 0;

							int						nType		= sqlite3_column_type(ptr->pStmt, i);
							if( 0 == nRow ) {
								//	column 정보를 넣을 때 한번에 하려 했으나 윗 부분에서 하면 모두 type == 5로 나온다.
								//	실데이터가 필요해.
								column[i]["@type"]	= nType;
							}
							switch( nType ) {
								case SQLITE_INTEGER:
									col.append((uint64_t)sqlite3_column_int64(ptr->pStmt, i));
									break;

								case SQLITE_FLOAT:
									col.append(sqlite3_column_double(ptr->pStmt, i));
									break;

								case SQLITE_TEXT:
									col.append((char *)sqlite3_column_text(ptr->pStmt, i));
									break;

								case SQLITE_NULL:
									col.append(Json::Value::null);
									break;

								case SQLITE_BLOB:
									col.append("not supported value type");
									break;
							}
						}
						catch( std::exception & e ) {
							SetError(res, -1, pErrorCallback, "%s(%d):%s", __FILE__, __LINE__, e.what());
						}
					}
					row.append(col);
					nCount++;
				}
				else if( SQLITE_DONE == nStatus ) {
					break;
				}
				else 
					break;
			}
			res["status"]	= nStatus;
		}
		catch( std::exception & e ) {
			SetError(res, -1, pErrorCallback, "%s(%d):%s", __FILE__, __LINE__, e.what());
		}	
		Reset(ptr);
		res["bind"]	= req["bind"];
		return nCount;
	}
	STMTPtr			Get(PCSTR pName) {
		STMTPtr		ptr	= nullptr;
		
		auto	t	= m_table.find(pName);
		if( m_table.end() != t )	{
			ptr	= t->second;
		}
		return ptr;
	}
	void			Reset(STMTPtr ptr) {
		if( ptr )	sqlite3_reset(ptr->pStmt);
	}
	void			Create(PVOID pData, DWORD dwSize) {
		//m_log.Log("%-32s\n%s", __func__, (char *)pData);

		Json::CharReaderBuilder	builder;
		const std::unique_ptr<Json::CharReader>		reader(builder.newCharReader());
		std::string		err;
		Json::Value		doc;
		
		_STMT			stmt;

		if (reader->parse((const char *)pData, (const char *)pData + dwSize, &doc, &err)) {

			for( auto & t : doc.getMemberNames() ) {
				Json::Value	&bname	= doc[t];
				try {
					if( false == bname.isMember("@db") ) {
						m_log.Log("%-32s %s has no @db.", __func__, t.c_str());
					}
					Json::Value	&dbName	= bname["@db"];
					stmt.pDB	= Db(__utf16(dbName.asCString()));
					m_log.Log("%-32s %p", dbName.asCString(), stmt.pDB->Handle());
					for( auto & tt : bname.getMemberNames() ) {
						Json::Value	&sname		= bname[tt];
						if( sname.isString() ) {						
							try {							
								if( !tt.compare("@db") ) {
							
								}
								else {
									stmt.name	= t + "." + tt;
									stmt.query	= sname.asString();
									if( stmt.pDB ) {
										stmt.pStmt	= stmt.pDB->Stmt(sname.asCString());
										if( stmt.pStmt ) {
											m_log.Log("%-32s: %s", stmt.name.c_str(), sname.asCString());		
											Add(&stmt);
										}
										else {
											m_log.Log("%-32s: %s", stmt.name.c_str(), sqlite3_errmsg(stmt.pDB->Handle()));
										}
									}
									else {

									}					
								}
							}
							catch( std::exception & e ) {
								m_log.Log("%-32s exception(1) %s", __func__, e.what());		
								JsonUtil::Json2String(sname, [&](std::string & str) {
									m_log.Log("%s", str.c_str());
								});
							}	

						}
					}
				}
				catch( std::exception & e) {
					m_log.Log("%-32s exception(2) %s", __func__, e.what());
					JsonUtil::Json2String(bname, [&](std::string & str) {
						m_log.Log("%s: %s", t.c_str(), str.c_str());
					});
				}
			}			
		}
		else {
			m_log.Log("%-32s not parse.", __func__);
		
		}
	}
	void		Destroy() {
		m_table.clear();
	}
	bool		Add(IN _STMT * p) {
		try
		{
			auto	t		= m_table.find(p->name);
			if( m_table.end() != t ) {
				m_table.erase(p->name);
			}
			STMTPtr		ptr	= std::shared_ptr<STMT>(new STMT(p));
			m_table[ptr->name]	= ptr;
		}
		catch(std::exception & e) {
			UNREFERENCED_PARAMETER(e);
			//e.what();
			return false;
		}
		return true;
	}

	virtual	CDB*	Db(PCWSTR pName) = NULL;
private:
	CAppLog		m_log;
	STMTMap		m_table;
};