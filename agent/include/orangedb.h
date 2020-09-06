#pragma once

//#include "..\..\lib\zipvfs-sources\src\sqlite3.h"
#include "sqlite3.h"
#include "yagent.string.h"
#include <functional>

#pragma comment(lib, "orangedb.lib")

int		sqlite3_open_db(const char* zName, sqlite3** pDb);
class CDB
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
		Close();
	}
	bool	Initialize(PCWSTR pFilePath)
	{
		if( pFilePath )	return Open(pFilePath);
		return false;
	}
	void	Destroy()
	{
		if( IsOpened() )	Close();
	}
	void	Begin()
	{
		if( m_pDb )
			sqlite3_exec(m_pDb, "BEGIN;", NULL, NULL, NULL);
	}
	void	Commit()
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
			printf("%s	%s\n%s\n", __FUNCTION__, query, sqlite3_errmsg(m_pDb));
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
	bool		Open(PCWSTR pFilePath) {
		return Open(__ansi(pFilePath));
	}
	bool		Open(PCSTR pFilePath)
	{
		if (SQLITE_OK == sqlite3_open_db(__ansi(pFilePath), &m_pDb)) {

			return true;
		}
		return false;
	}
	void		Close()
	{
		if (m_pDb) {
			sqlite3_close(m_pDb);
			m_pDb	= NULL;
		}
	}
	sqlite3* Handle() {
		return m_pDb;
	}
private:
	sqlite3			*m_pDb;
};
