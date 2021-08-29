#pragma once

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
		const Json::Value & req, 
		Json::Value & res,
		PQueryCallback	pErrorCallback = NULL
	) {	
		//	sqlite3 오류 코드 > 0 0인 경우 성공
		//	sqlite3 이외의 오류인 경우는 음수를 사용할 것.
		//	-1	exception
		//	-2	not found
		UINT	nCount	= 0;	
		try {
			if( false == req.isMember("name") ) {
				SetError(res, -2, pErrorCallback, "[name] is not found.");
				return 0;
			}
			if( req.isMember("bind") && !req["bind"].isArray() ) {
				SetError(res, -2, pErrorCallback, "[bind] is not array.");
				return 0;
			}
			const	Json::Value		&name	= req["name"];
			const	Json::Value		&bind	= req["bind"];

			STMTPtr	ptr	= Get(name.asCString());
			if( nullptr == ptr ) {
				SetError(res, -2, pErrorCallback, "[%s] is not found.", name.asCString());
				return 0;
			}	

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
			int			nStatus;
			int			nColumnCount	= sqlite3_column_count(ptr->pStmt);
			const char	*pColumnName	= NULL;
			const char	*pColumnType	= NULL;
			Json::Value	column;

			res["columnCount"]	= nColumnCount;
			for( auto i = 0 ; i < nColumnCount ; i++ ) {
				pColumnName		= sqlite3_column_name(ptr->pStmt, i);			
				column["name"]	= pColumnName? pColumnName : Json::Value::null;
				pColumnType		= sqlite3_column_decltype(ptr->pStmt, i);
				column["type"]	= pColumnType? pColumnType : Json::Value::null;

				res["column"].append(column);
			}
			while( true ) {
				nStatus	= sqlite3_step(ptr->pStmt);
				Json::Value		row;
				if( SQLITE_ROW == nStatus ) {
					nIndex	= 0;
					for( auto & t : bind ) {
						if( t.isObject() ) {				
							try {
								const	Json::Value	&type		= t["type"];
								const	Json::Value	&value		= t["value"];

								const	unsigned char	*pValue		= NULL;
								const	int				nValue		= 0;
								const	int64_t			nValue64	= 0;

								if( !type.asString().compare("int") ) {
									row[nIndex]	= sqlite3_column_int(ptr->pStmt, nIndex);
								}
								else if( !type.asString().compare("int64") ) {
									row[nIndex]	= sqlite3_column_int64(ptr->pStmt, nIndex);
								}
								else if( !type.asString().compare("uint64") ) {
									row[nIndex]	= (uint64_t)sqlite3_column_int64(ptr->pStmt, nIndex);
								}
								else {
									pValue	= sqlite3_column_text(ptr->pStmt, nIndex);
									row[nIndex]	= pValue? pValue : Json::Value::null;
								}
								nIndex++;
								res["row"].append(row);
							}
							catch( std::exception & e ) {
								SetError(res, -1, pErrorCallback, "row:%s", e.what());
							}
						}		
					}
					nCount++;
				}
				else if( SQLITE_DONE == nStatus ) {
					nCount++;
					break;

				}
				else 
					break;
			}
			res["lastQueryStatus"]	= nStatus;
			res["rowCount"]	= nCount;
		}
		catch( std::exception & e ) {
			SetError(res, -1, pErrorCallback, "exception:%s", e.what());
		}	
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
		sqlite3_reset(ptr->pStmt);
	}
	void			Create(PVOID pData, DWORD dwSize) {
		CDB		*pDB	= Db(DB_EVENT_NAME);

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
					//m_log.Log("%-32s %s %p", "@db", dbName.asCString(), pDB);
					for( auto & tt : bname.getMemberNames() ) {
						Json::Value	&sname		= bname[tt];
						if( sname.isString() ) {						
							try {							
								if( !tt.compare("@db") ) {
							
								}
								else {
									stmt.name	= t + "." + tt;
									stmt.query	= sname.asString();
									if( pDB ) {
										stmt.pStmt	= pDB->Stmt(sname.asCString());
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