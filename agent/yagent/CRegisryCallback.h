#pragma once
/*
	

	\REGISTRY\A is a hidden registry hive for use by Windows Store apps (aka Metro-style apps).
	https://superuser.com/questions/689788/what-does-the-path-registry-a-in-sysinternals-procmon-log-mean

*/

class CRegistry
{
public:
	CRegistry(PY_REGISTRY _p) 
		:	
		p(_p),
		dwLastTick(GetTickCount64())	
	{
		
	}
	~CRegistry() {
	
	}
	PY_REGISTRY	p;
	DWORD64		dwLastTick;
};


typedef std::shared_ptr<CRegistry>	RegPtr;
typedef std::map<REGUID, RegPtr>	RegMap;

class CRegistryCallback
	:
	protected		IEventCallback,
	public virtual	CAppLog
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
	void					Dump(PY_REGISTRY p) {
		m_log.Log("  mode     :%d", p->mode);
		m_log.Log("  category :%d", p->category);
		m_log.Log("  wSize    :%d", p->wSize);
		m_log.Log("  wRevision:%d", p->wRevision);
		m_log.Log("  subType  :%d", p->subType);
		m_log.Log("  RegUID   :%p", p->RegUID);
		m_log.Log("  RegPUID  :%p", p->RegPUID);
		m_log.Log("  PID      :%d", p->PID);
		m_log.Log("  PUID     :%p", p->PUID);
		m_log.Log("  RegPath  :%ws [%d]", p->RegPath.pBuf, p->RegPath.wSize);
		m_log.Log("  RegValue :%ws [%d]", p->RegValueName.pBuf, p->RegValueName.wSize);
		m_log.Log("  count    :%d", p->nCount);
		m_log.Log("  Size     :%I64d", p->nDataSize);
	}
	static	bool			Proc2
	(
		PY_HEADER			pMessage,
		PVOID				pContext
	) 
	{
		PY_REGISTRY			p	= (PY_REGISTRY)pMessage;
		CRegistryCallback	*pClass = (CRegistryCallback *)pContext;
		SetStringOffset(p, &p->RegPath);
		SetStringOffset(p, &p->RegValueName);
		//	SetStringOffset(p);
		pClass->m_lock.Lock(p, [&](PVOID pContext) {
			auto	t	= pClass->m_table.find(p->RegPUID);
			if( pClass->m_table.end() == t ) {
				RegPtr	ptr	= std::make_shared<CRegistry>((PY_REGISTRY)(new char[p->wSize]));
				CopyMemory(ptr->p, pContext, p->wSize);
				SetStringOffset(ptr.get(), &ptr->p->RegPath);
				SetStringOffset(ptr.get(), &ptr->p->RegValueName);
				pClass->m_table[ptr->p->RegPUID]	= ptr;			
				//pClass->Dump(ptr.get());
			}
			else {
				t->second->p->nCount		+= p->nCount;
				t->second->p->nDataSize		+= p->nDataSize;			
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
		PY_REGISTRY			p
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
		PY_REGISTRY			p
	) {
		bool			bRet	= false;
		sqlite3_stmt	*pStmt	= m_stmt.pInsert;
		if (pStmt) {
			int		nIndex = 0;

			sqlite3_bind_int64(pStmt,	++nIndex, p->RegPUID);
			sqlite3_bind_int64(pStmt,	++nIndex, p->RegUID);
			sqlite3_bind_int64(pStmt,	++nIndex, p->PUID);
			sqlite3_bind_int(pStmt,		++nIndex, p->subType);
			sqlite3_bind_int(pStmt,		++nIndex, p->PID);
			sqlite3_bind_text16(pStmt,	++nIndex, p->RegPath.pBuf, -1, SQLITE_STATIC);
			sqlite3_bind_text16(pStmt,	++nIndex, p->RegValueName.pBuf, -1, SQLITE_STATIC);
			sqlite3_bind_int64(pStmt, ++nIndex, p->nDataSize);
			sqlite3_bind_int(pStmt, ++nIndex, p->nCount);
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
		PY_REGISTRY			p
	) {
		bool			bRet	= false;
		sqlite3_stmt	*pStmt	= m_stmt.pUpdate;
		if (pStmt) {
			int			nIndex = 0;
			sqlite3_bind_int64(pStmt, ++nIndex, p->nDataSize);
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
						if (pClass->IsExisting(t->second->p))
							pClass->Update(t->second->p);
						else	{
							pClass->Insert(t->second->p);
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
