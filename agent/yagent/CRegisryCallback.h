#pragma once
/*
	

	\REGISTRY\A is a hidden registry hive for use by Windows Store apps (aka Metro-style apps).
	https://superuser.com/questions/689788/what-does-the-path-registry-a-in-sysinternals-procmon-log-mean

*/

class CRegistry
	:
	public	Y_REGISTRY_ENTRY
{
public:
	CRegistry(PY_REGISTRY_DATA _p) 
		:	
		dwLastTick(GetTickCount64())	
	{
		CopyMemory(dynamic_cast<PY_REGISTRY_DATA>(this), _p, sizeof(Y_REGISTRY_DATA));		
	}
	~CRegistry() {
	
	}
	DWORD64		dwLastTick;
};


typedef std::shared_ptr<CRegistry>	RegPtr;
typedef std::map<REGUID, RegPtr>	RegMap;

class CRegistryCallback
	:
	protected		IEventCallback,
	public virtual	CAppLog,
	public	virtual	CStringTable
{
public:
	CRegistryCallback() 
		:	m_log(L"registry.log")
	
	{
		m_name = EVENT_CALLBACK_NAME;
		m_log.Log(NULL);
	}
	~CRegistryCallback() {
		m_log.Log(__func__);
	}
	virtual	BootUID	GetBootUID()	= NULL;
	virtual	CDB *	Db()	= NULL;
	virtual	PCWSTR	UUID2String(IN UUID * p, PWSTR pValue, DWORD dwSize)	= NULL;
	virtual	bool	SendMessageToWebApp(Json::Value & req, Json::Value & res) = NULL;
	virtual	INotifyCenter *	NotifyCenter() = NULL;
	PCSTR			Name() {
		return m_name.c_str();
	}
	void			Create()
	{
		const char	*pIsExisting	= "select count(RegPUID) from registry where RegPUID=?";
		const char	*pInsert		= "insert into registry"	\
			"("\
			"RegPUID,RegUID,PUID,SubType,PID,"\
			"RegPath,RegValueName,DataSize,Cnt"\
			") "\
			"values("\
			"?,?,?,?,?"\
			",?,?,?,?"\
			")";
		const char	*pUpdate			= "update registry "	\
			"set DataSize = DataSize + ?, Cnt = Cnt + ?, UpdateTime = CURRENT_TIMESTAMP "
			"where RegPUID=? and SubType=?";

		if (Db()->IsOpened()) {
	
			if( NULL == (m_stmt.pIsExisting	= Db()->Stmt(pIsExisting)) )
				m_log.Log("%s", sqlite3_errmsg(Db()->Handle()));
			if( NULL == (m_stmt.pInsert	= Db()->Stmt(pInsert)) )
				m_log.Log("%s", sqlite3_errmsg(Db()->Handle()));
			if( NULL == (m_stmt.pUpdate	= Db()->Stmt(pUpdate)) )
				m_log.Log("%s", sqlite3_errmsg(Db()->Handle()));
		}
		else {
			ZeroMemory(&m_stmt, sizeof(m_stmt));
		}
		NotifyCenter()->RegisterNotifyCallback(__func__, NOTIFY_TYPE_AGENT, NOTIFY_EVENT_PERIODIC, this, PeriodicCallback);
	}
	void					Destroy()
	{
		if (Db()->IsOpened()) {
			if( m_stmt.pIsExisting)		Db()->Free(m_stmt.pIsExisting);
			if( m_stmt.pInsert)			Db()->Free(m_stmt.pInsert);
			ZeroMemory(&m_stmt, sizeof(m_stmt));
		}
	}
protected:
//#define USE_LOG
	void					Dump(CRegistry * p) {
		m_log.Log("  mode     :%d", p->mode);
		m_log.Log("  category :%d", p->category);
		m_log.Log("  wSize    :%d", p->wDataSize);
		m_log.Log("  wRevision:%d", p->wRevision);
		m_log.Log("  subType  :%d", p->subType);
		m_log.Log("  RegUID   :%p", p->RegUID);
		m_log.Log("  RegPUID  :%p", p->RegPUID);
		m_log.Log("  PID      :%d", p->PID);
		m_log.Log("  PUID     :%p", p->PUID);
		//m_log.Log("  RegPath  :%ws [%d]", p->RegPath.pBuf, p->RegPath.wSize);
		//m_log.Log("  RegValue :%ws [%d]", p->RegValueName.pBuf, p->RegValueName.wSize);
		m_log.Log("  count    :%d", p->nCount);
		m_log.Log("  Size     :%I64d", p->nRegDataSize);
	}
	static	bool			Proc2
	(
		PY_HEADER			pMessage,
		PVOID				pContext
	) 
	{
		PY_REGISTRY_MESSAGE	p	= (PY_REGISTRY_MESSAGE)pMessage;
		CRegistryCallback	*pClass = (CRegistryCallback *)pContext;
		SetStringOffset(p, &p->RegPath);
		SetStringOffset(p, &p->RegValueName);
		//	SetStringOffset(p);
		pClass->m_lock.Lock(p, [&](PVOID pContext) {
			auto	t	= pClass->m_table.find(p->RegPUID);
			if( pClass->m_table.end() == t ) {
				RegPtr	ptr	= std::make_shared<CRegistry>(dynamic_cast<PY_REGISTRY_DATA>(p));

				ptr->RegPathUID			= pClass->GetStrUID(StringRegistryKey, p->RegPath.pBuf);
				ptr->RegValueNameUID	= pClass->GetStrUID(StringRegistryValue, p->RegValueName.pBuf);
				pClass->m_table[ptr->RegPUID]	= ptr;			
				//pClass->Dump(ptr.get());
			}
			else {
				t->second->nCount		+= p->nCount;
				t->second->nRegDataSize	+= p->nRegDataSize;			
			}
			pClass->m_log.Log("%04d [%d] %ws", pClass->m_table.size(), p->subType, p->RegPath.pBuf);
		});
		return true;
	}
	static	bool			Proc(
		ULONGLONG			nMessageId,
		PVOID				pCallbackPtr,
		PYFILTER_DATA		p
	) 
	{
		return false;
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
	RegMap				m_table;

	bool	IsExisting(
		CRegistry	*p
	) {
		int			nCount	= 0;
		sqlite3_stmt* pStmt = m_stmt.pIsExisting;
		if (pStmt) {
			int		nIndex = 0;
			sqlite3_bind_int64(pStmt, ++nIndex, p->RegPUID);
			//sqlite3_bind_int(pStmt, ++nIndex, p->subType);
			if (SQLITE_ROW == sqlite3_step(pStmt))	{
				nCount	= sqlite3_column_int(pStmt, 0);
			}
			else {

			}
			sqlite3_reset(pStmt);
		}
		return nCount? true : false;
	}
	bool	Insert(
		CRegistry	*p
	) {
		bool			bRet	= false;
		sqlite3_stmt	*pStmt	= m_stmt.pInsert;
		if (pStmt) {
			int		nIndex = 0;

			std::wstring	strRegPath;
			std::wstring	strRegValueName;

			CStringTable::Lock(NULL, [&](PVOID pContext) {
				strRegPath		= GetString(p->RegPathUID);
				strRegValueName	= GetString(p->RegValueNameUID);			
			});
			sqlite3_bind_int64(pStmt,	++nIndex, p->RegPUID);
			sqlite3_bind_int64(pStmt,	++nIndex, p->RegUID);
			sqlite3_bind_int64(pStmt,	++nIndex, p->PUID);
			sqlite3_bind_int(pStmt,		++nIndex, p->subType);
			sqlite3_bind_int(pStmt,		++nIndex, p->PID);
			sqlite3_bind_text16(pStmt,	++nIndex, strRegPath.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text16(pStmt,	++nIndex, strRegValueName.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_int64(pStmt,	++nIndex, p->nRegDataSize);
			sqlite3_bind_int(pStmt,		++nIndex, p->nCount);
			if (SQLITE_DONE == sqlite3_step(pStmt))	bRet = true;
			else {
				m_log.Log("%-32s %s", __func__, sqlite3_errmsg(Db()->Handle()));
				Dump(p);
			}
			sqlite3_reset(pStmt);
		} 
		return bRet;
	}
	bool	Update(
		CRegistry	*p
	) {
		bool			bRet	= false;
		sqlite3_stmt	*pStmt	= m_stmt.pUpdate;
		if (pStmt) {
			int			nIndex = 0;
			sqlite3_bind_int64(pStmt, ++nIndex, p->nRegDataSize);
			sqlite3_bind_int(pStmt, ++nIndex, p->nCount);
			sqlite3_bind_int64(pStmt,	++nIndex, p->RegPUID);
			sqlite3_bind_int(pStmt, ++nIndex, p->subType);
			if (SQLITE_DONE == sqlite3_step(pStmt))	bRet = true;
			else {
				m_log.Log("%-32s %s", __func__, sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		}
		return bRet;
	}
	static	void	PeriodicCallback(
		WORD wType, WORD wEvent, PVOID pData, ULONG_PTR nDataSize, PVOID pContext
	) {
		CRegistryCallback	*pClass	= (CRegistryCallback *)pContext;
		static	DWORD	dwCount;
		if( dwCount++ % 10 )	return;

		//pClass->m_log.Log("%s %4d %4d", __FUNCTION__, wType, wEvent);

		//pClass->Db()->Begin(__func__);
		pClass->m_lock.Lock(NULL, [&](PVOID pContext) {
			try {
				UNREFERENCED_PARAMETER(pContext);		
				DWORD64		dwTick	= GetTickCount64();
				for( auto t = pClass->m_table.begin() ; t != pClass->m_table.end() ;  ) {
					if( (dwTick - t->second->dwLastTick ) > (60 * 1000) ) {
						if (pClass->IsExisting(t->second.get()))
							pClass->Update(t->second.get());
						else	{
							pClass->Insert(t->second.get());
						}
						auto	d	= t++;
						pClass->m_table.erase(d);
					}
					else
						t++;
				}
			}
			catch( std::exception & e) {
				pClass->m_log.Log("Proc2", e.what());
			}
		});
		//pClass->Db()->Commit(__func__);
	}
};
