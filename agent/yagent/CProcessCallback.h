#pragma once

#pragma comment(lib, "Rpcrt4.lib")

#define	GUID_STRLEN	37

class CProcessCallback
	:
	protected		IEventCallback,
	public virtual	CAppLog
{
public:
	CProcessCallback() {
		m_name = EVENT_CALLBACK_NAME;
	}
	~CProcessCallback() {

	}
	virtual	DWORD	BootId()	= NULL;
	virtual	CDB *	Db()	= NULL;
	virtual	PCWSTR	UUID2String(IN UUID * p, PWSTR pValue, DWORD dwSize)	= NULL;
	PCSTR			Name() {
		return m_name.c_str();
	}
	void			Create()
	{
		const char	*pIsExisting	= "select count(ProcGuid) from process where ProcGuid=?";
		const char	*pInsert		= "insert into process"	\
			"("\
			"ProcGuid,BootId,ProcPath,ProcName,CmdLine,"\
			"PID,PPID,SessionId,PProcGuid,IsSystem,"\
			"ProcUserId,CreateTime,ExitTime,KernelTime,UserTime"\
			") "\
			"values("\
			"?,?,?,?,?"\
			",?,?,?,?,?"\
			",?,?,?,?,?"\
			")";
		const char	*pUpdate		= "update process "	\
			"set CreateTime=?, ExitTime=?, KernelTime=?, UserTime=? "\
			"where ProcGuid=?";
		const char	*pSelect		= "select ProcPath,PID "\
			"from	process "\
			"where	ProcGuid=?";
		const char*	pProcessList	= "select ProcName, ProcPath, count(*) cnt, sum(KernelTime+UserTime) time, sum(KernelTime) ktime, sum(UserTime) utime "	\
			"from process "	\
			"where bootid=? "	\
			"group by ProcPath "	\
			"order by time desc";

		if (Db()->IsOpened()) {
	
			Db()->AddStmt("process.list", pProcessList);

			if( NULL == (m_stmt.pIsExisting	= Db()->Stmt(pIsExisting)) )
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			if( NULL == (m_stmt.pInsert	= Db()->Stmt(pInsert)) )
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			if( NULL == (m_stmt.pUpdate	= Db()->Stmt(pUpdate)) )
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			if (NULL == (m_stmt.pSelect = Db()->Stmt(pSelect)))
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			if (NULL == (m_stmt.pProcessList = Db()->Stmt(pProcessList)))
				Log("%s", sqlite3_errmsg(Db()->Handle()));
		}
		else {
			ZeroMemory(&m_stmt, sizeof(m_stmt));
		}
	}
	void					Destroy()
	{
		if (Db()->IsOpened()) {
			if( m_stmt.pIsExisting)		Db()->Free(m_stmt.pIsExisting);
			if( m_stmt.pInsert)			Db()->Free(m_stmt.pInsert);
			if( m_stmt.pUpdate)			Db()->Free(m_stmt.pUpdate);
			if (m_stmt.pSelect)			Db()->Free(m_stmt.pSelect);
			if (m_stmt.pProcessList)	Db()->Free(m_stmt.pProcessList);
			ZeroMemory(&m_stmt, sizeof(m_stmt));
		}
	}
	DWORD		ProcessList(
		PVOID	pContext,
		std::function<bool 
			(PVOID pContext, PCWSTR pProcName, PCWSTR pProcPath, int nCount)> pCallback
	
	) {
		DWORD			dwCount	= 0;
		sqlite3_stmt	*pStmt	= m_stmt.pProcessList;

		pStmt	= Db()->GetStmt("process.list");
		Log("%-32s BootId=%d", __FUNCTION__, BootId());
		if (pStmt) {
			int			nIndex = 0;
			int			nColumnCount	= sqlite3_column_count(pStmt);
			const char	*pColumnName	= NULL;

			for( auto i = 0 ; i < nColumnCount ; i++ ) {
				pColumnName	= sqlite3_column_name(pStmt, i);			
			Log("%d %s", i, pColumnName);
			}
			sqlite3_bind_int(pStmt, ++nIndex, BootId());
			while ( SQLITE_ROW == sqlite3_step(pStmt)) {
				wchar_t		*pProcName	= (wchar_t *)sqlite3_column_text16(pStmt, 0);
				wchar_t		*pProcPath	= (wchar_t *)sqlite3_column_text16(pStmt, 1);
				int			nCount		= sqlite3_column_int(pStmt, 2);
				if( pCallback ) {
					bool	bRet	= pCallback(pContext, pProcName, pProcPath, nCount);
					if( false == bRet )	break;
				}
				else 
					break;
				dwCount++;
			}
			sqlite3_reset(pStmt);
		}
		else {
			Log("%-32s pStmt is null.", __FUNCTION__);
		}
		return dwCount;
	}
	bool		GetProcess(PCWSTR pProcGuid, PWSTR pValue, DWORD dwSize)
	{
		bool	bRet = false;
		sqlite3_stmt* pStmt = m_stmt.pSelect;
		if (pStmt) {
			int		nIndex = 0;
			sqlite3_bind_text16(pStmt, ++nIndex, pProcGuid, -1, SQLITE_STATIC);
			if (SQLITE_ROW == sqlite3_step(pStmt)) {
				PCWSTR	_ProcPath = NULL;
				DWORD	_PID = 0;
				_ProcPath = (PCWSTR)sqlite3_column_text16(pStmt, 0);
				_PID = sqlite3_column_int(pStmt, 1);
				StringCbCopy(pValue, dwSize, _ProcPath ? _ProcPath : L"");
				bRet = true;
			}
			sqlite3_reset(pStmt);
		}
		return bRet;
	}
protected:
	static	bool			Proc(
		ULONGLONG			nMessageId,
		PVOID				pCallbackPtr,
		PYFILTER_DATA		p
	) 
	{
		WCHAR	szProcGuid[GUID_STRLEN] = L"";
		WCHAR	szPProcGuid[GUID_STRLEN] = L"";
		WCHAR	szPath[AGENT_PATH_SIZE] = L"";
		PWSTR	pFileName = NULL;
		char	szTime[40] = "";

		CProcessCallback	*pClass = (CProcessCallback *)pCallbackPtr;

		pClass->UUID2String(&p->ProcGuid, szProcGuid, sizeof(szProcGuid));
		if (YFilter::Message::SubType::ProcessStart		== p->subType	||
			YFilter::Message::SubType::ProcessStart2	== p->subType	) {
			pClass->UUID2String(&p->PProcGuid, szPProcGuid, sizeof(szPProcGuid));
			if( 0 == p->PPID || 4 == p->PPID )
				pClass->Log("%s %6d %ws", __FUNCTION__, p->PID, p->szPath);
			if( false == CAppPath::GetFilePath(p->szPath, szPath, sizeof(szPath)) )
				StringCbCopy(szPath, sizeof(szPath), p->szPath);
			if (pClass->IsExisting(szProcGuid))
				pClass->Update(szProcGuid, p);
			else	pClass->Insert(szProcGuid, szPath, szPProcGuid, p);
			SYSTEMTIME	stCreate;
			SYSTEMTIME	stExit;
			SYSTEMTIME	stKernel;
			SYSTEMTIME	stUser;

			CTime::LargeInteger2SystemTime(&p->times.CreateTime, &stCreate);
			CTime::LargeInteger2SystemTime(&p->times.ExitTime, &stExit);
			CTime::LargeInteger2SystemTime(&p->times.KernelTime, &stKernel, false);
			CTime::LargeInteger2SystemTime(&p->times.UserTime, &stUser, false);

			//pClass->Log("%-20s %6d %ws", "PROCESS_START", p->dwProcessId, szPath);
			//pClass->Log("  path          %ws", p->szPath);
			//pClass->Log("  MessageId     %I64d", nMessageId);
			//pClass->Log("  ProcGuid      %ws", szProcGuid);
			//pClass->Log("  PID           %d", p->dwProcessId);
			//pClass->Log("  PPID          %d", p->dwParentProcessId);
			//pClass->Log("  command       %ws", p->szCommand);
			//pClass->Log("  create        %s", SystemTime2String(&stCreate, szTime, sizeof(szTime)));
			//pClass->Log("  kernel        %s", SystemTime2String(&stKernel, szTime, sizeof(szTime), true));
			//pClass->Log("  user          %s", SystemTime2String(&stUser, szTime, sizeof(szTime), true));
			//pClass->Log("  PProcGuid     %ws", szPProcGuid);
			//pClass->Log("%s", TEXTLINE);
		}
		else if (YFilter::Message::SubType::ProcessStop == p->subType) {
			if (p->PID)
			{
				//	드라이버에서 생성을 감지했던 프로세스
			}
			else
			{
				//	드라이버 로드전에 이미 생성되었던 프로세스
			}
			SYSTEMTIME	stCreate;
			SYSTEMTIME	stExit;
			SYSTEMTIME	stKernel;
			SYSTEMTIME	stUser;

			CTime::LargeInteger2SystemTime(&p->times.CreateTime, &stCreate);
			CTime::LargeInteger2SystemTime(&p->times.ExitTime, &stExit);
			CTime::LargeInteger2SystemTime(&p->times.KernelTime, &stKernel, false);
			CTime::LargeInteger2SystemTime(&p->times.UserTime, &stUser, false);

			if (false == CAppPath::GetFilePath(p->szPath, szPath, sizeof(szPath)))
				StringCbCopy(szPath, sizeof(szPath), p->szPath);
			if( pClass->IsExisting(szProcGuid) )
				pClass->Update(szProcGuid, p);
			//else 
			//	pClass->Insert(szProcGuid, szPath, szPProcGuid, p);
			//pClass->Log("%-20s %6d %ws", "PROCESS_STOP", p->dwProcessId, szPath);
			//pClass->Log("  path          %ws", p->szPath);
			//pClass->Log("  ProdGuid      %ws", UUID2StringW(&p->ProcGuid, szProcGuid, sizeof(szProcGuid)));
			//pClass->Log("  create        %s", SystemTime2String(&stCreate, szTime, sizeof(szTime)));
			//pClass->Log("  exit          %s", SystemTime2String(&stExit, szTime, sizeof(szTime)));
			//pClass->Log("  kernel        %s", SystemTime2String(&stKernel, szTime, sizeof(szTime), true));
			//pClass->Log("  user          %s", SystemTime2String(&stUser, szTime, sizeof(szTime), true));

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
	std::string			m_name;
	struct {
		sqlite3_stmt	*pInsert;
		sqlite3_stmt	*pUpdate;
		sqlite3_stmt	*pIsExisting;
		sqlite3_stmt	*pSelect;
		sqlite3_stmt	*pProcessList;
	}	m_stmt;

	bool	IsExisting(
		PCWSTR				pProcGuid
	) {
		int			nCount	= 0;
		sqlite3_stmt* pStmt = m_stmt.pIsExisting;
		if (pStmt) {
			int		nIndex = 0;
			sqlite3_bind_text16(pStmt, ++nIndex, pProcGuid, -1, SQLITE_STATIC);
			if (SQLITE_ROW == sqlite3_step(pStmt))	{
				nCount	= sqlite3_column_int(pStmt, 0);
			}
			sqlite3_reset(pStmt);
		}
		return nCount? true : false;
	}
	bool	Insert(
		PCWSTR				pProcGuid, 
		PCWSTR				pProcPath, 
		PCWSTR				pPProcGuid,
		PYFILTER_DATA		pData
	) {
		if (NULL == pProcPath)			return false;

		bool			bRet	= false;
		sqlite3_stmt	*pStmt	= m_stmt.pInsert;
		if (pStmt) {
			int		nIndex = 0;
			PCWSTR	pProcName = wcsrchr(pProcPath, '\\');
			sqlite3_bind_text16(pStmt,	++nIndex, pProcGuid, -1, SQLITE_STATIC);
			sqlite3_bind_int(pStmt,		++nIndex, YAgent::GetBootId());
			sqlite3_bind_text16(pStmt,	++nIndex, pProcPath, -1, SQLITE_STATIC);
			sqlite3_bind_text16(pStmt,	++nIndex, pProcName ? pProcName + 1 : pProcPath, -1, SQLITE_TRANSIENT);
			sqlite3_bind_text16(pStmt,	++nIndex, pData->szCommand, -1, SQLITE_STATIC);

			sqlite3_bind_int(pStmt,		++nIndex, pData->PID);
			sqlite3_bind_int(pStmt,		++nIndex, pData->PPID);
			sqlite3_bind_int(pStmt,		++nIndex, 0);
			if (pPProcGuid && *pPProcGuid)
				sqlite3_bind_text16(pStmt, ++nIndex, pPProcGuid, -1, SQLITE_STATIC);
			else
				sqlite3_bind_null(pStmt, ++nIndex);
			sqlite3_bind_int(pStmt,		++nIndex, false);

			sqlite3_bind_null(pStmt, ++nIndex);
			char		szTime[50]	= "";
			if( pData->times.CreateTime.QuadPart ) {
				sqlite3_bind_text(pStmt,	++nIndex, 
					CTime::LaregInteger2LocalTimeString(&pData->times.CreateTime, szTime, sizeof(szTime)), 
					-1, SQLITE_TRANSIENT);
			}
			else {
				Log("---- CreateTime is null");
				sqlite3_bind_null(pStmt, ++nIndex);
			}
			if( pData->times.ExitTime.QuadPart ) {
				sqlite3_bind_text(pStmt, ++nIndex,
					CTime::LaregInteger2LocalTimeString(&pData->times.ExitTime, szTime, sizeof(szTime)),
					-1, SQLITE_STATIC);
			}
			else 
				sqlite3_bind_null(pStmt, ++nIndex);

			sqlite3_bind_int64(pStmt,	++nIndex, pData->times.KernelTime.QuadPart);
			sqlite3_bind_int64(pStmt,	++nIndex, pData->times.UserTime.QuadPart);
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
		bool			bRet	= false;
		sqlite3_stmt	*pStmt	= m_stmt.pUpdate;
		if (pStmt) {
			int		nIndex = 0;
			char		szTime[50] = "";
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
					-1, SQLITE_STATIC);
			}
			else
				sqlite3_bind_null(pStmt, ++nIndex);
			sqlite3_bind_int64(pStmt, ++nIndex, pData->times.KernelTime.QuadPart);
			sqlite3_bind_int64(pStmt, ++nIndex, pData->times.UserTime.QuadPart);
			sqlite3_bind_text16(pStmt, ++nIndex, pProcGuid, -1, SQLITE_STATIC);
			if (SQLITE_DONE == sqlite3_step(pStmt))	bRet = true;
			else {
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		}
		return bRet;
	}
};
