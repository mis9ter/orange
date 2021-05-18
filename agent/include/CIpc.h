#ifndef __CIPC_H__
#define __CIPC_H__

#include "yagent.string.h"

#pragma warning(disable:4995)

#define __NOT_USE_XCOMMON

#include <windows.h>
#include <objbase.h>
#include <Sddl.h>
#include <AclAPI.h>
#include <process.h>
#include <strsafe.h>
#include "IIPc.h"

#define THREAD_CATEGORY_COUNT	2
#define REQUEST_THREAD_COUNT_MAX	30
#define IPC_DATA_SIZE			(1024 * 1024 * 4)

typedef struct 
{
	PVOID			pThis;
	HANDLE			hThread;
	HANDLE			hIocp;
	HANDLE			hInitCompleteEvent;
} ThreadInfo;
typedef enum 
{
	IPCThreadTypeService, 
	IPCThreadTypeRequest 
} IPCThreadType;

typedef HANDLE	(WINAPI*	XPFN_CREATEFILEW)(
	_In_ LPCWSTR lpFileName,
	_In_ DWORD dwDesiredAccess,
	_In_ DWORD dwShareMode,
	_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	_In_ DWORD dwCreationDisposition,
	_In_ DWORD dwFlagsAndAttributes,
	_In_opt_ HANDLE hTemplateFile
);
#define __lock		/##/
#define __unlock	/##/
#define	xErrorLog

class CTryFinally
{
public:
	CTryFinally(
		std::function<void()> tryCallback, 
		std::function<void()> finallyCallback) 
	{
		if( tryCallback )	tryCallback();
		m_finallyCallback	= finallyCallback;
	}
	~CTryFinally() {
		if( m_finallyCallback )	m_finallyCallback();
	}
private:
	std::function<void(void)>	m_finallyCallback;
};

//static	CAppLog	g_log(L"ipc.log");


class CRequestThread
{
public:
	CRequestThread(IN IIPcClient * p) :
		hThread(NULL),
		hPipe(INVALID_HANDLE_VALUE) ,
		pContext(NULL),
		hShutdown(NULL),
		pClient(p)
	{
		//g_log.Log(__FUNCTION__);
	}
	~CRequestThread() {
		//g_log.Log(__FUNCTION__);
		if( INVALID_HANDLE_VALUE != hPipe )	{
			pClient->Disconnect(hPipe, __FUNCTION__);
			hPipe	= INVALID_HANDLE_VALUE;			
		}
		if( NULL != hThread ) {
			WaitForSingleObject(hThread, INFINITE);
			CloseHandle(hThread);
		}
	}
	HANDLE		hThread;
	HANDLE		hShutdown;
	HANDLE		hPipe;
	PVOID		pContext;
	IIPcClient *pClient;
private:

};

#include <list>
typedef std::shared_ptr<CRequestThread>		RequestThreadPtr;
typedef std::list<RequestThreadPtr>			RequestThreadList;

