#pragma once
#pragma comment(lib, "Rpcrt4.lib")

#define	GUID_STRLEN	37
#define USE_PROCESS_LOG		1

class CProcess
	:
	public Y_PROCESS_DATA
{
public:
	CProcess(PY_PROCESS_DATA _p) 
		:
		dwLastTick(GetTickCount64()),
		bUpdate(true),
		ProcPathUID(0),
		ProcNameUID(0),
		DevicePathUID(0),
		CommandUID(0)
	{
		CopyMemory(dynamic_cast<PY_PROCESS_DATA>(this), _p, sizeof(Y_PROCESS_DATA));
		ZeroMemory(&registry, sizeof(registry));
	}
	STRUID			ProcPathUID;
	STRUID			ProcNameUID;
	STRUID			DevicePathUID;
	STRUID			CommandUID;
	DWORD64			dwLastTick;
	bool			bUpdate;
	REG_COUNT		registry;
	ModuleMap		module;
};

typedef std::shared_ptr<CProcess>			ProcessPtr;
typedef std::map<PROCUID, ProcessPtr>		ProcessMap;

class CProcessCallback
	:
	protected		IEventCallback,
	public virtual	CAppLog,
	virtual public	CStringTable
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
	virtual	BootUID			GetBootUID()	= NULL;
	virtual	CDB *			Db()	= NULL;
	virtual	PCWSTR			UUID2String(IN UUID * p, PWSTR pValue, DWORD dwSize)	= NULL;
	virtual	bool			SendMessageToWebApp(Json::Value & req, Json::Value & res) = NULL;
	virtual	INotifyCenter *	NotifyCenter() = NULL;
	virtual	uint64_t		GetTimestamp(LARGE_INTEGER *)	= NULL;
	virtual	bool			GetModules2(DWORD PID, PVOID pContext, ModuleListCallback2 pCallback)	= NULL;

	PCSTR			Name() {
		return m_name.c_str();
	}
	void		Create()
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
		NotifyCenter()->RegisterNotifyCallback(__func__, NOTIFY_TYPE_AGENT, NOTIFY_EVENT_PERIODIC, 90, 
							this, PeriodicCallback);
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
	bool		AddModule(PROCUID PUID, PMODULE p) {	
		bool	bRet	= false;
		m_lock.Lock(p, [&](PVOID pContext) {
			try {
				auto	t	= m_table.find(PUID);
				if( m_table.end() == t ) {
					return;
				}
				ModulePtr	mptr	= std::make_shared<CModule>(p);
				mptr->FullNameUID	= GetStrUID(StringModulePath, p->FullName);
				mptr->BaseNameUID	= GetStrUID(StringModuleName, p->BaseName);
				std::pair<ModuleMap::iterator, bool>	ret	= 
					t->second->module.insert(std::pair(p->ImageBase, mptr));
				bRet	= ret.second;
			}
			catch( std::exception & e) {
				m_log.Log("CProcessCallback::AddModule", e.what());
			}
		});
		return true;
	}
	bool		FindModule(PROCUID PUID, PVOID pStartAddress,
					PVOID	pContext, std::function<void (PVOID, CProcess *, CModule *, PVOID)> pCallback)
	
	{	
		bool			bRet	= false;
		std::wstring	str;

		m_lock.Lock(NULL, [&](PVOID pContext) {
			try {
				auto	t	= m_table.find(PUID);
				if( m_table.end() == t ) {
					if( pCallback ) {
						pCallback(pContext, NULL, NULL, NULL);
					}				
					return;
				}
				//m_log.Log("* %ws", GetString(t->second->ProcPathUID, str));
				//m_log.Log("%p THREAD", pStartAddress);
				for( auto i : t->second->module ) {
					if( pStartAddress >= i.second->ImageBase 	&&
						(ULONG_PTR)pStartAddress <= ((ULONG_PTR)i.second->ImageBase + (ULONG_PTR)i.second->ImageSize)) {
						if( pCallback ) {
							pCallback(pContext, t->second.get(), i.second.get(), &t->second->module);
						}
						bRet	= true;
						break;
					}				
				}
				if( false == bRet ) {
					if( pCallback ) {
						pCallback(pContext, t->second.get(), NULL, &t->second->module);
					}				
				}
			}
			catch( std::exception & e) {
				m_log.Log("CProcessCallback::AddModule", e.what());
			}
		});
		return bRet;
	}
	bool		GetProcess(PROCUID PUID, 
		PVOID	pContext, std::function<void (PVOID, CProcess *)> pCallback)
	{	
		bool			bRet	= false;
		std::wstring	str;

		m_lock.Lock(NULL, [&](PVOID pContext) {
			try {
				auto	t	= m_table.find(PUID);
				if( m_table.end() == t ) {
					m_log.Log("%-32s PUID:%p is not found.", __func__, PUID);
					return;
				}
				if( pCallback ) {
					pCallback(pContext, t->second.get());
				}
			}
			catch( std::exception & e) {
				m_log.Log("CProcessCallback::GetProcess", e.what());
			}
		});
		return bRet;
	}
