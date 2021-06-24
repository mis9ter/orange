#pragma once

//#include "..\..\lib\zipvfs-sources\src\sqlite3.h"
#include "sqlite3.h"
#include "yagent.common.h"
#include "yagent.string.h"
#include "yagent.json.h"

#ifdef _WIN64	//_M_X64
#pragma comment(lib, "orangedb.x64.lib")
#pragma comment(lib, "lz4lib.x64.lib")
#else 
#pragma comment(lib, "orangedb.win32.lib")
#pragma comment(lib, "lz4lib.win32.lib")
#endif
int		sqlite3_open_db(const char* zName, sqlite3** pDb);

typedef std::function<void (std::string & name, std::string & query, sqlite3_stmt * p)>	PStmtCallback;
typedef struct STMT {
	std::string		name;
	std::string		query;
	sqlite3_stmt	*pStmt;
	PStmtCallback	pCallback;

	STMT(IN PCSTR pName, IN PCSTR pQuery, sqlite3_stmt *p) 
		:
		name(pName),
		query(pQuery),
		pStmt(p),
		pCallback(nullptr)
	{
		
	}
	~STMT() {
		if( pCallback )
			pCallback(name, query, pStmt);
	}
} STMT;

typedef std::shared_ptr<STMT>			STMTPtr;
typedef std::map<std::string, STMTPtr>	STMTMap;

class CStmtTable
{
public:
	CStmtTable() {
	
	}
	~CStmtTable() {
		Reset();
	}
	CStmtTable *	StmtTable() {
		return this;
	}
	STMTPtr		Get(IN PCSTR pName) {
		auto	t	= m_table.find(pName);
		if( m_table.end() == t )	return nullptr;
		return t->second;	
	}
	bool		Add(IN PCSTR pName, IN PCSTR pQuery, sqlite3_stmt *p, PStmtCallback pCallback) {
		try
		{
			STMTPtr		ptr	= std::shared_ptr<STMT>(new STMT(pName, pQuery, p));
			ptr->pCallback	= pCallback;
			m_table[ptr->name]	= ptr;
		}
		catch(std::exception & e) {
			UNREFERENCED_PARAMETER(e);
			//e.what();
			return false;
		}
		return true;
	}
	void		Reset()
	{
		m_table.clear();
	}
private:
	STMTMap		m_table;
};

