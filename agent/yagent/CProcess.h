#pragma once

#define	GUID_STRLEN	37

class CProcess
	:
	public virtual	CAppLog
{
public:
	CProcess() {

	}
	~CProcess() {

	}
	virtual	CDB *	Db()	= NULL;
	virtual	CPathConvertor* PathConvertor()	= NULL;

	static	PCWSTR		UUID2String(IN UUID * p, PWSTR pValue, DWORD dwSize) {
		StringCbPrintf(pValue, dwSize, 
			L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			p->Data1, p->Data2, p->Data3,
			p->Data4[0], p->Data4[1], p->Data4[2], p->Data4[3],
			p->Data4[4], p->Data4[5], p->Data4[6], p->Data4[7]);
		return pValue;
	}

	static	bool	GetFilePath(IN PCWSTR pLinkPath, OUT PWSTR pValue, IN DWORD dwSize)
	{
		bool	bRet	= false;
		WCHAR	szPath[AGENT_PATH_SIZE];
		PCWSTR	pSystemRoot		= L"\\SystemRoot";
		PCWSTR	pWinDir			= L"%windir%";
		int		nSystemRoot		= (int)wcslen(pSystemRoot);
		WCHAR	szWinDir[200]	= L"";

		if (0 == _wcsnicmp(pSystemRoot, pLinkPath, nSystemRoot)) {
			ExpandEnvironmentStrings(pWinDir, szWinDir, _countof(szWinDir));
			StringCbPrintf(pValue, dwSize, L"%s%s", szWinDir, pLinkPath + nSystemRoot);
			return true;
		}
		HANDLE	hFile = CreateFile(pLinkPath,
			0,
			0,
			0,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS,
			0);
		if (INVALID_HANDLE_VALUE != hFile)
		{
			auto rcode = GetFinalPathNameByHandle(hFile, szPath, _countof(szPath), 
							FILE_NAME_NORMALIZED);
			if (rcode)
			{
				if (szPath[0] == L'\\' && szPath[1] == L'\\' && 
					szPath[2] == L'?' && szPath[3] == L'\\')
						StringCbCopy(pValue, dwSize, szPath + 4);
				else
					StringCbCopy(pValue, dwSize, szPath);
				bRet	= true;
			}
			CloseHandle(hFile);
		}
		return bRet;
	}

protected:
	static	bool			ProcessCallbackProc(
		ULONGLONG			nMessageId,
		PVOID				pCallbackPtr,
		PYFILTER_DATA		p
	) 
	{
		//	GetFileAttributesExA

		CProcess	*pClass = (CProcess *)pCallbackPtr;
		if (YFilter::Message::SubType::ProcessStart == p->subType) {
			WCHAR	szPath[AGENT_PATH_SIZE] = L"";
			PWSTR	pFileName = NULL;
			if( false == GetFilePath(p->szPath, szPath, sizeof(szPath)) )
				StringCbCopy(szPath, sizeof(szPath), p->szPath);

			WCHAR	szProcGuid[GUID_STRLEN]	= L"";
			pClass->Start(UUID2String(&p->uuid, szProcGuid, sizeof(szProcGuid)), 
				szPath, p->szCommand, p->dwProcessId, p->dwParentProcessId);
			pClass->Log("PROCESS_START:%6d %ws", p->dwProcessId, szPath);
			/*
			pClass->Log("MessageId    :%I64d", nMessageId);
			pClass->Log("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
				p->uuid.Data1, p->uuid.Data2, p->uuid.Data3,
				p->uuid.Data4[0], p->uuid.Data4[1], p->uuid.Data4[2], p->uuid.Data4[3],
				p->uuid.Data4[4], p->uuid.Data4[5], p->uuid.Data4[6], p->uuid.Data4[7]);

			pClass->Log("%ws", p->szPath);
			pClass->Log("%ws", p->szCommand);
			pClass->Log("%s", TEXTLINE);
			*/
		}
		else if (YFilter::Message::SubType::ProcessStop == p->subType) {
			if (p->dwProcessId)
			{
				//	드라이버에서 생성을 감지했던 프로세스
			}
			else
			{
				//	드라이버 로드전에 이미 생성되었던 프로세스
			}
			WCHAR	szPath[AGENT_PATH_SIZE] = L"";
			PWSTR	pFileName = NULL;
			if (false == GetFilePath(p->szPath, szPath, sizeof(szPath)))
				StringCbCopy(szPath, sizeof(szPath), p->szPath);			CString	path = pClass->PathConvertor()->MakeSymbolicPath(p->szPath);
			pClass->Log("PROCESS_STOP :%6d %ws", p->dwProcessId, szPath);

			/*
			pClass->Log("MessageId    :%I64d", nMessageId);
			pClass->Log("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
				p->uuid.Data1, p->uuid.Data2, p->uuid.Data3,
				p->uuid.Data4[0], p->uuid.Data4[1], p->uuid.Data4[2], p->uuid.Data4[3],
				p->uuid.Data4[4], p->uuid.Data4[5], p->uuid.Data4[6], p->uuid.Data4[7]);
			pClass->Log("%ws", p->szPath);
			pClass->Log("%ws", p->szCommand);
			pClass->Log("%s", TEXTLINE);
			*/
		}
		else
		{
			pClass->Log("%s unknown", __FUNCTION__);
		}
		return true;
	}

private:

	bool	Start(PCWSTR pProcGuid, PCWSTR pProcPath, PCWSTR pCmdLine, int PID, int PPID) {
		if( false == Db()->IsOpened() )	return false;
		if( NULL == pProcPath )			return false;

		bool			bRet	= false;
		const char		*pQuery	=	"insert into process_history"	\
									"(ProcGuid,ProcPath,ProcName,procName,CmdLine,PID,PPID) " \
									"values(?,?,?,?,?,?,?)";
		sqlite3_stmt	*pStmt	= Db()->Stmt(pQuery);
		if (pStmt) {
			int		nIndex		= 0;
			PCWSTR	pProcName	= wcsrchr(pProcPath, '\\');
			sqlite3_bind_text16(pStmt, ++nIndex, pProcGuid, -1, SQLITE_STATIC);
			sqlite3_bind_text16(pStmt, ++nIndex, pProcPath, -1, SQLITE_STATIC);
			sqlite3_bind_text16(pStmt, ++nIndex, pProcName? pProcName+1 : L"", -1, SQLITE_TRANSIENT);
			sqlite3_bind_text16(pStmt, ++nIndex, pCmdLine, -1, SQLITE_STATIC);
			sqlite3_bind_int(	pStmt, ++nIndex, PID);
			sqlite3_bind_int(	pStmt, ++nIndex, PPID);
			if( SQLITE_DONE == sqlite3_step(pStmt) )	bRet	= true;
			Db()->Free(pStmt);
		}		
		return bRet;
	}

};
