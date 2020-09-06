#pragma once

#pragma comment(lib, "Rpcrt4.lib")

#define	GUID_STRLEN	37

class CModuleCallback
	:
	public virtual	CAppLog
{
public:
	CModuleCallback() {
		ZeroMemory(&m_stmt, sizeof(m_stmt));

	}
	~CModuleCallback() {

	}
	virtual	CDB* Db() = NULL;
	virtual	PCWSTR		UUID2String(IN UUID* p, PWSTR pValue, DWORD dwSize) = NULL;
	virtual	bool		GetProcess(PCWSTR pProcGuid, PWSTR pValue, IN DWORD dwSize)	= NULL;

	void					Create()
	{
		const char* pInsert = "insert into module"	\
			"("\
			"ProcGuid,PID,FilePath,FileName,FileExt,RProcGuid,BaseAddress,FileSize"\
			",SignatureLevel,SignatureType) "\
			"values("\
			"?,?,?,?,?,?,?,?"\
			",?,?)";
		const char* pIsExisting = "select count(ProcGuid) from module where ProcGuid=? and FilePath=?";
		const char* pUpdate = "update module "	\
			"set LoadCount=LoadCount+1, LastTime=CURRENT_TIMESTAMP, BaseAddress=? "\
			"where ProcGuid=? and FilePath=?";
		const char*	pSelect	= "select p.ProcGuid,p.ProcPath, m.FilePath "\
			"from process p, module m "\
			"where p.ProcGuid=m.ProcGuid and m.ProcGuid=? and ? between m.BaseAddress and m.BaseAddress+m.FileSize";
		if (Db()->IsOpened()) {

			if (NULL == (m_stmt.pInsert = Db()->Stmt(pInsert)))
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			if (NULL == (m_stmt.pUpdate = Db()->Stmt(pUpdate)))
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			if (NULL == (m_stmt.pIsExisting = Db()->Stmt(pIsExisting)))
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			if (NULL == (m_stmt.pSelect = Db()->Stmt(pSelect)))
				Log("%s", sqlite3_errmsg(Db()->Handle()));
		}
		else {
			ZeroMemory(&m_stmt, sizeof(m_stmt));
		}
	}
	void					Destroy()
	{
		if (Db()->IsOpened()) {
			if (m_stmt.pInsert)		Db()->Free(m_stmt.pInsert);
			if (m_stmt.pUpdate)		Db()->Free(m_stmt.pUpdate);
			if (m_stmt.pIsExisting)	Db()->Free(m_stmt.pIsExisting);
			if( m_stmt.pSelect)		Db()->Free(m_stmt.pSelect);
			ZeroMemory(&m_stmt, sizeof(m_stmt));
		}
	}
	bool	GetModule(PCWSTR pProcGuid, DWORD PID, ULONG_PTR pAddress, PWSTR pValue, DWORD dwSize)
	{
		bool	bRet = false;
		sqlite3_stmt* pStmt = m_stmt.pSelect;
		if (pStmt) {
			int		nIndex = 0;
			sqlite3_bind_text16(pStmt, ++nIndex, pProcGuid, -1, SQLITE_STATIC);
			sqlite3_bind_int64(pStmt, ++nIndex, pAddress);
			if (SQLITE_ROW == sqlite3_step(pStmt)) {
				PCWSTR	_ProcGuid = NULL;
				PCWSTR	_ProcPath	= NULL;
				PCWSTR	_FilePath = NULL;
				_ProcGuid = (PCWSTR)sqlite3_column_text16(pStmt, 0);
				_ProcPath = (PCWSTR)sqlite3_column_text16(pStmt, 1);
				_FilePath = (PCWSTR)sqlite3_column_text16(pStmt, 2);
				StringCbCopy(pValue, dwSize, _FilePath? _FilePath: L"");
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
		WCHAR	szProcPath[AGENT_PATH_SIZE] = L"";
		WCHAR	szModulePath[AGENT_PATH_SIZE] = L"";
		PWSTR	pFileName = NULL;
		char	szTime[40] = "";

		bool	bInsert;

		CModuleCallback* pClass = (CModuleCallback*)pCallbackPtr;

		pClass->UUID2String(&p->ProcGuid, szProcGuid, sizeof(szProcGuid));
		if (YFilter::Message::SubType::ModuleLoad == p->subType) {
			pClass->UUID2String(&p->PProcGuid, szPProcGuid, sizeof(szPProcGuid));
			if (false == CAppPath::GetFilePath(p->szPath, szProcPath, sizeof(szProcPath)))
				StringCbCopy(szProcPath, sizeof(szProcPath), p->szPath);
			if (false == CAppPath::GetFilePath(p->szCommand, szModulePath, sizeof(szModulePath)))
				StringCbCopy(szModulePath, sizeof(szModulePath), p->szCommand);
			if( pClass->IsExisting(szProcGuid, szModulePath) )
				pClass->Update(szProcGuid, szModulePath, p), bInsert = false;
			else 
				pClass->Insert(szProcGuid, szModulePath, p), bInsert = true;
			//pClass->Log("%-20s %6d %ws %s", "MODULE_LOAD", 
			//	p->dwProcessId, szProcPath, bInsert? "INSERT":"UPDATE");
			//pClass->Log("  ProdGuid      %ws", szProcGuid);
			//pClass->Log("  Path          %ws", szModulePath);
			//pClass->Log("  address       %p", p->pBaseAddress);
			//pClass->Log("  Size          %d", (DWORD)p->pImageSize);
		}
		else
		{
			pClass->Log("%s unknown", __FUNCTION__);
		}
		return true;
	}
private:
	struct {
		sqlite3_stmt* pInsert;
		sqlite3_stmt* pUpdate;
		sqlite3_stmt* pIsExisting;
		sqlite3_stmt* pSelect;

	}	m_stmt;
	bool	IsExisting(
		PCWSTR		pProcGuid,
		PCWSTR		pFilePath
	) {
		int			nCount = 0;
		sqlite3_stmt* pStmt = m_stmt.pIsExisting;
		if (pStmt) {
			int		nIndex = 0;
			sqlite3_bind_text16(pStmt, ++nIndex, pProcGuid, -1, SQLITE_STATIC);
			sqlite3_bind_text16(pStmt, ++nIndex, pFilePath, -1, SQLITE_STATIC);
			if (SQLITE_ROW == sqlite3_step(pStmt)) {
				nCount = sqlite3_column_int(pStmt, 0);
			}
			sqlite3_reset(pStmt);
		}
		return nCount ? true : false;
	}
	bool	Update(
		PCWSTR				pProcGuid,
		PCWSTR				pFilePath,
		PYFILTER_DATA		pData
	) {
		if (NULL == pFilePath)			return false;

		bool			bRet = false;
		sqlite3_stmt* pStmt = m_stmt.pUpdate;
		if (pStmt) {
			int		nIndex = 0;
			sqlite3_bind_int64(pStmt, ++nIndex, pData->pBaseAddress);
			sqlite3_bind_text16(pStmt, ++nIndex, pProcGuid, -1, SQLITE_STATIC);
			sqlite3_bind_text16(pStmt, ++nIndex, pFilePath, -1, SQLITE_STATIC);
			if (SQLITE_DONE == sqlite3_step(pStmt))	bRet = true;
			else {
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		}
		else {
			Log("%s ---- m_stmt.pInsert is null.", __FUNCTION__);
		}
		return bRet;
	}
	bool	Insert(
		PCWSTR				pProcGuid,
		PCWSTR				pFilePath,
		PYFILTER_DATA		pData
	) {
		if (NULL == pFilePath)			return false;

		bool			bRet = false;
		sqlite3_stmt* pStmt = m_stmt.pInsert;
		if (pStmt) {
			int		nIndex = 0;
			PCWSTR	pFileName	= wcsrchr(pFilePath, '\\');
			PCWSTR	pFileExt	= pFileName? wcsrchr(pFileName, L'.') : NULL;
			sqlite3_bind_text16(pStmt, ++nIndex, pProcGuid, -1, SQLITE_STATIC);
			sqlite3_bind_int(pStmt, ++nIndex, pData->PID);
			sqlite3_bind_text16(pStmt, ++nIndex, pFilePath, -1, SQLITE_STATIC);
			if( pFileName )
				sqlite3_bind_text16(pStmt, ++nIndex, pFileName+1, -1, SQLITE_STATIC);
			else sqlite3_bind_null(pStmt, ++nIndex);
			if( pFileExt )
				sqlite3_bind_text16(pStmt, ++nIndex, pFileExt+1, -1, SQLITE_STATIC);
			else 
				sqlite3_bind_null(pStmt, ++nIndex);
			sqlite3_bind_null(pStmt, ++nIndex);
			sqlite3_bind_int64(pStmt, ++nIndex, pData->pBaseAddress);
			sqlite3_bind_int64(pStmt, ++nIndex, pData->pImageSize);
			sqlite3_bind_int(pStmt, ++nIndex, pData->ImageProperties.Properties.ImageSignatureLevel);
			sqlite3_bind_int(pStmt, ++nIndex, pData->ImageProperties.Properties.ImageSignatureType);
			if (SQLITE_DONE == sqlite3_step(pStmt))	bRet = true;
			else {
				Log("%s", sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		}
		else {
			Log("%s ---- m_stmt.pInsert is null.", __FUNCTION__);
		}
		return bRet;
	}
};