class CDB;
class CPatchDB
	:
	virtual	public	CAppLog
{
public:
	CPatchDB() 
	{

	}
	~CPatchDB() {

	}
	DWORD		Patch(
		IN PCWSTR	pSrcPath, 
		IN PCWSTR	pDestPath,
		IN HANDLE	hBreakEvent
	);
	bool		DBLog(
		IN	CDB		&db,
		IN	INT		nAffected,
		IN	PCSTR	pQuery,
		IN	PCSTR	pFormat,
		...	
	);

private:
	HANDLE	m_hBreakEvent;

	bool	GetSchemaList(IN CDB & db, IN bool bTable, OUT Json::Value & doc);
	bool	GetColumnList(IN CDB & db, IN PCSTR pTableName, OUT Json::Value & doc);
	DWORD	GetColumnListString(IN const Json::Value & src, IN const Json::Value & dest, OUT std::string &str);
	DWORD	CheckSchema(
		IN	CDB					&srcDb,
		IN	const Json::Value	&srcSchema,
		IN	CDB					&destDb,
		IN	const Json::Value	&destSchema
	);
};
class CDB
	:
	public	CStmtTable,
	public	CPatchDB
{
public:
	CDB()	:
		m_pDb(NULL),
		m_log(L"db.log")
	{
		
	}
	//CDB(PCSTR pFilePath = NULL) :
	//	m_pDb(NULL)
	//{
	//	if (pFilePath)	Open(pFilePath);
	//}
	~CDB() {
		if( IsOpened() )
			Close(__FUNCTION__);
	}
	sqlite3_stmt *	GetStmt(IN PCSTR pName)
	{
		auto	t	= CStmtTable::Get(pName);
		if( nullptr == t )	return NULL;
		return t->pStmt;
	}
	bool	AddStmt(IN PCSTR pName, IN PCSTR pQuery)
	{
		sqlite3_stmt	*pStmt	= Stmt(pQuery);
		if( NULL == pStmt )	return false;
		return CStmtTable::Add(pName, pQuery, pStmt, FreeStmtCallback);
	}
	bool	Initialize(PCWSTR pFilePath, PCSTR pCause)
	{
		m_log.Log("%s pFilePath=%ws @%s", __FUNCTION__, pFilePath, pCause ? pCause : "");
		if( pFilePath )	return Open(pFilePath, pCause);
		return false;
	}
	void	Destroy(PCSTR pCause)
	{
		if( IsOpened() )	Close(__FUNCTION__);
		m_log.Log("%s @%s", __FUNCTION__, pCause ? pCause : "");
	}
	bool	Open(PCWSTR pFilePath, PCSTR pCause) 
	{
		return Open(__ansi(pFilePath), pCause);
	}
	bool	Open(PCSTR pFilePath, PCSTR pCause)
	{
		m_log.Log("%s %s @%s", __FUNCTION__, pFilePath, pCause);
		__function_lock(m_lock.Get());
		if (SQLITE_OK == sqlite3_open_db(__ansi(pFilePath), &m_pDb)) {

			return true;
		}
		return false;
	}
	bool	Execute(IN	std::function<void (int nAffected, PCSTR pQuery, PCSTR pError)> pCallback,
		IN const char *pFmt, ...)
	{
		if (NULL == pFmt || NULL == m_pDb )
		{
			return false;
		}
		va_list		argptr;
		char		szBuf[10000] = "";
		bool		bRet		= false;

		va_start(argptr, pFmt);
		::StringCbVPrintfA(szBuf, sizeof(szBuf), pFmt, argptr);
		va_end(argptr);
		char	*errmsg	= NULL;
		if (SQLITE_OK == sqlite3_exec(Handle(), szBuf, NULL, NULL, &errmsg))
		{
			bRet	= true;
			if( pCallback )	pCallback(sqlite3_changes(Handle()), szBuf, NULL);
		}
		else
		{
			if( pCallback )	pCallback(0, szBuf, errmsg);
		}
		return bRet;
	}
	bool	Executes(OUT int * pAffected, IN bool bShowQuery, IN bool bShowError, IN const char *pQuery)
	{
		bool	bRet	= false;

		if( pQuery && m_pDb ) {
		char	*errmsg = NULL;
			if (SQLITE_OK == sqlite3_exec(Handle(), pQuery, NULL, NULL, &errmsg))
			{
				if (pAffected)
					*pAffected = sqlite3_changes(Handle());
				bRet = true;
				if (bShowQuery)
					m_log.Log("%s", pQuery);
			}
			else
			{
				if (pAffected)	*pAffected = 0;
				if (bShowError && errmsg)
				{
					m_log.Log("%s\n%s", pQuery, errmsg);
				}
			}
		}
		return bRet;
	}
	void	Begin(PCSTR pCause)
	{
		//m_lock2.Lock(pCause);
		if( m_pDb ) {
			sqlite3_exec(m_pDb, "BEGIN;", NULL, NULL, NULL);
			m_log.Log("%s", __FUNCTION__);
		}
	}
	int		Commit(PCSTR pCause)
	{
		int		nCount	= 0;
		if( m_pDb ) {
			sqlite3_exec(m_pDb, "COMMIT;", NULL, NULL, NULL);
			nCount	= sqlite3_total_changes(m_pDb);
			m_log.Log("%s %d", __FUNCTION__, nCount);
		}
		//m_lock2.Unlock(pCause);
		return nCount;
	}
	sqlite3_stmt*	Stmt(IN PCWSTR query) {
		__function_lock(m_lock.Get());
		if (NULL == m_pDb)	return NULL;
		sqlite3_stmt* stmt = NULL;
		sqlite3_prepare16_v2(m_pDb, query, -1, &stmt, NULL);
		if (stmt)
		{
			m_log.Log("%s	%ws", __FUNCTION__, query);
		}
		else {
			m_log.Log("%s	%ws\n%ws", __FUNCTION__, query, (LPWSTR)sqlite3_errmsg16(m_pDb));
		}
		return stmt;
	}
	sqlite3_stmt*	Stmt(IN PCSTR query)
	{
		__function_lock(m_lock.Get());
		if (NULL == m_pDb)	return NULL;
		sqlite3_stmt* stmt = NULL;
		sqlite3_prepare_v2(m_pDb, query, -1, &stmt, NULL);
		if ( stmt)
		{
			m_log.Log("%s	%s", __FUNCTION__, query);
		}
		else {
			m_log.Log("%s	%s", __FUNCTION__, query);
			m_log.Log("%s	%s", __FUNCTION__, sqlite3_errmsg(m_pDb));
		}
		return stmt;
	}
	size_t		Cursor(
		sqlite3_stmt* pStmt, 
		PVOID pCallbackPtr, 
		std::function<bool (size_t nRows, PVOID pCallbackPtr, sqlite3_stmt * pStmt)> pCallback
	) {
		if( NULL == pStmt )	return 0;
		size_t		nRows	= 0;
		int			nRet;
		__function_lock(m_lock.Get());
		while (true) {
			if (SQLITE_ROW == (nRet = sqlite3_step(pStmt))) {
				if( pCallback ) {
					if( false == pCallback(nRows, pCallbackPtr, pStmt) )
						break;
				}
				nRows++;
			}
			else if( SQLITE_DONE == nRet ) break;
			else {
				m_log.Log("%s return code:%d\n", __FUNCTION__, nRet);
				break;
			}
		}
		return nRows;
	}
	static	void		Free(IN sqlite3_stmt* pStmt) {
		if (pStmt)
		{
			sqlite3_reset(pStmt);
			sqlite3_finalize(pStmt);
		}
	}
	bool		IsOpened() {
		__function_lock(m_lock.Get());
		return m_pDb? true : false;
	}
	void		Close(PCSTR pCause)
	{
		__function_lock(m_lock.Get());
		if (m_pDb) {
			sqlite3_close(m_pDb);
			m_pDb	= NULL;
			m_log.Log("%s @%s", __FUNCTION__, pCause);
		}
	}
	sqlite3*	Handle() {
		__function_lock(m_lock.Get());
		return m_pDb;
	}
	static	void FreeStmtCallback(std::string & name, std::string & query, sqlite3_stmt * p) {
		Free(p);
	}
	bool		CreateStmtTable(IN PCSTR pName, IN PCSTR pQuery) {
		sqlite3_stmt	*pStmt	= Stmt(pQuery);
		if( pStmt ) {
			if( CStmtTable::Add(pName, pQuery, pStmt, FreeStmtCallback)) {
			
			}	
			else {
				std::string		name	= pName;
				std::string		query	= pQuery;
				FreeStmtCallback(name, query, pStmt);
			}
		}
		else {
			m_log.Log("%s %s\n%s", __FUNCTION__, pQuery, sqlite3_errmsg(Handle()));
			return false;
		}
		return true;
	}
private:
	sqlite3			*m_pDb;
	CLock			m_lock;
	CLock			m_lock2;
	CAppLog			m_log;
};