class CIPc : 
	public	IIPc
{
public:
	CIPc() 
		:
		m_log(L"ipc.log")
	{
		m_szServerName[0]			= NULL;
		::ZeroMemory(m_threads, sizeof(m_threads));

		m_pCallback			= NULL;
		m_pCallbackPtr		= NULL;

		m_hShutdown			= ::CreateEvent(NULL, TRUE, FALSE, NULL);
		m_bShutdown			= FALSE;

		m_bLowIntegrityMode = FALSE;
		m_bStarted			= FALSE;

		m_dwRequestThreadCount = 1;

		::InitializeCriticalSection(&m_lock);
		m_pCreateFileW = (XPFN_CREATEFILEW)GetProcAddress(GetModuleHandle(L"kernelbase.dll"), "CreateFileW");
		if (NULL == m_pCreateFileW)
		{
			m_pCreateFileW = ::CreateFileW;
		}
	}
	virtual ~CIPc() {
		::CloseHandle(m_hShutdown);
		m_hShutdown = NULL;
		::DeleteCriticalSection(&m_lock);
	}
	//////////////////////////////////////////////////////////////////////////////
	// The Virtual functions of XModule
	//////////////////////////////////////////////////////////////////////////////
	long			__retainCount;
	VOID			DeleteInstance(IIPc* pInstance)
	{
		pInstance->Release();
	}
	void			Release() 
	{ 
		if( 0 == ::InterlockedDecrement(&__retainCount) )
		{
			delete this; 
		}
	} 
	void			Retain() 
	{ 
		::InterlockedIncrement(&__retainCount);	
	}
	///////////////////////////////////////////////////////////////////////////
	// The Virtual functions of IIpc
	///////////////////////////////////////////////////////////////////////////
	IIPcServer *	GetServer() {
		return dynamic_cast<IIPcServer *>(this);
	}
	IIPcClient *	GetClient() {
		return dynamic_cast<IIPcClient *>(this);
	}
	void			Free(IN VOID *ptr = NULL) {
		if( ptr )
		{
			delete ptr;
		} else {
			Shutdown();

			delete this;
		}	
	}
	///////////////////////////////////////////////////////////////////////////
	// The Virtual Functions of IIpcServer
	///////////////////////////////////////////////////////////////////////////
	bool			Start(
		IN LPCTSTR	pName, 
		BOOL		bLowIntegrityMode = FALSE, 
		LONG		RequestThreadCount = 1) 
	{
		unsigned	uId;
		HANDLE		hEventArray[REQUEST_THREAD_COUNT_MAX+1] = {0,};
		DWORD		dwResult = 0;
		bool		bReturnValue = FALSE;
		HANDLE		hIocp = NULL;

		m_log.Log("%-32s %ws", __FUNCTION__, pName);

		if (RequestThreadCount > REQUEST_THREAD_COUNT_MAX)
		{
			RequestThreadCount = REQUEST_THREAD_COUNT_MAX;
		}

		m_dwRequestThreadCount	= RequestThreadCount;
		m_bLowIntegrityMode		= bLowIntegrityMode;

		::ResetEvent(m_hShutdown);
		::StringCbCopy(m_szServerName, sizeof(m_szServerName), pName);

		m_threads[0].pThis		= this;
		m_threads[0].hIocp		= ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		m_threads[0].hInitCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_threads[0].hThread	= (HANDLE)_beginthreadex(NULL, 0, ServiceThread, this, 0, &uId);
		hEventArray[0] = m_threads[0].hInitCompleteEvent;

		return true;
	}
	void			Shutdown() {

		m_log.Log("%-32s", __FUNCTION__);
		unsigned	i;
		HANDLE		hThreads[REQUEST_THREAD_COUNT_MAX+1] = {0,};

		if( WAIT_OBJECT_0 == WaitForSingleObject(m_hShutdown, 0) ) {
			//	이미 종료 이벤트가 셋된 경우라면 시작된 것이 아니다.
			return;
		}
		::SetEvent(m_hShutdown);
		m_bShutdown = TRUE;

		//	아래 부분: 왜 종료할 때 Connect를 호출하는거지?
		//	현재 pipe 연결을 기다리는 ServiceThread의 loop를 돌게 해주기 위해서
		HANDLE	hUser	= Connect(m_szServerName, __FUNCTION__);
		if( INVALID_HANDLE_VALUE != hUser )
		{		
			Disconnect(hUser, __FUNCTION__);
		}
		CallThread(IPCThreadTypeService, NULL, 0);

		hThreads[0] = m_threads[0].hThread;

		for( i = 0 ; i < m_dwRequestThreadCount ; i++ )
		{
			CallThread(IPCThreadTypeRequest, NULL, 0);
			hThreads[i+1]	= m_threads[i+1].hThread;
		}

		::WaitForMultipleObjects(m_dwRequestThreadCount+1, hThreads, TRUE, 3000);

		for( i = 0 ; i < m_dwRequestThreadCount+1 ; i++ )
		{
			if (i < 2)
			{
				if (m_threads[i].hIocp)
				{
					::CloseHandle(m_threads[i].hIocp);
				}
			}

			m_threads[i].hIocp = NULL;

			if (m_threads[i].hInitCompleteEvent)
			{
				::CloseHandle(m_threads[i].hInitCompleteEvent);
				m_threads[i].hInitCompleteEvent = NULL;
			}

			if( m_threads[i].hThread )
			{
				::CloseHandle(m_threads[i].hThread);
				m_threads[i].hThread = NULL;
			}
		}
	}
	void			SetServiceCallback(IN IPCServiceCallback pCallback, IN LPVOID pCallbackPtr) {
		m_pCallback		= pCallback;
		m_pCallbackPtr	= pCallbackPtr;
	}
	///////////////////////////////////////////////////////////////////////////
	// The Virtual Functions of IIpcClient
	///////////////////////////////////////////////////////////////////////////
	HANDLE			Connect(IN LPCTSTR pName, IN LPCSTR pCause) {
		HANDLE	hPipe = INVALID_HANDLE_VALUE;
		__lock;
		for (DWORD dwIndex = 0; dwIndex < 5; dwIndex++)
		{
			//hPipe	= ::CreateFile(pName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
			hPipe = m_pCreateFileW(pName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
			if( INVALID_HANDLE_VALUE != hPipe )
			{
				m_log.Log("%-32s connected", __FUNCTION__);
				break;
			}
			CErrorMessage	err(GetLastError());
			m_log.Log("%-32s error=%s(%d)", __FUNCTION__, (PCSTR)err, (DWORD)err);
			if( (DWORD)err != ERROR_PIPE_BUSY )
			{
				Sleep(100);
				continue;
			}
			if( !::WaitNamedPipe(pName, 3000) )
			{
				m_log.Log("%-32s error=%s(%d)", __FUNCTION__, (PCSTR)err, (DWORD)err);
				break;
			}			
		}
		__unlock;
		return hPipe;
	}
	void			Disconnect(HANDLE hIpc, LPCSTR pCause) {
		__lock;
		if( INVALID_HANDLE_VALUE != hIpc )
		{
			m_log.Log("%-32s disconnect @%s", __FUNCTION__, pCause);
			::FlushFileBuffers(hIpc);
			m_log.Log("%-32s FlushFileBuffers @%s", __FUNCTION__, pCause);
			::DisconnectNamedPipe(hIpc);
			m_log.Log("%-32s DisconnectNamedPipe @%s", __FUNCTION__, pCause);
			::CloseHandle(hIpc);
			m_log.Log("%-32s CloseHandle @%s", __FUNCTION__, pCause);
		}
		__unlock;
	}
	DWORD			Write(
		IN PCSTR	pCause,
		IN HANDLE	hIpc,
		IN void		*pData,
		IN DWORD	dwDataSize
	) {
		DWORD		dwBytes	= 0;
		if( hIpc == INVALID_HANDLE_VALUE )
		{
			return false;
		}
		__lock;
		if( ::WriteFile(hIpc, pData, dwDataSize, &dwBytes, NULL) )
		{
			m_log.Log("%-32s %-32s write %d bytes", __FUNCTION__, pCause, dwBytes);
		}
		else
		{
			CErrorMessage	err(GetLastError());
			m_log.Log("%-32s %-32s (%d) %s", __FUNCTION__, pCause, (DWORD)err, (PCSTR)err);
		}
		__unlock;
		return dwBytes;
	}
	DWORD			Read(
		IN PCSTR	pCause,
		IN HANDLE	hIpc,
		OUT void	*pData,
		OUT DWORD	dwDataSize
					) {
		DWORD	dwBytes	= 0,
				dwReadBytes = 0;
		if( hIpc == INVALID_HANDLE_VALUE )
		{
			//_log(_T("__CIPc_Read_INVALID_HANDLE_VALUE"));
			return false;
		} 
		__lock;
		if( ::ReadFile(hIpc, pData, dwDataSize, &dwBytes, NULL) )
		{
			m_log.Log("%-32s %-32s read %p %d bytes", __FUNCTION__, pCause, hIpc, dwBytes);
		}
		else
		{
			CErrorMessage	err(GetLastError());
			m_log.Log("%-32s %-32s %p (%d)%s", __FUNCTION__, pCause, hIpc, (DWORD)err, (PCSTR)err);
		}
		__unlock;
		return dwBytes;
	}
	bool			Request(
		IN PCSTR	pCause,
		IN	HANDLE	hIpc,
		IN	PVOID	pRequest,
		IN	DWORD	dwRequestSize,
		IN	std::function<void(PVOID pResponseData, DWORD dwResponseSize)> pCallback 	
	) {
		IPCHeader	header;

		header.type		= IPCJson;
		header.dwSize	= dwRequestSize;
		if( Write(pCause, hIpc, &header, sizeof(header)) ) 
		{
			if( Write(pCause, hIpc, pRequest, dwRequestSize)) 
			{
				if( Read(pCause, hIpc, &header, sizeof(header)) ) 
				{
					if( header.dwSize ) {
						CBuffer	data(header.dwSize);
						if( data.Data() ) {
							if( Read(pCause, hIpc, data.Data(), data.Size()) ) {
								if( pCallback )	pCallback(data.Data(), data.Size());
								return true;
							}					
						}
					}
					else {
						if( pCallback )	pCallback(NULL, 0);
						return true;
					}
				}				
			
			}
	
		}
		return false;
	}
	///////////////////////////////////////////////////////////////////////////
private:
	CAppLog				m_log;
	XPFN_CREATEFILEW	m_pCreateFileW;
	TCHAR				m_szServerName[MAX_PATH];
	ThreadInfo			m_threads[REQUEST_THREAD_COUNT_MAX+1];
	IPCServiceCallback	m_pCallback;
	void				*m_pCallbackPtr;
	//CLock				m_lock;
	CRITICAL_SECTION	m_lock;
	HANDLE				m_hShutdown;
	BOOL				m_bShutdown;
	 

	void			CallThread(
						IN IPCThreadType	type,
						IN LPVOID			object,
						IN DWORD			dwValue
					) {
		if (m_threads[type].hIocp)
		{
			::PostQueuedCompletionStatus(m_threads[type].hIocp, dwValue, (ULONG_PTR)object, NULL);
		}
	}
	static	BOOL		IsVistaOrLater()
	{
		OSVERSIONINFOEX osvi;
		DWORDLONG dwlConditionMask = 0;
		int op = VER_GREATER_EQUAL;

		// Initialize the OSVERSIONINFOEX structure.

		ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		osvi.dwMajorVersion = 6;
		osvi.dwMinorVersion = 0;
		osvi.wServicePackMajor = 0;
		osvi.wServicePackMinor = 0;

		// Initialize the condition mask.

		VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, op);
		VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, op);
		VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, op);
		VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMINOR, op);

		// Perform the test.

		return VerifyVersionInfo(
			&osvi,
			VER_MAJORVERSION | VER_MINORVERSION |
			VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
			dwlConditionMask);

	}
	static	BOOL		BuildSidFromSddl(
		PSECURITY_ATTRIBUTES* ppSA, 
		PACL* ppAcl, 
		const WCHAR* pszSddl	= L"D:(A;;GA;;;WD)S:(ML;;NWNRNX;;;LW)"
	)
	{
		DWORD cbAcl = 256;
		PACL pAcl = (ACL*)LocalAlloc(LPTR, cbAcl);
		BOOL	bResult = FALSE;
		PSECURITY_DESCRIPTOR pSd = NULL;
		PSECURITY_ATTRIBUTES pSa = NULL;

		if (pAcl == NULL)
			return 0;

		do 
		{
			if (InitializeAcl(pAcl, cbAcl, ACL_REVISION) == FALSE)
			{
				break;
			}

			if (ConvertStringSecurityDescriptorToSecurityDescriptorW( pszSddl, SDDL_REVISION_1, &pSd, NULL) == FALSE)
			{
				break;
			}

			pSa = (PSECURITY_ATTRIBUTES)LocalAlloc(LPTR, sizeof(SECURITY_ATTRIBUTES));
			pSa->nLength = sizeof(SECURITY_ATTRIBUTES);
			pSa->lpSecurityDescriptor = pSd;
			pSa->bInheritHandle = FALSE;

			*ppSA = pSa;
			*ppAcl = pAcl;

			bResult = TRUE;

		} while (FALSE);

		if (bResult == FALSE)
		{
			if (pAcl)
			{
				LocalFree(pAcl);
			}

			if (pSd)
			{
				LocalFree(pSd);
			}

			if (pSa)
			{
				LocalFree(pSa);
			}
		}

		return bResult;
	}


	static	BOOL		FreeAnnonymousSid(PSECURITY_ATTRIBUTES pSA, PACL pAcl)
	{
		if (pAcl)
		{
			LocalFree(pAcl);		
		}

		if (pSA)
		{
			if (pSA->lpSecurityDescriptor)
			{
				LocalFree(pSA->lpSecurityDescriptor);
			}

			LocalFree(pSA);
		}

		return TRUE;
	}
	static unsigned	__stdcall	ServiceThread(IN LPVOID p) {
		CIPc		*pClass	= (CIPc *)p;
		BOOL		bInitialized = FALSE;

		PACL		pAcl = NULL;
		PSECURITY_ATTRIBUTES	pSa = NULL;
		BOOL		bResult = FALSE;

		CTryFinally		condes(
			[&]() {
				pClass->m_log.Log(__FUNCTION__);
				::CoInitializeEx(NULL, COINIT_MULTITHREADED);
				if (pClass->m_bLowIntegrityMode != FALSE && IsVistaOrLater())
				{
					bResult = BuildSidFromSddl(&pSa, &pAcl);
					if (bResult == FALSE)
					{
						CErrorMessage	err(GetLastError());
						pClass->m_log.Log("%-32s error:%d %s", __FUNCTION__, (DWORD)err, (PCSTR)err);
						return;
					}
					else {
						//xTrace(L"Create NamedPipe as LowIntegrityMode");
					}
				}
			},
			[&]() {
				//	이 함수 종료시 자동으로 실행된다.
				if (pAcl)
				{
					pClass->m_log.Log(__FUNCTION__);
					FreeAnnonymousSid(pSa, pAcl);
					pAcl = NULL;
					
				}
				::CoUninitialize();
			}	
		);		

		RequestThreadList	table;
		while( WAIT_TIMEOUT == WaitForSingleObject(pClass->m_hShutdown, 0) ) {
			RequestThreadPtr	ptr;	

			try {
				ptr	= std::shared_ptr<CRequestThread>(new CRequestThread(dynamic_cast<IIPcClient *>(pClass)));
			}
			catch( std::exception & e ) {
				pClass->m_log.Log("%-32s exception:%s", __FUNCTION__, e.what());
				break;
			}
			ptr->hPipe	= ::CreateNamedPipe(pClass->m_szServerName, 
				PIPE_ACCESS_DUPLEX, 
				PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT,
				PIPE_UNLIMITED_INSTANCES,
				IPC_DATA_SIZE,
				IPC_DATA_SIZE,
				0,
				pSa);
			if( INVALID_HANDLE_VALUE == ptr->hPipe )	{
				CErrorMessage	err(GetLastError());
				pClass->m_log.Log("%-32s %s(%d)", __FUNCTION__, (PCSTR)err, (DWORD)err);
				break;			
			}
			if( ConnectNamedPipe(ptr->hPipe, NULL) || ERROR_PIPE_CONNECTED == ::GetLastError() )
			{
				if( WAIT_OBJECT_0 == WaitForSingleObject(pClass->m_hShutdown, 0) ) {
					//	종료 상태입니다.
				}
				else {
					//	소수의 RequestThread를 상주하는것 보다
					//	필요시 RequestThread를 생성해서 처리하는 방식으로 변경한다.	
					ptr->pContext	= pClass;
					ptr->hShutdown	= pClass->m_hShutdown;
					ptr->hThread	= (HANDLE)::_beginthreadex(NULL, 0, RequestThread2, ptr.get(), 0, 0);
					if( ptr->hThread ) {
						table.push_back(ptr);
					}
				}
			}
			else
			{
				CErrorMessage	err(GetLastError());
				pClass->m_log.Log("%-32s ConnectNamedPipe() failed. error=%s(%d)", 
					(PCSTR)err, (DWORD)err);
			}
		}
		table.clear();
		return 0;
	}
	bool	CreateRequestThread(HANDLE h) {
	
	
	}
	static unsigned	__stdcall	RequestThread2(IN LPVOID ptr) 
	{
		//	pipe 연결이 맺어지면 이 스레드가 1개 생긴다.
		CRequestThread	*p		= (CRequestThread *)ptr;
		CIPc			*pClass	= (CIPc*)p->pContext;

		pClass->m_log.Log("%-32s %p connected", __FUNCTION__, p->hPipe);
		if( pClass->m_pCallback )
		{
			pClass->m_pCallback(p->hPipe, pClass->GetClient(), 
				pClass->m_pCallbackPtr, pClass->m_hShutdown);
		}
		pClass->m_log.Log("%-32s %p disconnected", __FUNCTION__, p->hPipe);
		return 0;
	}
	/*
	static unsigned	__stdcall	RequestThread(IN LPVOID p) {
		BOOL			bSuccess;
		ULONG_PTR		pCompletionKey;	
		LPOVERLAPPED	pContext;
		DWORD			dwValue;
		ThreadInfo*		pInfo = (ThreadInfo*)p;
		CIPc			*pClass		= (CIPc*)pInfo->pThis;

		//	xErrorLog(L"IPC_REQUEST_THREAD");

		SetEvent(pInfo->hInitCompleteEvent);

		while( true )
		{
			bSuccess	= ::GetQueuedCompletionStatus(
				pClass->m_threads[1].hIocp, 
				&dwValue,
				&pCompletionKey, 
				(LPOVERLAPPED *)&pContext, 
				INFINITE
			);

			pClass->m_log.Log("%-32s bSuccess=%d, dwValue=%d, pCompletionKey=%p, pContext=%p", 
				__FUNCTION__, bSuccess, dwValue, pCompletionKey, pContext);
			if( NULL == pCompletionKey )
			{
				break;
			}
			if( pClass->m_pCallback )
			{
				pClass->m_pCallback((HANDLE)pCompletionKey, pClass->GetClient(), pClass->m_pCallbackPtr);
			}
		}
		pClass->m_log.Log("%-32s %p disconnected", __FUNCTION__, pCompletionKey);
		FlushFileBuffers((HANDLE)pCompletionKey);
		DisconnectNamedPipe((HANDLE)pCompletionKey);
		CloseHandle((HANDLE)pCompletionKey);
		return 0;
	}
	*/
	BOOL		m_bLowIntegrityMode;
	BOOL		m_bStarted;
	DWORD		m_dwRequestThreadCount;
};

#endif
