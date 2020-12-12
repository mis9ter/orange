#pragma once

//#include "..\..\lib\zipvfs-sources\src\sqlite3.h"
#include "sqlite3.h"
#include "yagent.common.h"
#include "yagent.string.h"
#include <functional>

#ifdef _WIN64	//_M_X64
#pragma comment(lib, "orangedb.x64.lib")
#else 
#pragma comment(lib, "orangedb.win32.lib")
#endif
int		sqlite3_open_db(const char* zName, sqlite3** pDb);
class CDB
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
	bool		Open(PCWSTR pFilePath, PCSTR pCause) 
	{
		return Open(__ansi(pFilePath), pCause);
	}
	bool		Open(PCSTR pFilePath, PCSTR pCause)
	{
		m_log.Log("%s %s @%s", __FUNCTION__, pFilePath, pCause);
		__function_lock(m_lock.Get());
		if (SQLITE_OK == sqlite3_open_db(__ansi(pFilePath), &m_pDb)) {

			return true;
		}
		return false;
	}
	bool	Execute(OUT int * pAffected, IN bool bShowQuery, IN bool bShowError, 
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
			if( pAffected )
				*pAffected = sqlite3_changes(Handle());
			bRet	= true;
			if( bShowQuery )
				m_log.Log("%s", szBuf);
		}
		else
		{
			if( pAffected )	*pAffected	= 0;
			if( bShowError && errmsg )
			{
				m_log.Log("%s\n%s", szBuf, errmsg);
			}
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
		__function_lock(m_lock.Get());
		if( m_pDb ) {
			sqlite3_exec(m_pDb, "BEGIN;", NULL, NULL, NULL);
			m_log.Log("%s", __FUNCTION__);
		}
	}
	int		Commit(PCSTR pCause)
	{
		__function_lock(m_lock.Get());
		int		nCount	= 0;
		if( m_pDb ) {
			sqlite3_exec(m_pDb, "COMMIT;", NULL, NULL, NULL);
			nCount	= sqlite3_total_changes(m_pDb);
			m_log.Log("%s %d", __FUNCTION__, nCount);
		}
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
	void		Free(IN sqlite3_stmt* pStmt) {
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
	sqlite3* Handle() {
		__function_lock(m_lock.Get());
		return m_pDb;
	}
private:
	sqlite3			*m_pDb;
	CLock			m_lock;
	CAppLog			m_log;
};
