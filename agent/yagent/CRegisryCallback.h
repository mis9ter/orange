#pragma once
/*
	

	\REGISTRY\A is a hidden registry hive for use by Windows Store apps (aka Metro-style apps).
	https://superuser.com/questions/689788/what-does-the-path-registry-a-in-sysinternals-procmon-log-mean

*/
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
	PCSTR			Name() {
		return m_name.c_str();
	}
	void			Create()
	{
		const char	*pIsExisting	= "select count(RegUID) from registry where RegUID=? and SubType=? and RegValueName=?";
		const char	*pInsert		= "insert into registry"	\
			"("\
			"RegUID,SubType,RegPath,RegValueName,DataSize,Cnt"\
			") "\
			"values("\
			"?,?,?,?,?,1"\
			")";
		const char	*pUpdate			= "update registry "	\
			"set DataSize = DataSize + ?, Cnt = Cnt + 1, UpdateTime = CURRENT_TIMESTAMP "
			"where RegUID=? and SubType=? and RegValueName=?";

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
	static	bool			Proc2
	(
		PY_HEADER			pMessage,
		PVOID				pContext
	) 
	{
		PY_REGISTRY			p	= (PY_REGISTRY)pMessage;
		CRegistryCallback	*pClass = (CRegistryCallback *)pContext;
		
		//pClass->m_log.Log("  mode     :%d", p->mode);
		//pClass->m_log.Log("  category :%d", p->category);
		//pClass->m_log.Log("  wSize    :%d", p->wSize);
		//pClass->m_log.Log("  wRevision:%d", p->wRevision);
		//pClass->m_log.Log("  subType  :%d", p->subType);
		//pClass->m_log.Log("  RegUID   :%p", p->RegUID);

		p->RegPath.pBuf	= (PWSTR)((char *)p + p->RegPath.wOffset);						
		p->RegValueName.pBuf	= (PWSTR)((char *)p + p->RegValueName.wOffset);

		//pClass->m_log.Log("  RegPath  :%ws [%d]", p->RegPath.pBuf, p->RegPath.wSize);
		//pClass->m_log.Log("  RegValue :%ws [%d]", p->RegValueName.pBuf, p->RegValueName.wSize);
		//pClass->m_log.Log("  Size     :%d", p->nDataSize);	

		if( pClass->IsExisting(p) )
			pClass->Update(p);
		else
			if( false == pClass->Insert(p) )
				pClass->Update(p);

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

	bool	IsExisting(
		PY_REGISTRY			p
	) {
		int			nCount	= 0;
		sqlite3_stmt* pStmt = m_stmt.pIsExisting;
		if (pStmt) {
			int		nIndex = 0;
			sqlite3_bind_int64(pStmt, ++nIndex, p->RegUID);
			sqlite3_bind_int(pStmt, ++nIndex, p->subType);
			sqlite3_bind_text16(pStmt, ++nIndex, p->RegValueName.pBuf, -1, SQLITE_STATIC);
			
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

			sqlite3_bind_int64(pStmt,	++nIndex, p->RegUID);
			sqlite3_bind_int(pStmt,		++nIndex, p->subType);
			sqlite3_bind_text16(pStmt,	++nIndex, p->RegPath.pBuf, -1, SQLITE_STATIC);
			sqlite3_bind_text16(pStmt,	++nIndex, p->RegValueName.pBuf, -1, SQLITE_STATIC);
			sqlite3_bind_int64(pStmt, ++nIndex, p->nDataSize);
			if (SQLITE_DONE == sqlite3_step(pStmt))	bRet = true;
			else {
				m_log.Log("%-32s %s", __func__, sqlite3_errmsg(Db()->Handle()));
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
			sqlite3_bind_int64(pStmt,	++nIndex, p->RegUID);
			sqlite3_bind_int(pStmt,		++nIndex, p->subType);
			sqlite3_bind_text16(pStmt,	++nIndex, p->RegValueName.pBuf, -1, SQLITE_STATIC);

			if (SQLITE_DONE == sqlite3_step(pStmt))	bRet = true;
			else {
				m_log.Log("%-32s %s", __func__, sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		}
		return bRet;
	}
};
