#pragma once

#include <process.h>
#include <atomic>
#include "CDB.h"
#include "yagent.string.h"

#define IDLE_COMMIT		1
#define IDLE_COUNT		300
#define IDLE_TICKS		60000

class CAppPath
{
public:
	static	CPathConvertor* Convertor() {
		static	CPathConvertor	c;
		return &c;
	}
	static	bool	GetFilePath(IN PCWSTR pLinkPath, OUT PWSTR pValue, IN DWORD dwSize)
	{
		bool	bRet = false;
		WCHAR	szPath[AGENT_PATH_SIZE];
		PCWSTR	pSystemRoot = L"\\SystemRoot";
		PCWSTR	pWinDir = L"%windir%";
		int		nSystemRoot = (int)wcslen(pSystemRoot);
		WCHAR	szWinDir[200] = L"";

		if( L'\\' != pLinkPath[0] )	return false;
		if (0 == _wcsnicmp(pSystemRoot, pLinkPath, nSystemRoot)) {
			ExpandEnvironmentStrings(pWinDir, szWinDir, _countof(szWinDir));
			StringCbPrintf(pValue, dwSize, L"%s%s", szWinDir, pLinkPath + nSystemRoot);
			return true;
		}
		HANDLE	hFile = CreateFile(pLinkPath,
			0,
			0,
			0,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS,
			0);
		if (INVALID_HANDLE_VALUE == hFile)
		{
			CString	path	= Convertor()->MakeSymbolicPath(pLinkPath);
			StringCbCopy(pValue, dwSize, path);
			bRet	= true;
		}
		else {
			auto rcode = GetFinalPathNameByHandle(hFile, szPath, _countof(szPath),
				FILE_NAME_NORMALIZED);
			if (rcode)
			{
				if (szPath[0] == L'\\' && szPath[1] == L'\\' &&
					szPath[2] == L'?' && szPath[3] == L'\\')
					StringCbCopy(pValue, dwSize, szPath + 4);
				else
					StringCbCopy(pValue, dwSize, szPath);
				bRet = true;
			}
			CloseHandle(hFile);
		}
		return bRet;
	}
private:

};

#define EVENT_CALLBACK_NAME			__FILE__
#define EVENT_CALLBACK_NAME_SIZE	128

interface   IEventCallback
{
	virtual	void	Create()	= NULL;
	virtual	void	Destroy()	= NULL;
	virtual	PCSTR	Name()		= NULL;
};
#include "CProcessCallback.h"
#include "CThreadCallback.h"
#include "CModuleCallback.h"
#include "CPreProcess.h"
#include "CBootCallback.h"
#include "CRegisryCallback.h"

#define		EVENT_DB_NAME	L"event.db"

typedef	bool(*PEventCallbackProc)
(
	ULONGLONG			nMessageId,
	PVOID				pCallbackPtr,
	PYFILTER_DATA		p
);

typedef struct EventCallbackItem {
	EventCallbackItem(
		WORD				_wMode,
		WORD				_wCategory,
		PEventCallbackProc	_pCallback,
		PVOID				_pCallbackPtr
	) {
		wMode			= _wMode;
		wCategory		= _wCategory;
		pCallback		= _pCallback;
		pCallbackPtr	= _pCallbackPtr;
	}
	union {
		DWORD		dwId;
		struct {
			WORD	wMode;
			WORD	wCategory;
		};
	};
	PEventCallbackProc	pCallback;
	PVOID				pCallbackPtr;
} EventCallbackItem;

#include <map>
typedef std::shared_ptr<EventCallbackItem>		EventCallbackItemPtr;
typedef std::map<DWORD, EventCallbackItemPtr>	EventCallbackTable;
typedef std::map<DWORD, UUID>					ProcMap;

typedef std::map<std::string, IEventCallback*>	EventCallbackMap;

