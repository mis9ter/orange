#pragma once
#include "tlhelp32.h"
#pragma comment(lib, "Rpcrt4.lib")

#define	GUID_STRLEN	37

typedef struct _MODULE {
	_MODULE() 
		:
		ImageBase(0),
		EntryPoint(0),
		ImageSize(0)
	{
		ZeroMemory(FullName, sizeof(FullName));
		ZeroMemory(BaseName, sizeof(BaseName));
	}
	PVOID			ImageBase;
	PVOID			EntryPoint;
	ULONG			ImageSize;
	WCHAR			FullName[AGENT_PATH_SIZE];
	WCHAR			BaseName[AGENT_PATH_SIZE];
} MODULE, *PMODULE;
class CModule
{
public:
	CModule(PMODULE p) 
	{
		ImageBase	= p->ImageBase;
		EntryPoint	= p->EntryPoint;
		ImageSize	= p->ImageSize;
		FullNameUID	= 0;
		BaseNameUID	= 0;
	}
	CModule(PMODULEENTRY32 p)
	{
		ImageBase	= p->modBaseAddr;
		EntryPoint	= NULL;
		ImageSize	= p->modBaseSize;
		FullNameUID	= 0;
		BaseNameUID	= 0;
	}
	~CModule() {

	}
	PVOID			ImageBase;
	PVOID			EntryPoint;
	STRUID			FullNameUID;
	STRUID			BaseNameUID;
	ULONG			ImageSize;
};

typedef std::function<bool (PVOID,PMODULEENTRY32)>	ModuleListCallback2;
typedef std::function<bool (ULONG, PVOID,PMODULE)>	ModuleListCallback;
typedef std::shared_ptr<CModule>					ModulePtr;
typedef std::map<PVOID, ModulePtr>					ModuleMap;

bool	GetModules(DWORD PID, PVOID pContext, ModuleListCallback pCallback);

int		GetSystemModules(PVOID pContext, ModuleListCallback pCallback);
int		GetProcessModules(HANDLE hProcess, PVOID pContext, ModuleListCallback pCallback);

