#pragma once

#include <tlhelp32.h>

inline	BOOL GetModule(__in DWORD dwProcId, 
	__in	BYTE * dwThreadStartAddr,
	__out_bcount(MAX_PATH + 1) LPWSTR lpstrModule, 
	__in	DWORD	dwSize)
{
	BOOL bRet = FALSE;
	HANDLE hSnapshot;
	MODULEENTRY32 moduleEntry32;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPALL, dwProcId);

	moduleEntry32.dwSize = sizeof(MODULEENTRY32);
	moduleEntry32.th32ModuleID = 1;

	if (Module32First(hSnapshot, &moduleEntry32)) {
		if (dwThreadStartAddr >= moduleEntry32.modBaseAddr && 
			dwThreadStartAddr <= (moduleEntry32.modBaseAddr + moduleEntry32.modBaseSize)) {
			StringCbCopy(lpstrModule, dwSize, moduleEntry32.szExePath);
		}
		else {
			while (Module32Next(hSnapshot, &moduleEntry32)) {
				if (dwThreadStartAddr >= moduleEntry32.modBaseAddr && 
					dwThreadStartAddr <= (moduleEntry32.modBaseAddr + moduleEntry32.modBaseSize)) {
					StringCbCopy(lpstrModule, dwSize, moduleEntry32.szExePath);
					break;
				}
			}
		}
	}

	//if (pModuleStartAddr) *pModuleStartAddr = moduleEntry32.modBaseAddr;
	CloseHandle(hSnapshot);

	return bRet;
}


class CThreadCallback
	:
	protected		IEventCallback,
	public virtual	CAppLog
{
public:
	CThreadCallback()
	{
		ZeroMemory(&m_stmt, sizeof(m_stmt));
		m_name = EVENT_CALLBACK_NAME;
	}
	~CThreadCallback()
	{

	}
	virtual	CDB*		Db() = NULL;
	virtual	PCWSTR		UUID2String(IN UUID* p, PWSTR pValue, DWORD dwSize) = NULL;
	virtual	bool		GetModule(PCWSTR pProcGuid, DWORD PID, ULONG_PTR pAddress,
						PWSTR pValue, DWORD dwSize)	= NULL;
	PCSTR				Name() {
		return m_name.c_str();
	}
protected:
	void		Create()
	{
		const char* pInsert = "insert into thread"	\
			"("\
			"ProcGuid,PID,PPID,CPID,TID"\
			",CTID,StartAddress,RProcGuid,CreateTime,ExitTime"\
			",FilePath,FileName,FileExt,KernelTime,UserTime"\
			") values("\
			"?,?,?,?,?"\
			",?,?,?,?,?"\
			",?,?,?,?,?"\
			")";
		const char* pIsExisting = "select count(ProcGuid) from thread where ProcGuid=? and TID=?";
		const char* pUpdate = "update thread "	\
			"set ExitTime=?,KernelTime=?,UserTime=?,CreateCount=CreateCount+1 "\
			"where ProcGuid=? and TID=?";
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

	PCWSTR	GetFileName(PCWSTR pFilePath) {
		PCWSTR	p	= wcsrchr(pFilePath, L'\\');
		return p? p + 1: pFilePath;
	}

	static	bool			Proc(
		ULONGLONG			nMessageId,
		PVOID				pCallbackPtr,
		PYFILTER_DATA		p
	)
	{
		//	GetFileAttributesExA

		CThreadCallback	*pClass = (CThreadCallback *)pCallbackPtr;
		WCHAR	szPath[AGENT_PATH_SIZE] = L"";
		PWSTR	pFileName = NULL;

		WCHAR	szProcGuid[GUID_STRLEN] = L"";
		pClass->UUID2String(&p->ProcGuid, szProcGuid, sizeof(szProcGuid));


		SYSTEMTIME	stCreate;
		SYSTEMTIME	stExit;
		SYSTEMTIME	stKernel;
		SYSTEMTIME	stUser;

		char		szTime[40]	= "";

		CTime::LargeInteger2SystemTime(&p->times.CreateTime, &stCreate);
		CTime::LargeInteger2SystemTime(&p->times.ExitTime, &stExit);
		CTime::LargeInteger2SystemTime(&p->times.KernelTime, &stKernel, false);
		CTime::LargeInteger2SystemTime(&p->times.UserTime, &stUser, false);

		if (false == CAppPath::GetFilePath(p->szPath, szPath, sizeof(szPath)))
			StringCbCopy(szPath, sizeof(szPath), p->szPath);

		WCHAR	szModule[AGENT_PATH_SIZE] = L"";
		if (!pClass->GetModule(szProcGuid, p->PID, p->pStartAddress, szModule, sizeof(szModule)))
		{
			//::GetModule(p->PID, (BYTE*)p->pStartAddress, szModule, sizeof(szModule));
		}		
		//GetModule(p->dwProcessId, szModule, sizeof(szModule), (BYTE *)p->pStartAddress, NULL);

		switch (p->subType) {
			case YFilter::Message::SubType::ThreadStart:
				if (pClass->IsExisting(szProcGuid, p))
					pClass->Update(szProcGuid, p);
				else
					pClass->Insert(szProcGuid, szPath, szModule, p);
				//pClass->Log("%-20s %6d %ws", "THREAD_START", p->dwProcessId, szPath);
				//pClass->Log("  start address %p", p->pStartAddress);
				//pClass->Log("  start address %I64d", p->pStartAddress);
				//pClass->Insert(szProcGuid, szPath, NULL, p);
				//pClass->Log("  create   %s", SystemTime2String(&stCreate, szTime, sizeof(szTime)));
				//pClass->Log("  exit     %s", SystemTime2String(&stCreate, szTime, sizeof(szTime)));
				//pClass->Log("  kernel   %s", SystemTime2String(&stCreate, szTime, sizeof(szTime)));
				//pClass->Log("  user     %s", SystemTime2String(&stCreate, szTime, sizeof(szTime)));
				//pClass->Log("%8d %8d %I64d %ws", p->dwProcessId, p->dwThreadId, p->pStartAddress, szPath);
			break;

			case YFilter::Message::SubType::ThreadStop:
				//pClass->Log("%-20s %6d %ws", "THREAD_STOP", p->dwThreadId, szPath);
				//pClass->Log("  create        %s", SystemTime2String(&stCreate, szTime, sizeof(szTime)));
				//pClass->Log("  exit          %s", SystemTime2String(&stExit, szTime, sizeof(szTime)));
				//pClass->Log("  kernel        %s", SystemTime2String(&stKernel, szTime, sizeof(szTime), true));
				//pClass->Log("  user          %s", SystemTime2String(&stUser, szTime, sizeof(szTime), true));
				if( pClass->IsExisting(szProcGuid, p) )
					pClass->Update(szProcGuid, p);
				else
					pClass->Insert(szProcGuid, szPath, szModule, p);
			break;
			
			default:
				pClass->Log("%-20s ", "UNKNOWN_THREAD_TYPE");
				break;
		}
		return true;
	}
private:
	std::string		m_name;
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