protected:
	void	Dump(CProcess * p) {
		m_log.Log("  mode     :%d", p->mode);
		m_log.Log("  category :%d", p->category);
		m_log.Log("  wDataSize:%d", p->wDataSize);

		m_log.Log("  wRevision:%d", p->wRevision);
		m_log.Log("  subType  :%d", p->subType);

		m_log.Log("  PID      :%d", p->PID);
		m_log.Log("  TID      :%d", p->TID);
		m_log.Log("  CTID     :%d", p->CTID);
		m_log.Log("  CPID     :%d", p->CPID);
		m_log.Log("  PPID     :%p", p->PPID);
		m_log.Log("  RPID     :%p", p->PPID);

		m_log.Log("  PUID     :%p", p->PUID);
		m_log.Log("  PPUID    :%p", p->PPUID);
		m_log.Log("  CREATE   :%p", p->times.CreateTime.QuadPart);
		if( p->times.CreateTime.QuadPart ) {
			char	szTime[30]	= "";
			CTime::LaregInteger2LocalTimeString(&p->times.CreateTime, szTime, sizeof(szTime));
			m_log.Log("           :%s", szTime);
		}
		m_log.Log("  EXIT     :%p", p->times.ExitTime.QuadPart);
		if( p->times.ExitTime.QuadPart ) {
			char	szTime[30]	= "";
			CTime::LaregInteger2LocalTimeString(&p->times.ExitTime, szTime, sizeof(szTime));
			m_log.Log("           :%s", szTime);
		}
		m_log.Log("  KERNEL   :%p", p->times.KernelTime.QuadPart);
		m_log.Log("  USER     :%p", p->times.UserTime.QuadPart);

		//m_log.Log("  DevicePat:%ws [%d]", p->DevicePath.pBuf, p->DevicePath.wSize);
		//m_log.Log("  ProcPath :%ws", szWinPath);
		//m_log.Log("  Command  :%ws [%d]", p->Command.pBuf, p->Command.wSize);
		//pClass->m_log.Log("  Size     :%d", p->nDataSize);	
		m_log.Log(TEXTLINE);

	}
	static	bool			Proc2
	(
		PY_HEADER			pMessage,
		PVOID				pContext
	) 
	{
		PY_PROCESS_MESSAGE	p		= (PY_PROCESS_MESSAGE)pMessage;
		CProcessCallback	*pClass = (CProcessCallback *)pContext;
		SetStringOffset(p, &p->DevicePath);
		SetStringOffset(p, &p->Command);

		/*
		pClass->m_log.Log("  mode     :%d", p->mode);
		pClass->m_log.Log("  category :%d", p->category);
		pClass->m_log.Log("  wDataSize:%d", p->wDataSize);
		pClass->m_log.Log("  wStringSize:%d", p->wStringSize);
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
		pClass->m_log.Log("  CREATE   :%p", p->times.CreateTime.QuadPart);

		pClass->m_log.Log("  DevicePat:%d %d", p->DevicePath.wOffset, p->DevicePath.wSize);
		pClass->m_log.Log("  Command  :%d %d", p->Command.wOffset, p->Command.wSize);
		*/

		pClass->m_lock.Lock(p, [&](PVOID pContext) {
			try {
				auto	t	= pClass->m_table.find(p->PUID);
				if( pClass->m_table.end() == t ) {
					ProcessPtr	ptr	= std::make_shared<CProcess>(dynamic_cast<PY_PROCESS_DATA>(p));

					WCHAR	szPath[AGENT_PATH_SIZE]	= L"";
					if( false == CAppPath::GetFilePath(p->DevicePath.pBuf, szPath, sizeof(szPath)) )
						StringCbCopy(szPath, sizeof(szPath), p->DevicePath.pBuf);
					PCWSTR	pName	= wcsrchr(szPath, L'\\');

					ptr->ProcPathUID	= pClass->GetStrUID(StringProcPath, szPath);
					ptr->ProcNameUID	= pClass->GetStrUID(StringProcName, pName? pName + 1 : szPath);
					ptr->CommandUID		= pClass->GetStrUID(StringCommand, p->Command.pBuf);
					ptr->DevicePathUID	= pClass->GetStrUID(StringProcDevicePath, p->DevicePath.pBuf);
					pClass->m_table[ptr->PUID]	= ptr;
				}
				else {
					t->second->times	= p->times;
					t->second->subType	= p->subType;
					t->second->registry	= p->registry;
				}
				pClass->m_log.Log("[%d] C:%05d P:%05d %ws", p->subType, pClass->m_table.size(), p->PID, p->DevicePath.pBuf);
				t	= pClass->m_table.find(p->PUID);
				if( pClass->m_table.end() == t ) {
					pClass->m_log.Log("%-32s internal error", "CProcessCallback::Proc2");				
				}
				else {
					if( YFilter::Message::SubType::ProcessStart2 == p->subType ) {
						pClass->GetModules2(p->PID, NULL, [&](PVOID pContext, PMODULEENTRY32 pModule)->bool {
							if( p ) {
							
								ModulePtr	mptr	= std::make_shared<CModule>(pModule);
								mptr->BaseNameUID	= pClass->GetStrUID(StringModuleName, pModule->szModule);
								mptr->FullNameUID	= pClass->GetStrUID(StringModulePath, pModule->szExePath);
								t->second->module[mptr->ImageBase]	= mptr;
							}
							else {
								pClass->m_log.Log("PID:%d module is not found.", p->PID);	
							}
							return true;
						});		
					}
					if( false ) {
						pClass->m_log.Log("REG_COUNT:");
						pClass->m_log.Log("  GetValue  %05d %08d", p->registry.GetValue.nCount, (int)p->registry.GetValue.nSize);
						pClass->m_log.Log("  SetValue  %05d %08d", p->registry.SetValue.nCount, (int)p->registry.SetValue.nSize);
						pClass->m_log.Log("  CreateKey %05d %08d", p->registry.CreateKey.nCount, (int)p->registry.CreateKey.nSize);
						pClass->m_log.Log("  DeleteKey %05d %08d", p->registry.DeleteKey.nCount, (int)p->registry.DeleteKey.nSize);
						pClass->m_log.Log("MODULE_COUNT:%d", t->second->module.size());
					}
				}
				//pClass->m_log.Log("%04d [%d] %p %ws", pClass->m_table.size(), p->subType, t->second->p->PID, t->second->ProcPath);
			}
			catch( std::exception & e) {
				pClass->m_log.Log("Proc2", e.what());
			}
		});
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
	CLock				m_lock;
	ProcessMap			m_table;
	bool	IsExisting(
		CProcess	*p
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
		return nCount? true : false;
	}
	bool	Insert(
		CProcess	*p
	) {
		bool			bRet	= false;
		sqlite3_stmt	*pStmt	= m_stmt.pInsert;

		if (pStmt) {

			std::wstring	strProcPath;
			std::wstring	strDevicePath;
			std::wstring	strCommand;
			std::wstring	strName;

			CStringTable::Lock(NULL, [&](PVOID pContext) {
				GetString(p->ProcPathUID, strProcPath);
				GetString(p->DevicePathUID, strDevicePath);
				GetString(p->CommandUID, strCommand);
				GetString(p->ProcNameUID, strName);
			});

			int		nIndex = 0;
			PCWSTR	pProcName = wcsrchr(strProcPath.c_str(), '\\');
			if( pProcName )	pProcName++ ;
			else			pProcName	= strProcPath.c_str();
			sqlite3_bind_int64(pStmt,	++nIndex, p->PUID);
			sqlite3_bind_int64(pStmt,	++nIndex, YAgent::GetBootUID());
			sqlite3_bind_int(pStmt,		++nIndex, p->PID);
			sqlite3_bind_int(pStmt,		++nIndex, p->CPID);
			ProcessIdToSessionId(p->PID, &p->SID);
			sqlite3_bind_int(pStmt,		++nIndex, p->SID);

			sqlite3_bind_int64(pStmt,	++nIndex, p->PPUID);
			sqlite3_bind_int(pStmt,		++nIndex, p->PPID);
			sqlite3_bind_text16(pStmt,	++nIndex, strDevicePath.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text16(pStmt,	++nIndex, strProcPath.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text16(pStmt,	++nIndex, pProcName, -1, SQLITE_STATIC);

			sqlite3_bind_int(pStmt,		++nIndex, 0);
			sqlite3_bind_text16(pStmt,	++nIndex, strCommand.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_null(pStmt,	++nIndex);

			if( p->times.CreateTime.QuadPart )
				sqlite3_bind_int64(pStmt,	++nIndex, GetTimestamp(&p->times.CreateTime));
			else 
				sqlite3_bind_null(pStmt, ++nIndex);
			if( p->times.ExitTime.QuadPart )
				sqlite3_bind_int64(pStmt,	++nIndex, GetTimestamp(&p->times.ExitTime));
			else 
				sqlite3_bind_null(pStmt, ++nIndex);
			sqlite3_bind_int64(pStmt,	++nIndex, p->times.KernelTime.QuadPart);
			sqlite3_bind_int64(pStmt,	++nIndex, p->times.UserTime.QuadPart);

			if (SQLITE_DONE == sqlite3_step(pStmt))	bRet = true;
			else {
				m_log.Log("%s", sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		} 
		//m_log.Log("%-32s %d", __func__, bRet);
		return bRet;
	}
	bool			Update
	(
		CProcess	*p
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
				if( p->PID ) {
					m_log.Log("%-32s CreateTime is null", __func__);
					Dump(p);
				}
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
	void			UpdateProcessTime(CProcess * p) {
		HANDLE		h		= YAgent::GetProcessHandle(p->PID);
		FILETIME	t[4]	= {0,};
		if( h ) {
			if( GetProcessTimes(h, &t[0], &t[1], &t[2], &t[3]) ) {
				p->times.CreateTime.HighPart	= t[0].dwHighDateTime;
				p->times.CreateTime.LowPart		= t[0].dwLowDateTime;
				p->times.ExitTime.HighPart		= t[1].dwHighDateTime;
				p->times.ExitTime.LowPart		= t[1].dwLowDateTime;
				p->times.KernelTime.HighPart	= t[2].dwHighDateTime;
				p->times.KernelTime.LowPart		= t[2].dwLowDateTime;
				p->times.UserTime.HighPart		= t[3].dwHighDateTime;
				p->times.UserTime.LowPart		= t[3].dwLowDateTime;
			}
			CloseHandle(h);
		}
	}
	static	void	PeriodicCallback(
		WORD wType, WORD wEvent, PVOID pData, ULONG_PTR nDataSize, PVOID pContext
	) {
		CProcessCallback	*pClass	= (CProcessCallback *)pContext;

		static	DWORD	dwCount;

		//pClass->m_log.Log("%s %4d %4d", __FUNCTION__, wType, wEvent);
		//pClass->Db()->Begin(__func__);
		pClass->m_lock.Lock(NULL, [&](PVOID pContext) {
			try {
				UNREFERENCED_PARAMETER(pContext);		
				DWORD64	dwTick	= GetTickCount64();
				for( auto t = pClass->m_table.begin() ; t != pClass->m_table.end() ; ) {		
					if( YFilter::Message::SubType::ProcessStop == t->second->subType ) {
						t->second->bUpdate	= true;
					}
					else {
						//pClass->m_log.Log("%I64d - %I64d = %I64d", dwTick, t->second->dwLastTick, dwTick-t->second->dwLastTick);
						if( (dwTick - t->second->dwLastTick) > (180 * 1000) ) {
							pClass->UpdateProcessTime(t->second.get());
							t->second->bUpdate		= true;
							t->second->dwLastTick	= dwTick;
						}
					}
					std::wstring	strPath;
					std::wstring	strName;

					pClass->CStringTable::Lock(NULL, [&](PVOID pContext) {
						pClass->GetString(t->second->ProcPathUID, strPath);
						pClass->GetString(t->second->ProcNameUID, strName);
					});

					if( t->second->bUpdate ) {
						if (pClass->IsExisting(t->second.get())) {
							//pClass->m_log.Log("[U] %p %I64d %ws", t->second->PID, t->second->dwLastTick, strName.c_str());
							pClass->Update(t->second.get());
						}
						else	{
							pClass->Insert(t->second.get());	
							//pClass->m_log.Log("[I] %p %I64d %ws", t->second->PID, t->second->dwLastTick, strPath.c_str());
						}			
						t->second->bUpdate	= false;
					}
					if( YFilter::Message::SubType::ProcessStop == t->second->subType ) {
						//pClass->m_log.Log("[D] %p %I64d %ws", t->second->PID, t->second->dwLastTick, strName.c_str());
						auto	d	= t++;
						pClass->m_table.erase(d);
					}
					else {
						t++;					
					}
				}
			}
			catch( std::exception & e) {
				pClass->m_log.Log("Proc2", e.what());
			}
		});
		//pClass->Db()->Commit(__func__);
	}
};
