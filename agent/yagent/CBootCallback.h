#pragma once

class CBootCallback
	:
	protected		IEventCallback,
	virtual	public	CAppLog
{
public:
	CBootCallback() {
		m_name		= EVENT_CALLBACK_NAME;
		m_dwBootId	= YAgent::GetBootId();
		m_bShutdown	= false;
		ZeroMemory(&m_stmt, sizeof(m_stmt));
	}
	virtual	~CBootCallback() {

	}
	virtual	DWORD		BootId() {
		return m_dwBootId;
	}
	virtual	CDB*		Db() = NULL;
	virtual	INotifyCenter *	NotifyCenter() = NULL;
	void	Create() {
		const char* pInsert = "insert into [boot]"	\
			"("\
			"[BootId],[UpTime]"\
			") "\
			"values("\
			"?,?"\
			")";
		const char* pIsExisting = 
			"select count(BootId) from boot where BootId=?";
		const char* pUpdate = 
			"update boot "	\
			"set LastTime=datetime('now','localtime') "\
			"where BootId=?";
		const char* pUpdate2 = 
			"update boot "	\
			"set DownTime=datetime('now','localtime') "\
			"where BootId=?";
		const char* pUpdate3 = 
			"update boot "	\
			"set DownTime=LastTime "\
			"where DownTime is null and BootId < '%d'";
		const char* pSelect = "select BootId,UpTime,LastTime,DownTime "\
			"from boot "\
			"where BootId=?";

		if (Db()->IsOpened()) {

			if (NULL == (m_stmt.pInsert = Db()->Stmt(pInsert)))
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			if (NULL == (m_stmt.pUpdate = Db()->Stmt(pUpdate)))
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			if (NULL == (m_stmt.pUpdate2 = Db()->Stmt(pUpdate2)))
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			if (NULL == (m_stmt.pIsExisting = Db()->Stmt(pIsExisting)))
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			if (NULL == (m_stmt.pSelect = Db()->Stmt(pSelect)))
				Log("%s", sqlite3_errmsg(Db()->Handle()));

			int		nAffected	= 0;

			Db()->Execute(&nAffected, true, true, pUpdate3, m_dwBootId);
		}
		else {
			ZeroMemory(&m_stmt, sizeof(m_stmt));
		}
		Log("%s BootId=%d", __FUNCTION__, m_dwBootId);
		if( Select() )	Update(false);
		else
		{
			Insert();
		}

		//	시스템 종료 이벤트 통지
		NotifyCenter()->RegisterNotifyCallback(__FUNCTION__ " PreShutdownCallback", 
			NOTIFY_TYPE_SYSTEM, NOTIFY_EVENT_PRESHUTDOWN, 
			this, ShutdownCallback);
		NotifyCenter()->RegisterNotifyCallback(__FUNCTION__ " ShutdownCallback", 
			NOTIFY_TYPE_SYSTEM, NOTIFY_EVENT_SHUTDOWN, 
			this, ShutdownCallback);
		NotifyCenter()->RegisterNotifyCallback(__FUNCTION__ " PeriodicCallback", 
			NOTIFY_TYPE_AGENT, NOTIFY_EVENT_PERIODIC, 
			this, PeriodicCallback);
	}
	static	void	ShutdownCallback(
		WORD wType, WORD wEvent, PVOID pData, ULONG_PTR nDataSize, PVOID pContext
	) {
		CBootCallback	*pClass	= (CBootCallback *)pContext;
		pClass->m_bShutdown	= true;
		pClass->Log("%s %4d %4d", __FUNCTION__, wType, wEvent);
		pClass->Update(true);
	}
	static	void	PeriodicCallback(
		WORD wType, WORD wEvent, PVOID pData, ULONG_PTR nDataSize, PVOID pContext
	) {
		CBootCallback	*pClass	= (CBootCallback *)pContext;
		//pClass->Log("%s %4d %4d", __FUNCTION__, wType, wEvent);
		pClass->Update(pClass->m_bShutdown);
	}
	void	Destroy() {
		Log("%s", __FUNCTION__);
		if (Db()->IsOpened()) {
			if (m_stmt.pInsert)		Db()->Free(m_stmt.pInsert);
			if (m_stmt.pUpdate)		Db()->Free(m_stmt.pUpdate);
			if (m_stmt.pUpdate2)	Db()->Free(m_stmt.pUpdate2);
			if (m_stmt.pIsExisting)	Db()->Free(m_stmt.pIsExisting);
			if (m_stmt.pSelect)		Db()->Free(m_stmt.pSelect);
			ZeroMemory(&m_stmt, sizeof(m_stmt));
		}
	}
	PCSTR	Name() {
		return m_name.c_str();
	}
	bool	Select() {
		bool			bRet	= false;
		sqlite3_stmt	*pStmt	= m_stmt.pSelect;
		if (pStmt) {
			int		nIndex = 0;
			//sqlite3_bind_int(pStmt, ++nIndex, pProcGuid, -1, SQLITE_STATIC);
			sqlite3_bind_int(pStmt, ++nIndex, m_dwBootId);
			if (SQLITE_ROW == sqlite3_step(pStmt)) {
				DWORD		dwBootId	= sqlite3_column_int(pStmt, 0);
				int64_t		nUpTime		= sqlite3_column_int64(pStmt, 1);
				int64_t		nLastTime	= sqlite3_column_int64(pStmt, 2);
				int64_t		nDownTime	= sqlite3_column_int64(pStmt, 3);
				bRet	= true;
			}
			sqlite3_reset(pStmt);
		}
		return bRet;
	}
	bool	Insert() {
		bool			bRet	= false;
		sqlite3_stmt	*pStmt	= m_stmt.pInsert;
		SYSTEMTIME		st;
		char			szDateTime[30]	= "";
		CTime::GetBootLocalTime(&st);
		CTime::SystemTimeToString(&st, szDateTime, sizeof(szDateTime));
		
		if (pStmt) {
			int		nIndex = 0;
			sqlite3_bind_int(pStmt, ++nIndex, m_dwBootId);
			sqlite3_bind_text(pStmt, ++nIndex, szDateTime,  -1, SQLITE_STATIC);
			if (SQLITE_DONE == sqlite3_step(pStmt)) {
				bRet	= true;
			}
			sqlite3_reset(pStmt);
		}
		return bRet;
	}
	bool	Update(bool bShutdown) {
		bool			bRet	= false;
		sqlite3_stmt	*pStmt	= bShutdown? m_stmt.pUpdate2 : m_stmt.pUpdate;
		DWORD64			dwTimestamp	= CTime::GetUnixTimestamp();
		//Log("%s bShutdown=%d", __FUNCTION__, m_bShutdown? true : false);
		if (pStmt) {
			int		nIndex = 0;
			sqlite3_bind_int(pStmt, ++nIndex, m_dwBootId);
			if (SQLITE_DONE == sqlite3_step(pStmt)) {
				bRet	= true;
			}
			sqlite3_reset(pStmt);
		}
		return bRet;
	}
private:
	std::string			m_name;
	DWORD				m_dwBootId;
	std::atomic<bool>	m_bShutdown;	//	윈도 셧다운 중임다.

	struct {
		sqlite3_stmt*	pInsert;
		sqlite3_stmt*	pUpdate;
		sqlite3_stmt*	pUpdate2;
		sqlite3_stmt*	pIsExisting;
		sqlite3_stmt*	pSelect;
	}	m_stmt;

};
