#pragma once

//#include "..\..\lib\zipvfs-sources\src\sqlite3.h"
#include "sqlite3.h"
#include "yagent.string.h"
#include <functional>

#pragma comment(lib, "orangedb.lib")

int		sqlite3_open_db(const char* zName, sqlite3** pDb);
class CDB
	:
	virtual public	CAppLog
{
public:
	CDB()	:
		m_pDb(NULL)
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
		Log("%s pFilePath=%ws @%s", __FUNCTION__, pFilePath, pCause ? pCause : "");
		if( pFilePath )	return Open(pFilePath, pCause);
		return false;
	}
	void	Destroy(PCSTR pCause)
	{
		if( IsOpened() )	Close(__FUNCTION__);
		Log("%s @%s", __FUNCTION__, pCause ? pCause : "");
	}
	bool		Open(PCWSTR pFilePath, PCSTR pCause) 
	{
		return Open(__ansi(pFilePath), pCause);
	}
	bool		Open(PCSTR pFilePath, PCSTR pCause)
	{
		Log("%s %s @%s", __FUNCTION__, pFilePath, pCause);
		if (SQLITE_OK == sqlite3_open_db(__ansi(pFilePath), &m_pDb)) {

			return true;
		}
		return false;
	}
	void	Begin(PCSTR pCause)
	{
		if( m_pDb )
			sqlite3_exec(m_pDb, "BEGIN;", NULL, NULL, NULL);
	}
	void	Commit(PCSTR pCause)
	{
		if( m_pDb )
			sqlite3_exec(m_pDb, "COMMIT;", NULL, NULL, NULL);
	}
	sqlite3_stmt*	Stmt(IN PCWSTR query) {
		if (NULL == m_pDb)	return NULL;
		sqlite3_stmt* stmt = NULL;
		sqlite3_prepare16_v2(m_pDb, query, -1, &stmt, NULL);
		if (NULL == stmt)
		{
			_tprintf(_T("%S	%s\n%s\n"), __FUNCTION__, query, (LPWSTR)sqlite3_errmsg16(m_pDb));
		}
		return stmt;
	}
	sqlite3_stmt*	Stmt(IN PCSTR query)
	{
		if (NULL == m_pDb)	return NULL;
		sqlite3_stmt* stmt = NULL;
		sqlite3_prepare_v2(m_pDb, query, -1, &stmt, NULL);
		if (NULL == stmt)
		{
			printf("%s	%s\n", __FUNCTION__, query);
			printf("%s	%s\n", __FUNCTION__, sqlite3_errmsg(m_pDb));
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
				printf("%s return code:%d\n", __FUNCTION__, nRet);
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
		return m_pDb? true : false;
	}
	void		Close(PCSTR pCause)
	{
		if (m_pDb) {
			sqlite3_close(m_pDb);
			m_pDb	= NULL;
			Log("%s @%s", __FUNCTION__, pCause);
		}
	}
	sqlite3* Handle() {
		return m_pDb;
	}
private:
	sqlite3			*m_pDb;
};
