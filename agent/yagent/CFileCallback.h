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
		m_log.Log(NULL);
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
	virtual		STRUID	GetStrUID(IN StringType type, IN PCWSTR pStr)	= NULL;

	PCSTR				Name() {
		return m_name.c_str();
	}
protected:
	void		Create()
	{
		const char* pInsert = "insert into file"	\
			"("\
			"FPUID,FUID,PUID,PathUID,Count,CreateCount,DeleteCount"\
			",Directory,ReadCount,ReadSize,WriteCount,WriteSize"\
			") values("\
			"?,?,?,?,1,?,?"\
			",?,?,?,?,?"\
			")";
		const char* pIsExisting = "select count(FPUID) from file where FPUID=?";
		const char* pUpdate = "update file "	\
			"set Count=Count+1,CreateCount=CreateCount+?, DeleteCount=DeleteCount+?,ReadCount=ReadCount+?,ReadSize=ReadSize+?, "\
			"WriteCount=WriteCount+?,WriteSize=WriteSize+?,"	\
			"LastTime=CURRENT_TIMESTAMP	"	\
			"where FPUID=?";
		if (Db()->IsOpened()) {

			if (NULL == (m_stmt.pInsert = Db()->Stmt(pInsert)))
				m_log.Log("%-32s %s", __func__, sqlite3_errmsg(Db()->Handle()));
			if (NULL == (m_stmt.pUpdate = Db()->Stmt(pUpdate)))
				m_log.Log("%-32s %s", __func__, sqlite3_errmsg(Db()->Handle()));
			if (NULL == (m_stmt.pIsExisting = Db()->Stmt(pIsExisting)))
				m_log.Log("%-32s %s", __func__, sqlite3_errmsg(Db()->Handle()));
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
		PY_FILE_MESSAGE	p		= (PY_FILE_MESSAGE)pMessage;
		CFileCallback	*pClass = (CFileCallback *)pContext;
		SetStringOffset(p, &p->Path);

		pClass->m_lock.Lock(p, [&](PVOID pContext) {
			WCHAR	szPath[AGENT_PATH_SIZE]	= L"";
			if( false == CAppPath::GetFilePath(p->Path.pBuf, szPath, sizeof(szPath)) )
				StringCbCopy(szPath, sizeof(szPath), p->Path.pBuf);
			PCWSTR	pName	= wcsrchr(szPath, L'\\');
			std::wstring	proc;
			if( pClass->GetProcess(p->PUID, pClass, [&](PVOID pContext, CProcess *p) {
				pClass->GetString(p->ProcPathUID, proc);
			})) {
				pName	= _tcsrchr(proc.c_str(), L'\\');
				if( p->bCreate ) 
				{
					pClass->m_log.Log("%-32ws %d %ws", pName, p->bCreate, szPath);
					if( p->read.nCount )
						pClass->m_log.Log("  read :%d %d", p->read.nCount, (int)p->read.nBytes);
					if( p->write.nCount )
						pClass->m_log.Log("  write:%d %d", p->write.nCount, (int)p->write.nBytes);
				}
			} 
			else {
				pClass->m_log.Log("%p %d NOT FOUND", p->PUID, p->PID);			
			}

			if( pClass->IsExisting(p) )
				pClass->Update(p);
			else
				pClass->Insert(szPath, p);
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
		PY_FILE_MESSAGE	p
	) {
		int			nCount = 0;
		sqlite3_stmt* pStmt = m_stmt.pIsExisting;
		if (pStmt) {
			int		nIndex = 0;
			sqlite3_bind_int64(pStmt, ++nIndex, p->FPUID);
			if (SQLITE_ROW == sqlite3_step(pStmt)) {
				nCount = sqlite3_column_int(pStmt, 0);
			}
			else {
				m_log.Log("%-32s %s", __func__, sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		}
		return nCount ? true : false;
	}
	bool	Insert(
		PCWSTR			pFilePath,
		PY_FILE_MESSAGE	p
	) {
		bool			bRet = false;
		sqlite3_stmt* pStmt = m_stmt.pInsert;

		const char* pInsert = "insert into file"	\
			"("\
			"FPUID,FUID,PUID,PathUID,Create"\
			",Directory,ReadCount,ReadSize,WriteCount,WriteSize"\
			") values("\
			"?,?,?,?,?"\
			",?,?,?,?,?"\
			")";
		if (pStmt) {
			int		nIndex = 0;
			UID		PathUID	= GetStrUID(StringFilePath, pFilePath);

			sqlite3_bind_int64(pStmt, ++nIndex, p->FPUID);
			sqlite3_bind_int64(pStmt, ++nIndex, p->FUID);
			sqlite3_bind_int64(pStmt, ++nIndex, p->PUID);
			sqlite3_bind_int64(pStmt, ++nIndex, PathUID);
			sqlite3_bind_int(pStmt, ++nIndex, p->bCreate);
			sqlite3_bind_int(pStmt, ++nIndex, 0);
			sqlite3_bind_int(pStmt, ++nIndex, p->bIsDirectory);
			sqlite3_bind_int(pStmt, ++nIndex, p->read.nCount);
			sqlite3_bind_int64(pStmt, ++nIndex, p->read.nBytes);
			sqlite3_bind_int(pStmt, ++nIndex, p->write.nCount);
			sqlite3_bind_int64(pStmt, ++nIndex, p->write.nBytes);
			
			if (SQLITE_DONE == sqlite3_step(pStmt))	bRet = true;
			else {
				m_log.Log("%-32s %s", __func__, sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		}
		else {
			m_log.Log("%-32s %s", __func__, "pStmt is null");
		}
		return bRet;
	}
	bool	Update(
		PY_FILE_MESSAGE	p
	) {
		const char* pUpdate = "update file "	\
			"set Create=Create+?, Delete=Delete+?,ReadCount=ReadCount+?,ReadSize=ReadSize+?, "\
			"WriteCount=WriteCount+?,WriteSize=WriteSize+?,"	\
			"LastTime=CURRENT_TIMESTAMP	"	\
			"where FPUID=?";

		bool			bRet = false;
		sqlite3_stmt* pStmt = m_stmt.pUpdate;
		if (pStmt) {
			int		nIndex = 0;
			sqlite3_bind_int(pStmt,		++nIndex, p->bCreate);
			sqlite3_bind_int(pStmt,		++nIndex, 0);
			sqlite3_bind_int(pStmt,		++nIndex, p->read.nCount);
			sqlite3_bind_int64(pStmt,	++nIndex, p->read.nBytes);
			sqlite3_bind_int(pStmt,		++nIndex, p->write.nCount);
			sqlite3_bind_int64(pStmt,	++nIndex, p->write.nBytes);
			sqlite3_bind_int64(pStmt,	++nIndex, p->FPUID);
			if (SQLITE_DONE == sqlite3_step(pStmt))	bRet = true;
			else {
				m_log.Log("%s", sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		}
		return bRet;
	}
};
