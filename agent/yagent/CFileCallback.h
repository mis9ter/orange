#pragma once
#include <tlhelp32.h>

class CFileCallback
	:
	protected		IEventCallback,
	public virtual	CAppLog,
	virtual public	CStringTable
{
public:
	CFileCallback()
		:
		m_log(L"file.log")
	{
		ZeroMemory(&m_stmt, sizeof(m_stmt));
		m_name = EVENT_CALLBACK_NAME;
	}
	~CFileCallback()
	{

	}
	virtual	CDB*		Db() = NULL;
	virtual	PCWSTR		UUID2String(IN UUID* p, PWSTR pValue, DWORD dwSize) = NULL;
	virtual	bool		GetModule(PCWSTR pProcGuid, DWORD PID, ULONG_PTR pAddress,
						PWSTR pValue, DWORD dwSize)	= NULL;
	virtual		bool		FindModule(PROCUID PUID, PVOID pStartAddress,
						PVOID	pContext, std::function<void (PVOID, CProcess *, CModule *, PVOID)> pCallback)	= NULL;
	virtual		bool		GetProcess(PROCUID PUID, 
		PVOID	pContext, std::function<void (PVOID, CProcess *)> pCallback)	= NULL;

	PCSTR				Name() {
		return m_name.c_str();
	}
protected:
	void		Create()
	{
		const char* pInsert = "insert into thread"	\
			"("\
			"ProcGuid,ProcUID,PID,PPID,CPID,TID"\
			",CTID,StartAddress,RProcGuid,CreateTime,ExitTime"\
			",FilePath,FileName,FileExt,KernelTime,UserTime"\
			") values("\
			"?,?,?,?,?,?"\
			",?,?,?,?,?"\
			",?,?,?,?,?"\
			")";
		const char* pIsExisting = "select count(ProcGuid) from thread where ProcGuid=? and ProcUID=? and TID=?";
		const char* pUpdate = "update thread "	\
			"set ExitTime=?,KernelTime=?,UserTime=?,CreateCount=CreateCount+1 "\
			"where ProcGuid=? and ProcUID=? and TID=?";
		if (Db()->IsOpened()) {

			if (NULL == (m_stmt.pInsert = Db()->Stmt(pInsert)))
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			if (NULL == (m_stmt.pUpdate = Db()->Stmt(pUpdate)))
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			if (NULL == (m_stmt.pIsExisting = Db()->Stmt(pIsExisting)))
				Log("%s", sqlite3_errmsg(Db()->Handle()));
		}
		else {
			ZeroMemory(&m_stmt, sizeof(m_stmt));
		}
	}
	void		Destroy()
	{
		if (Db()->IsOpened()) {
			if (m_stmt.pInsert)		Db()->Free(m_stmt.pInsert);
			if (m_stmt.pUpdate)		Db()->Free(m_stmt.pUpdate);
			if (m_stmt.pIsExisting)	Db()->Free(m_stmt.pIsExisting);
			ZeroMemory(&m_stmt, sizeof(m_stmt));
		}
	}
	static	bool			Proc2
	(
		PY_HEADER			pMessage,
		PVOID				pContext
	) 
	{
		PY_THREAD_MESSAGE	p		= (PY_THREAD_MESSAGE)pMessage;
		CFileCallback		*pClass = (CFileCallback *)pContext;

		std::wstring		str;

		if( YFilter::Message::SubType::FileClose == p->subType ) {

		}
		pClass->m_lock.Lock(p, [&](PVOID pContext) {



		});
		return true;
	}
private:
	std::string		m_name;
	CLock			m_lock;
	CAppLog			m_log;
	struct {
		sqlite3_stmt* pInsert;
		sqlite3_stmt* pUpdate;
		sqlite3_stmt* pIsExisting;
	}	m_stmt;
	bool	IsExisting(
		PCWSTR			pProcGuid,
		PYFILTER_DATA	pData
	) {
		int			nCount = 0;
		sqlite3_stmt* pStmt = m_stmt.pIsExisting;
		if (pStmt) {
			int		nIndex = 0;
			sqlite3_bind_text16(pStmt,	++nIndex, pProcGuid, -1, SQLITE_STATIC);
			sqlite3_bind_int64(pStmt, ++nIndex, pData->PUID);
			sqlite3_bind_int(pStmt,		++nIndex, pData->TID);
			if (SQLITE_ROW == sqlite3_step(pStmt)) {
				nCount = sqlite3_column_int(pStmt, 0);
			}
			sqlite3_reset(pStmt);
		}
		return nCount ? true : false;
	}
	bool	Insert(
		PCWSTR				pProcGuid,
		PCWSTR				pProcPath,
		PCWSTR				pFilePath,
		PYFILTER_DATA		pData
	) {
		if (NULL == pProcPath)			return false;

		bool			bRet = false;
		sqlite3_stmt* pStmt = m_stmt.pInsert;
		if (pStmt) {
			int		nIndex = 0;
			PCWSTR	pProcName	= pProcPath? wcsrchr(pProcPath, L'\\')	: NULL;
			PCWSTR	pFileName	= pFilePath? wcsrchr(pFilePath, L'\\')	: NULL;
			PCWSTR	pFileExt	= pFileName? wcsrchr(pFileName, L'.')	: NULL;
			sqlite3_bind_text16(pStmt,	++nIndex, pProcGuid, -1, SQLITE_STATIC);
			sqlite3_bind_int64(pStmt, ++nIndex, pData->PUID);
			sqlite3_bind_int(pStmt,		++nIndex, pData->PID);
			sqlite3_bind_int(pStmt,		++nIndex, pData->PPID);
			sqlite3_bind_int(pStmt,		++nIndex, pData->CPID);
			sqlite3_bind_int(pStmt,		++nIndex, pData->TID);
			sqlite3_bind_int(pStmt,		++nIndex, pData->CTID);
			sqlite3_bind_int64(pStmt,	++nIndex, pData->pStartAddress);
			sqlite3_bind_null(pStmt,	++nIndex);
			
			char	szTime[50] = "";
			if (pData->times.CreateTime.QuadPart) {
				sqlite3_bind_text(pStmt, ++nIndex,
					CTime::LaregInteger2LocalTimeString(&pData->times.CreateTime, szTime, sizeof(szTime)),
					-1, SQLITE_TRANSIENT);
			}
			else {
				Log("---- CreateTime is null");
				sqlite3_bind_null(pStmt, ++nIndex);
			}
			if (pData->times.ExitTime.QuadPart) {
				sqlite3_bind_text(pStmt, ++nIndex,
					CTime::LaregInteger2LocalTimeString(&pData->times.ExitTime, szTime, sizeof(szTime)),
					-1, SQLITE_TRANSIENT);
			}
			else {
				sqlite3_bind_null(pStmt, ++nIndex);
			}
			if( pFilePath )
				sqlite3_bind_text16(pStmt, ++nIndex, pFilePath, -1, SQLITE_STATIC);
			else 
				sqlite3_bind_null(pStmt,	++nIndex);		//	FilePath
			if( pFileName )
				sqlite3_bind_text16(pStmt, ++nIndex, pFileName + 1, -1, SQLITE_STATIC);
			else
				sqlite3_bind_null(pStmt, ++nIndex);		//	FileName
			if( pFileExt )
				sqlite3_bind_text16(pStmt, ++nIndex, pFileExt + 1, -1, SQLITE_STATIC); 
			else 
				sqlite3_bind_null(pStmt,	++nIndex);		//	FileExt

			sqlite3_bind_int64(pStmt, ++nIndex, pData->times.KernelTime.QuadPart);
			sqlite3_bind_int64(pStmt, ++nIndex, pData->times.UserTime.QuadPart);
			if (SQLITE_DONE == sqlite3_step(pStmt))	bRet = true;
			else {
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		}
		return bRet;
	}
	bool	Update(
		PCWSTR				pProcGuid,
		PYFILTER_DATA		pData
	) {
		const char* pUpdate = "update thread "	\
			"set ExitTime=?,KernelTime=?,UserTime=? "\
			"where ProcGuid=? and TID=?";

		bool			bRet = false;
		sqlite3_stmt* pStmt = m_stmt.pUpdate;
		if (pStmt) {
			int		nIndex = 0;
			char	szTime[50] = "";
			if (pData->times.ExitTime.QuadPart) {
				sqlite3_bind_text(pStmt, ++nIndex,
					CTime::LaregInteger2LocalTimeString(&pData->times.ExitTime, szTime, sizeof(szTime)),
					-1, SQLITE_STATIC);
			}
			else
				sqlite3_bind_null(pStmt, ++nIndex);
			sqlite3_bind_int64(pStmt,	++nIndex, pData->times.KernelTime.QuadPart);
			sqlite3_bind_int64(pStmt,	++nIndex, pData->times.UserTime.QuadPart);
			sqlite3_bind_text16(pStmt,	++nIndex, pProcGuid, -1, SQLITE_STATIC);
			sqlite3_bind_int64(pStmt, ++nIndex, pData->PUID);
			sqlite3_bind_int(pStmt,		++nIndex, pData->TID);
			if (SQLITE_DONE == sqlite3_step(pStmt))	bRet = true;
			else {
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		}
		return bRet;
	}
};
