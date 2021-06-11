#pragma once

#pragma comment(lib, "Rpcrt4.lib")

#define	GUID_STRLEN	37
#define USE_PROCESS_LOG		1

class CProcessCallback
	:
	protected		IEventCallback,
	public virtual	CAppLog
{
public:
	CProcessCallback() 
		:	m_log(L"process.log")
	
	{
		m_name = EVENT_CALLBACK_NAME;
		m_log.Log(NULL);
	}
	~CProcessCallback() {
		m_log.Log("---- end ----");
	}
	virtual	BootUID	GetBootUID()	= NULL;
	virtual	CDB *	Db()	= NULL;
	virtual	PCWSTR	UUID2String(IN UUID * p, PWSTR pValue, DWORD dwSize)	= NULL;
	virtual	bool	SendMessageToWebApp(Json::Value & req, Json::Value & res) = NULL;
	PCSTR			Name() {
		return m_name.c_str();
	}
	void			Create()
	{
		const char	*pIsExisting	= "select count(PUID) from process where PUID=?";
		const char	*pInsert		= "insert into process"	\
			"("\
			"PUID,BootUID,PID,CPID,SID,"	\
			"PPUID,PPID,DevicePath,ProcPath,ProcName,"	\
			"IsSystem,CmdLine,UserId,CreateTime,ExitTime,"\
			"KernelTime,UserTime"\
			") "\
			"values("\
			"?,?,?,?,?"\
			",?,?,?,?,?"\
			",?,?,?,?,?"\
			",?,?)";
		const char	*pUpdate		= "update process "	\
			"set CreateTime=?, ExitTime=?, KernelTime=?, UserTime=? "\
			"where PUID=?";
		const char	*pSelect		= "select ProcPath,PID "\
			"from	process "\
			"where	PUID=?";
		const char*	pProcessList	= "select ProcName, ProcPath, count(*) cnt, sum(KernelTime+UserTime) time, sum(KernelTime) ktime, sum(UserTime) utime "	\
			"from process "	\
			"where BootUID=? "	\
			"group by ProcPath "	\
			"order by time desc";

		if (Db()->IsOpened()) {
	
			Db()->AddStmt("process.list", pProcessList);
			if( NULL == (m_stmt.pIsExisting	= Db()->Stmt(pIsExisting)) )
				m_log.Log("%s", sqlite3_errmsg(Db()->Handle()));
			if( NULL == (m_stmt.pInsert	= Db()->Stmt(pInsert)) )
				m_log.Log("%s", sqlite3_errmsg(Db()->Handle()));
			if( NULL == (m_stmt.pUpdate	= Db()->Stmt(pUpdate)) )
				m_log.Log("%s", sqlite3_errmsg(Db()->Handle()));
			if (NULL == (m_stmt.pSelect = Db()->Stmt(pSelect)))
				m_log.Log("%s", sqlite3_errmsg(Db()->Handle()));
			if (NULL == (m_stmt.pProcessList = Db()->Stmt(pProcessList)))
				m_log.Log("%s", sqlite3_errmsg(Db()->Handle()));
		}
		else {
			ZeroMemory(&m_stmt, sizeof(m_stmt));
		}
	}
	void		Destroy()
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
		Log("%-32s BootUID=%p", __FUNCTION__, GetBootUID());
		if (pStmt) {
			int			nIndex = 0;
			int			nColumnCount	= sqlite3_column_count(pStmt);
			const char	*pColumnName	= NULL;

			for( auto i = 0 ; i < nColumnCount ; i++ ) {
				pColumnName	= sqlite3_column_name(pStmt, i);			
			Log("%d %s", i, pColumnName);
			}
			sqlite3_bind_int64(pStmt, ++nIndex, GetBootUID());
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
	bool		GetProcess(PROCUID PUID, PWSTR pValue, DWORD dwSize)
	{
		bool	bRet = false;
		sqlite3_stmt* pStmt = m_stmt.pSelect;
		if (pStmt) {
			int		nIndex = 0;
			sqlite3_bind_int64(pStmt, ++nIndex, PUID);
			if (SQLITE_ROW == sqlite3_step(pStmt)) {
				PCWSTR	ProcPath = NULL;
				DWORD	PID = 0;
				ProcPath = (PCWSTR)sqlite3_column_text16(pStmt, 0);
				PID = sqlite3_column_int(pStmt, 1);
				StringCbCopy(pValue, dwSize, ProcPath ? ProcPath : L"");
				bRet = true;
			}
			sqlite3_reset(pStmt);
		}
		return bRet;
	}
protected:
	static	bool			Proc2
	(
		PY_HEADER			pMessage,
		PVOID				pContext
	) 
	{
		PY_PROCESS			p		= (PY_PROCESS)pMessage;
		CProcessCallback	*pClass = (CProcessCallback *)pContext;

#if defined(USE_PROCESS_LOG) && 1 == USE_PROCESS_LOG 
		pClass->m_log.Log("  mode     :%d", p->mode);
		pClass->m_log.Log("  category :%d", p->category);
		pClass->m_log.Log("  wSize    :%d", p->wSize);
		pClass->m_log.Log("  wRevision:%d", p->wRevision);
		pClass->m_log.Log("  subType  :%d", p->subType);

		pClass->m_log.Log("  PID      :%d", p->PID);
		pClass->m_log.Log("  TID      :%d", p->TID);
		pClass->m_log.Log("  CTID     :%d", p->CTID);
		pClass->m_log.Log("  CPID     :%d", p->CPID);
		pClass->m_log.Log("  PPID     :%p", p->PPID);
		pClass->m_log.Log("  RPID     :%p", p->PPID);

		pClass->m_log.Log("  PUID     :%p", p->PUID);
		pClass->m_log.Log("  PPUID    :%p", p->PPUID);
#endif

		p->DevicePath.pBuf	= (PWSTR)((char *)p + p->DevicePath.wOffset);						
		p->Command.pBuf		= (PWSTR)((char *)p + p->Command.wOffset);

		WCHAR		szWinPath[AGENT_PATH_SIZE]	= L"";

		if( false == CAppPath::GetFilePath(p->DevicePath.pBuf, szWinPath, sizeof(szWinPath)) )
			StringCbCopy(szWinPath, sizeof(szWinPath), p->DevicePath.pBuf);

#if defined(USE_PROCESS_LOG) && 1 == USE_PROCESS_LOG 
		pClass->m_log.Log("  DevicePat:%ws [%d]", p->DevicePath.pBuf, p->DevicePath.wSize);
		pClass->m_log.Log("  ProcPath :%ws", szWinPath);
		pClass->m_log.Log("  Command  :%ws [%d]", p->Command.pBuf, p->Command.wSize);
		//pClass->m_log.Log("  Size     :%d", p->nDataSize);	
		pClass->m_log.Log(TEXTLINE);
#endif

		if (pClass->IsExisting(p))
			pClass->Update(p);
		else	pClass->Insert(p, szWinPath);
		return true;
	}

	static	bool			Proc(
		ULONGLONG			nMessageId,
		PVOID				pCallbackPtr,
		PYFILTER_DATA		p
	) 
	{
	/*
		WCHAR	szProcGuid[GUID_STRLEN] = L"";
		WCHAR	szPProcGuid[GUID_STRLEN] = L"";
		WCHAR	szPath[AGENT_PATH_SIZE] = L"";
		PWSTR	pFileName = NULL;
		char	szTime[40] = "";

		Json::Value	req;

		CProcessCallback	*pClass = (CProcessCallback *)pCallbackPtr;
		pClass->UUID2String(&p->ProcGuid, szProcGuid, sizeof(szProcGuid));

		req["ProcPath"]		= __utf8(p->szPath);
		req["SubType"]		= p->subType;
		req["PID"]			= (int)p->PID;
		req["PPID"]			= (int)p->PPID;

		if (YFilter::Message::SubType::ProcessStart		== p->subType	||
			YFilter::Message::SubType::ProcessStart2	== p->subType	) {
			pClass->UUID2String(&p->PProcGuid, szPProcGuid, sizeof(szPProcGuid));
			if( 0 == p->PPID || 4 == p->PPID )
				pClass->Log("%s %6d %ws", __FUNCTION__, p->PID, p->szPath);
			if( false == CAppPath::GetFilePath(p->szPath, szPath, sizeof(szPath)) )
				StringCbCopy(szPath, sizeof(szPath), p->szPath);
			if (pClass->IsExisting(szProcGuid, p->PUID))
				pClass->Update(szProcGuid, p->PUID, p);
			else	pClass->Insert(szProcGuid, p->PUID, szPath, szPProcGuid, p);
			SYSTEMTIME	stCreate;
			SYSTEMTIME	stExit;
			SYSTEMTIME	stKernel;
			SYSTEMTIME	stUser;

			CTime::LargeInteger2SystemTime(&p->times.CreateTime, &stCreate);
			CTime::LargeInteger2SystemTime(&p->times.ExitTime, &stExit);
			CTime::LargeInteger2SystemTime(&p->times.KernelTime, &stKernel, false);
			CTime::LargeInteger2SystemTime(&p->times.UserTime, &stUser, false);

			if( 0 == p->PUID ) {
				pClass->m_log.Log("%-20s(%d) %6d %ws", "PROCESS_START", p->subType, p->PID, szPath);
				pClass->m_log.Log("  path          %ws", p->szPath);
				pClass->m_log.Log("  MessageId     %I64d", nMessageId);
				pClass->m_log.Log("  ProcGuid      %ws", szProcGuid);
				pClass->m_log.Log("  PUID          %p",	p->PUID);
				pClass->m_log.Log("  PPUID         %p",	p->PPUID);
				pClass->m_log.Log("  PID           %d", p->PID);
				pClass->m_log.Log("  PPID          %d", p->PPID);
				pClass->m_log.Log("  command       %ws", p->szCommand);
				pClass->m_log.Log("  create        %s", SystemTime2String(&stCreate, szTime, sizeof(szTime)));
				pClass->m_log.Log("  kernel        %s", SystemTime2String(&stKernel, szTime, sizeof(szTime), true));
				pClass->m_log.Log("  user          %s", SystemTime2String(&stUser, szTime, sizeof(szTime), true));
				pClass->m_log.Log("  PProcGuid     %ws", szPProcGuid);
				pClass->m_log.Log("%s", TEXTLINE);
			}
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
			if( pClass->IsExisting(szProcGuid, p->PUID) )
				pClass->Update(szProcGuid, p->PUID, p);
			//else 
			//	pClass->Insert(szProcGuid, szPath, szPProcGuid, p);

			if( 0 == p->PUID) {
				pClass->m_log.Log("%-20s %6d %ws", "PROCESS_STOP", p->PID, szPath);
				pClass->m_log.Log("  path          %ws", p->szPath);
				pClass->m_log.Log("  ProdGuid      %ws", UUID2StringW(&p->ProcGuid, szProcGuid, sizeof(szProcGuid)));
				pClass->m_log.Log("  PUID          %p",	p->PUID);
				pClass->m_log.Log("  PPUID         %p",	p->PPUID);
				pClass->m_log.Log("  create        %s", SystemTime2String(&stCreate, szTime, sizeof(szTime)));
				pClass->m_log.Log("  exit          %s", SystemTime2String(&stExit, szTime, sizeof(szTime)));
				pClass->m_log.Log("  kernel        %s", SystemTime2String(&stKernel, szTime, sizeof(szTime), true));
				pClass->m_log.Log("  user          %s", SystemTime2String(&stUser, szTime, sizeof(szTime), true));
				pClass->m_log.Log("%s", TEXTLINE);
			}
			pClass->Log("MessageId    :%I64d", nMessageId);
			pClass->Log("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
				p->uuid.Data1, p->uuid.Data2, p->uuid.Data3,
				p->uuid.Data4[0], p->uuid.Data4[1], p->uuid.Data4[2], p->uuid.Data4[3],
				p->uuid.Data4[4], p->uuid.Data4[5], p->uuid.Data4[6], p->uuid.Data4[7]);
			pClass->Log("%ws", p->szPath);
			pClass->Log("%ws", p->szCommand);
			pClass->Log("%s", TEXTLINE);
		}
		else
		{
			pClass->Log("%s unknown", __FUNCTION__);
		}

		Json::Value		res;
		//pClass->SendMessageToWebApp(req, res);
		*/
		return true;
	}

private:
	std::string			m_name;
	CAppLog				m_log;
	struct {
		sqlite3_stmt	*pInsert;
		sqlite3_stmt	*pUpdate;
		sqlite3_stmt	*pIsExisting;
		sqlite3_stmt	*pSelect;
		sqlite3_stmt	*pProcessList;
	}	m_stmt;

	bool	IsExisting(
		PY_PROCESS	p
	) {
		int			nCount	= 0;
		sqlite3_stmt* pStmt = m_stmt.pIsExisting;
		if (pStmt) {
			int		nIndex = 0;
			sqlite3_bind_int64(pStmt, ++nIndex, p->PUID);
			if (SQLITE_ROW == sqlite3_step(pStmt))	{
				nCount	= sqlite3_column_int(pStmt, 0);
			}
			else {
				m_log.Log("%s", sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		}
		m_log.Log("%-32s %d", __func__, nCount);
		return nCount? true : false;
	}
	/*
			const char	*pInsert		= "insert into process"	\
			"("\
			"PUID,BootUID,PID,CPID,SID,"	\
			"PPUID,PPID,DevicePath,ProcPath,ProcName,"	\
			"IsSystem,CmdLine,UserId,CreateTime,ExitTime,"\
			"KernelTime,UserTime"\
			") "\
			"values("\
			"?,?,?,?,?"\
			",?,?,?,?,?"\
			",?,?,?,?,?"\
			",?,?)";
	*/
	bool	Insert(
		PY_PROCESS	p,
		PCWSTR		pProcPath
	) {
		bool			bRet	= false;
		sqlite3_stmt	*pStmt	= m_stmt.pInsert;
		if (pStmt) {
			int		nIndex = 0;
			PCWSTR	pProcName = wcsrchr(pProcPath, '\\');
			if( pProcName )	pProcName++ ;
			else			pProcName	= pProcPath;
			sqlite3_bind_int64(pStmt,	++nIndex, p->PUID);
			sqlite3_bind_int64(pStmt,	++nIndex, YAgent::GetBootUID());
			sqlite3_bind_int(pStmt,		++nIndex, p->PID);
			sqlite3_bind_int(pStmt,		++nIndex, p->CPID);
			ProcessIdToSessionId(p->PID, &p->SID);
			sqlite3_bind_int(pStmt,		++nIndex, p->SID);

			sqlite3_bind_int64(pStmt,	++nIndex, p->PPUID);
			sqlite3_bind_int(pStmt,		++nIndex, p->PPID);
			sqlite3_bind_text16(pStmt,	++nIndex, p->DevicePath.pBuf, -1, SQLITE_STATIC);
			sqlite3_bind_text16(pStmt,	++nIndex, pProcPath, -1, SQLITE_STATIC);
			sqlite3_bind_text16(pStmt,	++nIndex, pProcName, -1, SQLITE_STATIC);

			sqlite3_bind_int(pStmt,		++nIndex, p->bIsSystem);
			sqlite3_bind_text16(pStmt,	++nIndex, p->Command.pBuf, -1, SQLITE_STATIC);
			sqlite3_bind_null(pStmt,	++nIndex);
			sqlite3_bind_int64(pStmt,	++nIndex, CTime::LargeInteger2UnixTimestamp(&p->times.CreateTime));
			sqlite3_bind_int64(pStmt,	++nIndex, CTime::LargeInteger2UnixTimestamp(&p->times.ExitTime));

			sqlite3_bind_int64(pStmt,	++nIndex, p->times.KernelTime.QuadPart);
			sqlite3_bind_int64(pStmt,	++nIndex, p->times.UserTime.QuadPart);

			if (SQLITE_DONE == sqlite3_step(pStmt))	bRet = true;
			else {
				m_log.Log("%s", sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		} 
		m_log.Log("%-32s %d", __func__, bRet);
		return bRet;
	}
	/*
		const char	*pUpdate		= "update process "	\
			"set CreateTime=?, ExitTime=?, KernelTime=?, UserTime=? "\
			"where PUID=?";
	*/
	bool	Update
	(
		PY_PROCESS		p
	) {
		bool			bRet	= false;
		sqlite3_stmt	*pStmt	= m_stmt.pUpdate;
		if (pStmt) {
			int		nIndex = 0;
			char		szTime[50] = "";
			if (p->times.CreateTime.QuadPart) {
				sqlite3_bind_text(pStmt, ++nIndex,
					CTime::LaregInteger2LocalTimeString(&p->times.CreateTime, szTime, sizeof(szTime)),
					-1, SQLITE_TRANSIENT);
			}
			else {
				Log("---- CreateTime is null");
				sqlite3_bind_null(pStmt, ++nIndex);
			}
			if (p->times.ExitTime.QuadPart) {
				sqlite3_bind_text(pStmt, ++nIndex,
					CTime::LaregInteger2LocalTimeString(&p->times.ExitTime, szTime, sizeof(szTime)),
					-1, SQLITE_STATIC);
			}
			else
				sqlite3_bind_null(pStmt, ++nIndex);
			sqlite3_bind_int64(pStmt, ++nIndex, p->times.KernelTime.QuadPart);
			sqlite3_bind_int64(pStmt, ++nIndex, p->times.UserTime.QuadPart);
			sqlite3_bind_int64(pStmt, ++nIndex, p->PUID);
			if (SQLITE_DONE == sqlite3_step(pStmt))	bRet = true;
			else {
				m_log.Log("%s", sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		}
		return bRet;
	}
};
