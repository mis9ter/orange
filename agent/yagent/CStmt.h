#pragma once

typedef std::function<void (std::string & name, std::string & query, sqlite3_stmt * p)>	PStmtCallback;
typedef struct STMT {
	std::string		name;
	std::string		query;
	sqlite3_stmt	*pStmt;

	STMT(IN PCSTR pName, IN PCSTR pQuery, sqlite3_stmt *p) 
		:
		name(pName),
		query(pQuery),
		pStmt(p)
	{

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
	}
	~CStmt() {
	
	}
	STMTPtr			Pop(PCSTR pName) {
		STMTPtr		ptr	= nullptr;
		
		auto	t	= m_table.find(pName);
		if( m_table.end() != t )	{
			ptr	= t->second;
		}
		return ptr;
	}
	void			Push(STMTPtr ptr) {
		sqlite3_reset(ptr->pStmt);
	}
	void			Create(PVOID pData, DWORD dwSize) {
		CDB		*pDB	= Db(DB_EVENT_NAME);

		//m_log.Log("%-32s\n%s", __func__, (char *)pData);

		Json::CharReaderBuilder	builder;
		const std::unique_ptr<Json::CharReader>		reader(builder.newCharReader());
		std::string		err;
		Json::Value		doc;
		std::string		strQueryName;
		if (reader->parse((const char *)pData, (const char *)pData + dwSize, &doc, &err)) {

			for( auto & t : doc.getMemberNames() ) {
				Json::Value	&bname	= doc[t];
				CDB		*pDB		= NULL;
				try {
					Json::Value	&dbName	= bname["@db"];
					pDB	= Db(__utf16(dbName.asCString()));
					//m_log.Log("%-32s %s %p", "@db", dbName.asCString(), pDB);
					for( auto & tt : bname.getMemberNames() ) {
						Json::Value	&sname		= bname[tt];
						if( sname.isString() ) {						
							try {							
								if( !tt.compare("@db") ) {
							
								}
								else {
									strQueryName	= t + "." + tt;
									if( pDB ) {
										sqlite3_stmt	*pStmt	= pDB->Stmt(sname.asCString());
										if( pStmt ) {
											m_log.Log("%-32s: %s", strQueryName.c_str(), sname.asCString());		
											Add(strQueryName.c_str(), sname.asCString(), pStmt);
										}
										else {
											m_log.Log("%-32s: %s", strQueryName.c_str(), sqlite3_errmsg(pDB->Handle()));
										}
									}
									else {

									}					
								}
							}
							catch( std::exception & e ) {
								m_log.Log("%-32s %s", __func__, e.what());		
							}	

						}
					}
				}
				catch( std::exception & e) {
					m_log.Log("%-32s %s", __func__, e.what());
				
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
	bool		Add(IN PCSTR pName, IN PCSTR pQuery, sqlite3_stmt *p) {
		try
		{
			auto	t		= m_table.find(pName);
			if( m_table.end() != t ) {
				m_table.erase(pName);
			}
			STMTPtr		ptr	= std::shared_ptr<STMT>(new STMT(pName, pQuery, p));
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