class CModuleCallback
	:
	protected		IEventCallback,
	public virtual	CAppLog
{
public:
	CModuleCallback() 
		:
		m_log(L"module.log")
	{
		ZeroMemory(&m_stmt, sizeof(m_stmt));
		m_name = EVENT_CALLBACK_NAME;
	}
	~CModuleCallback() {

	}
	virtual	bool		KOpenProcess(DWORD PID, ACCESS_MASK desiredAccess, PHANDLE pProcessHandle)	= NULL;
	bool				GetModules2(DWORD PID, PVOID pContext, ModuleListCallback pCallback) {
		if( PID < 4 )
			return false;
		if (4 == PID) {
			GetSystemModules(pContext, pCallback);
		}
		else {
			HANDLE	hProcess	= NULL;
			if (KOpenProcess(PID, PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, &hProcess)) {
				GetProcessModules(hProcess, pContext, pCallback);
				CloseHandle(hProcess);
			}
			else {
				Log("%-32s KOpenProcess() succeeded. hProcess=%p", __func__, hProcess);
			}
		}
		return true;
	}
	virtual	PCWSTR		UUID2String(IN UUID* p, PWSTR pValue, DWORD dwSize) = NULL;
	virtual	bool		GetProcess(PROCUID PUID, PWSTR pValue, IN DWORD dwSize)	= NULL;
	PCSTR				Name() {
		return m_name.c_str();
	}
	void				Create()
	{
		const char* pInsert = "insert into module"	\
			"("\
			"ProcGuid,ProcUID, PID,FilePath,FileName,FileExt,RProcGuid,BaseAddress,FileSize"\
			",SignatureLevel,SignatureType) "\
			"values("\
			"?,?,?,?,?,?,?,?,?"\
			",?,?)";
		const char* pIsExisting = "select count(ProcUID) from module where ProcGuid=? and ProcUID=? and FilePath=?";
		const char* pUpdate = "update module "	\
			"set LoadCount=LoadCount+1, LastTime=datetime('now','localtime'), BaseAddress=? "\
			"where ProcGuid=? and ProcUID=? and FilePath=?";
		const char*	pSelect	= "select p.ProcGuid,p.ProcUID,p.ProcPath, m.FilePath "\
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
	void				Destroy()
	{
		if (Db()->IsOpened()) {
			if (m_stmt.pInsert)		Db()->Free(m_stmt.pInsert);
			if (m_stmt.pUpdate)		Db()->Free(m_stmt.pUpdate);
			if (m_stmt.pIsExisting)	Db()->Free(m_stmt.pIsExisting);
			if( m_stmt.pSelect)		Db()->Free(m_stmt.pSelect);
			ZeroMemory(&m_stmt, sizeof(m_stmt));
		}
	}
	bool				GetModule(PCWSTR pProcGuid, DWORD PID, ULONG_PTR pAddress, PWSTR pValue, DWORD dwSize)
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
	virtual	CDB *		Db()	= NULL;
	virtual	INotifyCenter *	NotifyCenter() = NULL;
	virtual	CDB*		Db(PCWSTR pName) = NULL;
	virtual	bool		AddModule(PROCUID PUID, PMODULE p)	= NULL;
protected:
	void	Message2Json(IN PY_MODULE_MESSAGE p, OUT Json::Value& doc) {

		try {
			doc["@name"]		= "module";
			//doc["@crc"] = GetStringCRC16("module");
			doc["Category"]		= p->category;
			doc["SubType"]		= p->subType;
			doc["DevicePath"]	= __utf8(p->DevicePath.pBuf);
			doc["PID"]			= (int)p->PID;
			doc["TID"]			= (int)p->TID;
			doc["CTID"]			= (int)p->CTID;
			doc["PPID"]			= (int)p->PPID;
			doc["RPID"]			= (int)p->RPID;
			doc["PUID"]			= (UID)p->PUID;
			doc["ImageBase"]	= (uint64_t)p->ImageBase;
			doc["ImageSize"]	= (uint64_t)p->ImageSize;
		}
		catch (std::exception& e) {
			doc["@exception"] = e.what();
		}
	}
	static	bool			Proc2
	(
		PY_HEADER			pMessage,
		PVOID				pContext
	) 
	{
		PY_MODULE_MESSAGE	p		= (PY_MODULE_MESSAGE)pMessage;
		CModuleCallback		*pClass = (CModuleCallback *)pContext;
		SetStringOffset(p, &p->DevicePath);

		MODULE	m;
		if( false == CAppPath::GetFilePath(p->DevicePath.pBuf, m.FullName, sizeof(m.FullName)) )
			StringCbCopy(m.FullName, sizeof(m.FullName), p->DevicePath.pBuf);

		m.ImageBase		= p->ImageBase;
		m.ImageSize		= p->ImageSize;
		//pClass->m_log.Log("%p %ws %d", p->ImageBase, m.FullName, (int)p->ImageSize);

		Json::Value		doc;
		pClass->Message2Json(p, doc);

		PCWSTR	pName	= _tcsrchr(m.FullName, L'\\');
		if( pName )	pName++;
		else 
			pName	= m.FullName;
		StringCbCopy(m.BaseName, sizeof(m.BaseName), pName);

		doc["FilePath"]	= __utf8(m.FullName);
		doc["FileName"]	= __utf8(m.BaseName);

		JsonUtil::Json2String(doc, [&](std::string & str) {
			pClass->m_log.Log("%ws\n%s", pName, str.c_str());
		});

		pClass->AddModule(p->PUID, &m);
		//pClass->m_log.Log("ImageBase: %p %d", m.ImageBase, (int)m.ImageSize);
		//	
		return true;
	}
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
			if( pClass->IsExisting(szProcGuid, p->PUID, szModulePath) )
				pClass->Update(szProcGuid, p->PUID, szModulePath, p), bInsert = false;
			else 
				pClass->Insert(szProcGuid, p->PUID, szModulePath, p), bInsert = true;
			pClass->m_log.Log("%-20s %6d %ws %s", "MODULE_LOAD", 
				p->PID, szProcPath, bInsert? "INSERT":"UPDATE");
			pClass->m_log.Log("  ProdGuid      %ws", szProcGuid);
			pClass->m_log.Log("  PUID          %p", p->PUID);
			pClass->m_log.Log("  PPUID         %p", p->PPUID);
			pClass->m_log.Log("  Path          %ws", szModulePath);
			pClass->m_log.Log("  address       %p", p->pBaseAddress);
			pClass->m_log.Log("  Size          %d", (DWORD)p->pImageSize);
		}
		else
		{
			pClass->Log("%s unknown", __FUNCTION__);
		}
		return true;
	}
private:
	std::string		m_name;
	CAppLog			m_log;
	struct {
		sqlite3_stmt* pInsert;
		sqlite3_stmt* pUpdate;
		sqlite3_stmt* pIsExisting;
		sqlite3_stmt* pSelect;

	}	m_stmt;
	bool	IsExisting(
		PCWSTR		pProcGuid,
		PROCUID		ProcUID,
		PCWSTR		pFilePath
	) {
		int			nCount = 0;
		sqlite3_stmt* pStmt = m_stmt.pIsExisting;
		if (pStmt) {
			int		nIndex = 0;
			sqlite3_bind_text16(pStmt, ++nIndex, pProcGuid, -1, SQLITE_STATIC);
			sqlite3_bind_int64(pStmt, ++nIndex, ProcUID);
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
		PROCUID				ProcUID,
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
			sqlite3_bind_int64(pStmt, ++nIndex, ProcUID);
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
		PROCUID				ProcUID,
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
			sqlite3_bind_int64(pStmt, ++nIndex, ProcUID);
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