class CEventCallback
	:
	virtual	public	CAppLog,
	public	CProcessCallback,
	public	CThreadCallback,
	public	CModuleCallback,
	public	CPreProcess,
	public	CBootCallback,
	public	CRegistryCallback
{

public:
	CEventCallback() 
		:
		m_pDB(NULL),
		m_log(L"event.log")
	{
		m_log.Log(__FUNCTION__);

		m_hWait			= CreateEvent(NULL, TRUE, FALSE, NULL);
		m_dwBootId		= YAgent::GetBootId();
		
		m_counter.dwProcess	= 0;
		m_counter.dwModule	= 0;
		m_counter.dwThread	= 0;

		RegisterEventCallback(dynamic_cast<IEventCallback *>(dynamic_cast<CProcessCallback*>(this)));
		RegisterEventCallback(dynamic_cast<IEventCallback*>(dynamic_cast<CModuleCallback*>(this)));
		RegisterEventCallback(dynamic_cast<IEventCallback*>(dynamic_cast<CThreadCallback*>(this)));
		RegisterEventCallback(dynamic_cast<IEventCallback*>(dynamic_cast<CBootCallback*>(this)));	
		RegisterEventCallback(dynamic_cast<IEventCallback*>(dynamic_cast<CRegistryCallback*>(this)));	
		/*
		AddCallback(
			YFilter::Message::Mode::Event, 
			YFilter::Message::Category::Process,
			ProcessCallbackProc,
			dynamic_cast<CProcessCallback *>(this)		
		);
		AddCallback(
			YFilter::Message::Mode::Event,
			YFilter::Message::Category::Thread,
			ThreadCallbackProc,
			dynamic_cast<CThreadCallback *>(this)
		);
		*/
	}
	~CEventCallback() {
		CloseHandle(m_hWait);
		m_log.Log(__FUNCTION__);
	}
	void	RegisterEventCallback(IN IEventCallback* pCallback) {
		Log("%s %s", __FUNCTION__, pCallback->Name());
		m_events[pCallback->Name()] = pCallback;
	}
	virtual	BootUID	GetBootUID() {
		return CBootCallback::GetBootUID();	
	}
	virtual	CDB*	Db(PCWSTR pName) = NULL;
	CDB*			Db() {
		return m_pDB;
	}
	virtual	void	SetResult(
		IN	PCSTR	pFile, PCSTR pFunction, int nLine,
		IN	Json::Value & res, IN bool bRet, IN int nCode, IN PCSTR pMsg /*utf8*/
	)	= NULL;
	virtual	INotifyCenter *	NotifyCenter() = NULL;
	bool	GetModule(PCWSTR pProcGuid, DWORD PID, ULONG_PTR pAddress,
				PWSTR pValue, DWORD dwSize) {
		return CModuleCallback::GetModule(pProcGuid, PID, pAddress, pValue, dwSize);
	}
	bool	GetProcess(PROCUID PUID, PWSTR pValue, IN DWORD dwSize) {
		return CProcessCallback::GetProcess(PUID, pValue, dwSize);
	}
	static	void	SystemCallback(
		WORD wType, WORD wEvent, PVOID pData, ULONG_PTR nDataSize, PVOID pContext
	) {
		CEventCallback	*pClass	= (CEventCallback *)pContext;
		pClass->Log("%s %4d %4d", __FUNCTION__, wType, wEvent);
	}
	static	void	SessionCallback(
		WORD wType, WORD wEvent, PVOID pData, ULONG_PTR nDataSize, PVOID pContext
	) {
		CEventCallback	*pClass	= (CEventCallback *)pContext;
		pClass->Log("%s %4d %4d", __FUNCTION__, wType, wEvent);
	}
	bool		CreateCallback()
	{	
		Log(__FUNCTION__);

		CDB		*pDB	= Db(DB_EVENT_NAME);
		if( NULL == pDB ) {
		
			return false;
		}
		m_pDB	= pDB;
		if( pDB->IsOpened() ) {
#if 1 == IDLE_COMMIT
			pDB->Begin(__FUNCTION__);
#endif
			for (auto t : m_events) {
				t.second->Create();
			}
			//	이전에 실행되어 동작중인 프로세스에 대한 정보들 수집
			//	이 정보는 APP단에서 얻으려니 이모 저모 잘 안된다.
			//	커널단에서 받아야 겠다.
			//	커널 드라이버를 시작하면 이 정보부터 올라온다.

			ProcMap			table;
			/*
			CPreProcess::Check2([&](
				UUID	*pProcGuid, 
				DWORD	PID, 
				DWORD	PPID, 
				PCWSTR	pPath,
				PCWSTR	pCmdLine
			) {
				Log("%6d %ws", PID, pPath);
				table[PID]	= *pProcGuid;
				WCHAR	szProcGuid[40] = L"";
				UUID2String(pProcGuid, szProcGuid, sizeof(szProcGuid));
				Log("ProcGuid  : %ws", szProcGuid);
				Log("CmdLine   : %ws", pCmdLine);
				Log("PPID      : %6d", PPID);
				if( PPID ) {
					auto t = table.find(PPID);
					if( table.end() == t ) {
						Log("PProcGuid : NOT FOUND");
					}
					else {
						UUID2String(&t->second, szProcGuid, sizeof(szProcGuid));
						Log("PProcGuid : %ws", szProcGuid);
					}
				}
				else {
					Log("PProcGuid : NULL");
				}
				Log("-------------------------------------------");
			});
			*/
			NotifyCenter()->RegisterNotifyCallback("SessionCallback", NOTIFY_TYPE_SESSION, 
				NOTIFY_EVENT_SESSION, this, SessionCallback);
			return true;
		}
		else {
			Log("%s IsOpened() = %d", __FUNCTION__, pDB->IsOpened());
		}
		return false;
	}
	void		DestroyCallback() {
		Log("%s", __FUNCTION__);
		for (auto t : m_events) {
			Log("%s %s", __FUNCTION__, t.second->Name());
			//t.second->Destroy();
		}
		CDB		*pDB	= Db(DB_EVENT_NAME);
		if( NULL == pDB ) {
			return;
		}
#if 1 == IDLE_COMMIT
		if(pDB->IsOpened())	pDB->Commit(__FUNCTION__);
#endif
	}
	PCWSTR		UUID2String(IN UUID* p, PWSTR pValue, DWORD dwSize) {
		StringCbPrintf(pValue, dwSize,
			L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			p->Data1, p->Data2, p->Data3,
			p->Data4[0], p->Data4[1], p->Data4[2], p->Data4[3],
			p->Data4[4], p->Data4[5], p->Data4[6], p->Data4[7]);
		return pValue;
	}

	DWORD	GetBootId()
	{
		return m_dwBootId;
	}
	DWORD		QueryUnknonwn
	(
		const	Json::Value		&query,
		const	Json::Value		&bind,
		Json::Value		&res

	) {
		DWORD			dwCount		= 0;
		sqlite3_stmt	*pStmt		= NULL;
		Json::Value		&resdata	= res["data"];
		Json::Value		&rescolumn	= resdata["column"];
		Json::Value		&resrow		= resdata["row"];
		Json::Value		&resmore	= resdata["more"];

		resmore	= false;

		CDB		*pDB	= Db(DB_EVENT_NAME);
		if( NULL == pDB ) {
			return 0;
		}

		try {
			pStmt	= pDB->Stmt(query.asCString());
			if( pStmt ) {
				int				nIndex = 0;
				int				nColumnCount	= sqlite3_column_count(pStmt);
				const wchar_t	*pColumnName	= NULL;

				for( auto i = 0 ; i < nColumnCount ; i++ ) {
					pColumnName	= (wchar_t *)sqlite3_column_name16(pStmt, i);			
					rescolumn.append(__utf8(pColumnName));
				}
				for( auto & t : bind ) {
					int			nType	= t["type"].asInt();
					const	Json::Value	&value	= t["value"];

					switch( nType ) {
						case 0:						//	int
						case SQLITE_INTEGER:		//	int64
							if( t.isMember("name") ) {
								if( t["name"].isString() && !t["name"].asString().compare("[LastBootId]")) {
									sqlite3_bind_int64(pStmt, ++nIndex, YAgent::GetBootId());
								}
							}
							else {
								sqlite3_bind_int64(pStmt, ++nIndex, value.asInt64());
							}
							break;

						case SQLITE_FLOAT:
							sqlite3_bind_double(pStmt, ++nIndex, value.asDouble());
							break;

						case SQLITE_TEXT:			//	string
							sqlite3_bind_text16(pStmt, ++nIndex, __utf16(value.asCString()), -1, SQLITE_TRANSIENT);
							break;					

						default:
							Log("%-32s unknown type:%d in bind", __func__, nType);
							break;
					}					
				}		

				int		nStatus;
				while ( SQLITE_ROW == (nStatus = sqlite3_step(pStmt)) ) {
					Json::Value		row;
					if( dwCount > 1000 )	{
						resmore	= true;
						break;
					}
					row.clear();
					for( auto i = 0 ; i < nColumnCount ; i++ ) {
						int			nType	= sqlite3_column_type(pStmt, i);
						wchar_t		*pText	= NULL;
						double		dDouble	= 0;
						PVOID		*pBlob	= NULL;
						int64_t		nInt	= 0;

						std::string	name	= __utf8((PCWSTR)sqlite3_column_name16(pStmt, i));
						switch( nType ) {
							case SQLITE_INTEGER:
								row[name]	= sqlite3_column_int64(pStmt, i);
								break;

							case SQLITE_FLOAT:
								row[name]	= sqlite3_column_double(pStmt, i);
								break;

							case SQLITE_TEXT:
								row[name]	= __utf8((wchar_t *)sqlite3_column_text16(pStmt, i));
								break;

							case SQLITE_BLOB:
								row[name]	= "blob";
								break;

							case SQLITE_NULL:
								row[name]	= Json::Value::null;
								break;

							default:
								row[name]	= "unknown";
								break;
						}
					}
					resrow.append(row);
					dwCount++;
				}
				pDB->Free(pStmt);
				SetResult(__FILE__, __func__, __LINE__, res, true, 0, "");
			}
			else {
				//	invalid stmt
				SetResult(__FILE__, __func__, __LINE__, res, false, 
					sqlite3_errcode(pDB->Handle()), sqlite3_errmsg(pDB->Handle()) );
			}
		}
		catch( std::exception & e) {
			SetResult(__FILE__, __func__, __LINE__, res, false, 0, e.what());
			SQLITE_OK;
		}
		resdata["count"]	= (int)dwCount;
		return dwCount;
	}
	struct {
		std::atomic<DWORD>		dwProcess;
		std::atomic<DWORD>		dwThread;
		std::atomic<DWORD>		dwModule;
		std::atomic<DWORD>		dwRegistry;

	} m_counter;
protected:
	void	Wait(IN DWORD dwMilliSeconds)
	{
		WaitForSingleObject(m_hWait, dwMilliSeconds);
	}
	bool	SendMessageToWebApp(Json::Value & req, Json::Value & res) 
	{
		CIPC	client;
		HANDLE	hClient;

		if( INVALID_HANDLE_VALUE != (hClient = client.Connect(AGENT_WEBAPP_PIPE_NAME, __func__)) ) {

			Log("%-32s connected", __FUNCTION__);
			IPCHeader   header  = {IPCJson, };

			std::string					str;
			Json::StreamWriterBuilder	builder;
			builder["indentation"]	= "";
			str	= Json::writeString(builder, req);
			header.dwSize   = (DWORD)(str.length() + 1);

			DWORD   dwBytes = 0;

			if( client.Request(__FUNCTION__, hClient, (PVOID)str.c_str(), (DWORD)str.length()+1,
				[&](PVOID pResponseData, DWORD dwResponseSize) {
				Log("%-32s response %d bytes", __FUNCTION__, dwResponseSize);
				Log("%s", pResponseData);

				Json::CharReaderBuilder	rbuilder;
				const std::unique_ptr<Json::CharReader>		reader(rbuilder.newCharReader());

				std::string		errors;
				try {
					if( reader->parse((const char *)pResponseData, (const char *)pResponseData + dwResponseSize, 
						&res, &errors) ) {

					}
				}
				catch( std::exception & e) {
					res["exception"]["function"]	= __func__;
					res["exception"]["file"]		= __FUNCTION__;
					res["exception"]["line"]		= __LINE__;
					res["exception"]["msg"]			= e.what();
				}

			}) ) {



			}
			client.Disconnect(hClient, __FUNCTION__);
		}
		else {
			//Log("%-32s WebApp is not connected", __func__);
			return false;
		}
		return true;
	}

	/*
	void	AddCallback(
		YFilter::Message::Mode		mode,
		YFilter::Message::Category	category,
		PEventCallbackProc			pCallback,
		PVOID						pCallbackPtr
	) {

		EventCallbackItemPtr	ptr	= std::make_shared<EventCallbackItem>(
										mode, category, pCallback, pCallbackPtr);
		CallbackTable()[ptr->dwId]	= ptr;
	}
	EventCallbackItemPtr	GetCallback(
		YFilter::Message::Mode		mode,
		YFilter::Message::Category	category
	) {
		EventCallbackItemPtr	ptr	= std::make_shared<EventCallbackItem>(
									mode, category, NULL, NULL);

		auto	t	= CallbackTable().find(ptr->dwId);
		if (CallbackTable().end() != t) {
			ptr	= t->second;
		}
		return ptr;
	}	
	*/
	static	bool			EventCallbackProc(
		ULONGLONG			nMessageId,
		PVOID				pCallbackPtr,
		int					nCategory,
		int					nSubType,
		PVOID				pData,
		size_t				nSize
	) {
		bool							bRet	= false;
		CEventCallback					*pClass	= (CEventCallback *)pCallbackPtr;		
		static	std::atomic<DWORD>		dwCount	= 0;
		static	std::atomic<DWORD64>	dwTicks	= 0;

		dwCount++;
		if( YFilter::Message::Category::Process == nCategory ) {
			pClass->m_counter.dwProcess++;
			PYFILTER_DATA	p = (PYFILTER_DATA)pData;
			bRet	= CProcessCallback::Proc(nMessageId, dynamic_cast<CProcessCallback *>(pClass), p);
		}
		else if (YFilter::Message::Category::Thread == nCategory) {
			pClass->m_counter.dwThread++;
			PYFILTER_DATA	p = (PYFILTER_DATA)pData;
			bRet	= CThreadCallback::Proc(nMessageId, dynamic_cast<CThreadCallback *>(pClass), p);
		}
		else if(YFilter::Message::Category::Module == nCategory) {
			pClass->m_counter.dwModule++;
			PYFILTER_DATA	p = (PYFILTER_DATA)pData;
			bRet	= CModuleCallback::Proc(nMessageId, dynamic_cast<CModuleCallback*>(pClass), p);
		}
		else {
			PYFILTER_DATA	p = (PYFILTER_DATA)pData;
			//pClass->Log("%d %ws", nCategory, p->szPath);
		
		}
		#if 1 == IDLE_COMMIT
		DWORD	dwGap	= (DWORD)(GetTickCount64() - dwTicks);
		if (dwCount >= IDLE_COUNT || (dwCount && dwGap >= IDLE_TICKS) ) {
			pClass->CommitAndBegin(dwGap);
			dwCount	= 0;
			dwTicks	= GetTickCount64();
		}
		#endif
		return bRet;
	}
	static	bool			EventCallbackProc2(
		PY_HEADER			pMessage,
		PVOID				pContext
	) {
		bool							bRet	= false;
		CEventCallback					*pClass	= (CEventCallback *)pContext;		
		static	std::atomic<DWORD>		dwCount	= 0;
		static	std::atomic<DWORD64>	dwTicks	= 0;

		//pClass->Log("%-32s %p", __func__, YAgent::GetBootUID());

		dwCount++;
		switch( pMessage->category )
		{
			case YFilter::Message::Category::Registry:
				pClass->m_counter.dwRegistry++;
				bRet	= CRegistryCallback::Proc2(pMessage, dynamic_cast<CRegistryCallback *>(pClass));
				break;

			case YFilter::Message::Category::Process:
				pClass->m_counter.dwProcess++;
				bRet	= CProcessCallback::Proc2(pMessage, dynamic_cast<CProcessCallback *>(pClass));
				break;

			default:
				pClass->m_log.Log("unknown category:%d", pMessage->category);
				break;
		}
#if 1 == IDLE_COMMIT
		DWORD	dwGap	= (DWORD)(GetTickCount64() - dwTicks);
		if (dwCount >= IDLE_COUNT || (dwCount && dwGap >= IDLE_TICKS) ) {
			pClass->CommitAndBegin(dwGap);
			dwCount	= 0;
			dwTicks	= GetTickCount64();
		}
#endif
		return bRet;
	}
	EventCallbackTable & CallbackTable() {
		return m_table;
	}
private:
	CAppLog					m_log;
	DWORD					m_dwBootId;
	EventCallbackTable		m_table;
	CLock					m_lock;
	HANDLE					m_hWait;
	EventCallbackMap		m_events;
	CDB						*m_pDB;

	void					CommitAndBegin(DWORD dwTicks)
	{
		int		nCount	= 0;
		static	std::atomic<DWORD>	dwCount;
	//	__function_lock(m_lock.Get());

		CDB		*pDB	= Db(DB_EVENT_NAME);
		if( NULL == pDB ) {
			return;
		}
		nCount	= pDB->Commit(__FUNCTION__);
		pDB->Begin(__FUNCTION__);

		//if( 0 == dwCount++ % 10 )
		//	SetProcessWorkingSetSize(GetCurrentProcess(), 128 * 1024, 1024 * 1024);
	}
};
