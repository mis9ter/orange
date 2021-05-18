#include "stdafx.h"

#include "CxIpc.h"

#include "xLogWriter.h"

#define _log /##/


// #define __lock		m_lock.Lock()
// #define __unlock	m_lock.Unlock()

#define __lock		/##/
#define __unlock	/##/

//#pragma comment(lib, "xcommon_dll.lib")
//#pragma comment(lib, "xcommon_static.lib")

CxIPc::CxIPc()
{
	__retainCount	= 0;

// 	WSADATA		wd;
// 
// 	::WSAStartup(0x0202, &wd);

	m_szServerName[0]			= NULL;
	::ZeroMemory(m_threads, sizeof(m_threads));

	m_pCallbackAddress	= NULL;
	m_pObject			= NULL;

	m_hShutdown			= ::CreateEvent(NULL, TRUE, FALSE, NULL);
	m_bShutdown			= FALSE;

	m_bLowIntegrityMode = FALSE;
	m_bStarted			= FALSE;

	m_dwRequestThreadCount = 1;

	::InitializeCriticalSection(&m_lock);

	m_CreateFileW_Base = (XPFN_CREATEFILEW)GetProcAddress(GetModuleHandle(L"kernelbase.dll"), "CreateFileW");

	if (NULL == m_CreateFileW_Base)
	{
		m_CreateFileW_Base = ::CreateFileW;
	}
}

CxIPc::~CxIPc()
{
	::CloseHandle(m_hShutdown);
	m_hShutdown = NULL;

	::DeleteCriticalSection(&m_lock);

}

IxIPcServer *	CxIPc::GetServer()
{
	return dynamic_cast<IxIPcServer *>(this);
}

IxIPcClient *	CxIPc::GetClient()
{
	return dynamic_cast<IxIPcClient *>(this);
}

void	CxIPc::SetServiceCallback(IN IPCServiceCallback pAddress)
{
	m_pCallbackAddress	= pAddress;
}

void	CxIPc::SetObject(IN void *pObject)
{
	m_pObject	= pObject;
}

void	CxIPc::Free(IN VOID *ptr)
{
	if( ptr )
	{
		delete ptr;
	} else {
		Shutdown();

		delete this;
	}
}

HANDLE	CxIPc::Connect(IN LPCTSTR pName)
{
	HANDLE	hPipe = NULL;
	DWORD	dwError;

	//_log(_T("--------------------------------------------------------------------"));

	__try
	{
		__lock;

		for (DWORD dwIndex = 0; dwIndex < 5; dwIndex++)
		{
			//hPipe	= ::CreateFile(pName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
			hPipe = m_CreateFileW_Base(pName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
			
			if( INVALID_HANDLE_VALUE != hPipe )
			{
//				xErrorLog(_T("__connected_to_user, pName : %s"), pName);
				break;
			}

			dwError	= ::GetLastError();
			if( dwError != ERROR_PIPE_BUSY )
			{
				//xErrorLog(L"error: can not connect to user. code = %d, pName : %s", dwError, pName);
				Sleep(100);
				continue;
			}

			if( !::WaitNamedPipe(pName, 3000) )
			{
				xErrorLog(L"error: can not wait to user. code = %d pName : %s", ::GetLastError(), pName);
				__leave;
			}			
		}
	}
	__finally
	{
		__unlock;
	}
	return hPipe;
}

void	CxIPc::Disconnect(IN HANDLE hIpc)
{
	__lock;

	if( INVALID_HANDLE_VALUE != hIpc )
	{
		//_log(_T("DISCONNECTED_FROM_IPCSERVER"));

		::FlushFileBuffers(hIpc);
		::DisconnectNamedPipe(hIpc);
		::CloseHandle(hIpc);
	}

	__unlock;

	//_log(_T("--------------------------------------------------------------------"));
}

bool	CxIPc::Write(
				IN HANDLE	hIpc,
				IN void		*pData,
				IN DWORD	dwDataSize
)
{
	bool		bRet	= false;
	DWORD		dwBytes;

	if( hIpc == INVALID_HANDLE_VALUE )
	{
		return false;
	}

	__try
	{
		__lock;

		if( ::WriteFile(hIpc, pData, dwDataSize, &dwBytes, NULL) &&
			dwBytes == dwDataSize )
		{
			//_log(_T("IPCWRITE_%d"), dwBytes);
		}
		else
		{
			//_log(_T("error: can not write. code = %d"), ::GetLastError());
			xErrorLog(L"[FAILED] Fail to Write Pipe");
			__leave;
		}

		bRet	= true;
	}
	__finally
	{
		__unlock;
	}

	//_log(_T("IPCWRITE:%d"), bRet);

	return bRet;
}

bool	CxIPc::Read(
				IN HANDLE	hIpc,
				OUT void	*pData,
				IN DWORD	dwDataSize
)
{
	bool	bRet	= false;
	DWORD	dwBytes;

	if( hIpc == INVALID_HANDLE_VALUE )
	{
		//_log(_T("__CIPc_Read_INVALID_HANDLE_VALUE"));
		return false;
	} 

	__try
	{
		__lock;

		if( dwDataSize )
		{
			if( ::ReadFile(hIpc, pData, dwDataSize, &dwBytes, NULL) && dwBytes == dwDataSize )
			{
				//_log(_T("IPCREAD_%d"), dwBytes);
				bRet	= true;
				__leave;
			}
			else
			{
				//_log(_T("error: can not read. code = %d"), ::GetLastError());
			}
		}
	}
	__finally
	{
		__unlock;
	}

	return bRet;
}

///////////////////////////////////////////////////////////////////////////////

bool	CxIPc::Start(IN LPCTSTR pName, BOOL	bLowIntegrityMode, LONG		RequestThreadCount)
{
	unsigned	uId, i;
	HANDLE		hEventArray[REQUEST_THREAD_COUNT_MAX+1] = {0,};
	DWORD		dwResult = 0;
	bool		bReturnValue = FALSE;
	HANDLE		hIocp = NULL;

	if (RequestThreadCount > REQUEST_THREAD_COUNT_MAX)
	{
		RequestThreadCount = REQUEST_THREAD_COUNT_MAX;
	}

	m_dwRequestThreadCount = RequestThreadCount;
	m_bLowIntegrityMode = bLowIntegrityMode;

	::ResetEvent(m_hShutdown);

	::StringCbCopy(m_szServerName, sizeof(m_szServerName), pName);

	m_threads[0].pThis		= this;
	m_threads[0].hIocp		= ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	m_threads[0].hInitCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_threads[0].hThread	= (HANDLE)_beginthreadex(NULL, 0, ServiceThread, this, 0, &uId);
	hEventArray[0] = m_threads[0].hInitCompleteEvent;

	hIocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	for( i = 1 ; i <= m_dwRequestThreadCount; i++ )
	{
		m_threads[i].pThis		= this;
		m_threads[i].hIocp		= hIocp;
		m_threads[i].hInitCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_threads[i].hThread	= (HANDLE)_beginthreadex(NULL, 0, RequestThread, &m_threads[i], 0, &uId);		
		hEventArray[i] = m_threads[i].hInitCompleteEvent;
	}

	dwResult = WaitForMultipleObjects(m_dwRequestThreadCount+1, hEventArray, TRUE, 30 * 1000);
	switch (dwResult)
	{
	case WAIT_OBJECT_0:
	case WAIT_OBJECT_0 + 1:
		xErrorLog(L"Succeed to Initialize Threads");
		bReturnValue = TRUE;
		m_bStarted = TRUE;
		break;

	case WAIT_TIMEOUT:
	default:
		xErrorLog(L"Fail to Initialize Threads (dwResult %d, m_dwRequestThreadCount %d)", dwResult, m_dwRequestThreadCount);
		bReturnValue = FALSE;

	}
	
	return bReturnValue;
}

void	CxIPc::Shutdown()
{
	unsigned	i;
	HANDLE		hThreads[REQUEST_THREAD_COUNT_MAX+1] = {0,};

	if (m_bStarted == FALSE)
	{
		return;
	}

	::SetEvent(m_hShutdown);
	m_bShutdown = TRUE;

	HANDLE	hUser	= Connect(m_szServerName);
	if( INVALID_HANDLE_VALUE != hUser )
	{		
		Disconnect(hUser);
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


void	CxIPc::CallThread(
			IN IPCThreadType	type,
			IN LPVOID			object,
			IN DWORD			dwValue
		)
{
	if (m_threads[type].hIocp)
	{
		::PostQueuedCompletionStatus(m_threads[type].hIocp, dwValue, (ULONG_PTR)object, NULL);
	}
}

unsigned	CxIPc::ServiceThread(IN LPVOID p)
{
	CxIPc		*pClass	= (CxIPc *)p;
	HANDLE		hPipe;
	BOOL		bConnected;
	BOOL		bInitialized = FALSE;

	PACL		pAcl = NULL;
	PSECURITY_ATTRIBUTES	pSa = NULL;
	BOOL		bResult = FALSE;

	xErrorLog(L"IPC_SERVER[%s]_BEGAN", pClass->m_szServerName);

	::CoInitializeEx(NULL, COINIT_MULTITHREADED);

	if (pClass->m_bLowIntegrityMode != FALSE && CxCommonApi::IsVistaOrLater())
	{
		bResult = CxCommonApi::BuildSidFromSddl(&pSa, &pAcl);
		if (bResult == FALSE)
		{
			xErrorLog(L"BuildSidFromSddl Failed");
		}
		else {
			xTrace(L"Create NamedPipe as LowIntegrityMode");
		}
	}

	while( true )
	{
		hPipe	= ::CreateNamedPipe(pClass->m_szServerName, 
									PIPE_ACCESS_DUPLEX, 
									PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT,
									PIPE_UNLIMITED_INSTANCES,
									IPC_DATA_SIZE,
									IPC_DATA_SIZE,
									0,
									pSa);

		if( INVALID_HANDLE_VALUE == hPipe )
		{
			xErrorLog(L"error: hPipe is INVALID_HANDLE_VALUE");
			break;
		}

		if (bInitialized == FALSE)
		{
			SetEvent(pClass->m_threads[0].hInitCompleteEvent);

			bInitialized = TRUE;
		}
		
// 		if( WAIT_OBJECT_0 == ::WaitForSingleObject(pClass->m_hShutdown, 0) )
// 		{
// 			::CloseHandle(hPipe);
// 			break;
// 		}

		if (pClass->m_bShutdown)
		{
			::CloseHandle(hPipe);
			break;
		}
		
		bConnected	= ConnectNamedPipe(hPipe, NULL)? TRUE : (::GetLastError() == ERROR_PIPE_CONNECTED);

		if( bConnected )
		{
			//xErrorLog(L"[PIPE_CONNECTED] bConnected %d LastError %d", bConnected, GetLastError());
			pClass->CallThread(IPCThreadTypeRequest, hPipe, 0);
		}
		else
		{
			xErrorLog(L"[FAILED] ConnectNamedPipe Failed.");
			::CloseHandle(hPipe);
		}

	}

	if (pAcl)
	{
		CxCommonApi::FreeAnnonymousSid(pSa, pAcl);
		pAcl = NULL;
	}

	xErrorLog(L"IPC_SERVER[%s]_FINISHED", pClass->m_szServerName);

	::CoUninitialize();

	return 0;
}

unsigned	CxIPc::RequestThread(IN LPVOID p)
{
	BOOL			bSuccess;
	ULONG_PTR		pCompletionKey;	
	LPOVERLAPPED	pContext;
	DWORD			dwValue;
	ThreadInfo*		pInfo = (ThreadInfo*)p;
	CxIPc			*pClass		= (CxIPc*)pInfo->pThis;

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
		
		xLog(L"__IPC_RequestThread:[%d][%d][%08x][%08x]", bSuccess, dwValue, pCompletionKey, pContext);

		if( NULL == pCompletionKey )
		{
			break;
		}

		//xErrorLog(L"[PIPE_RECV] bSuccess %d, dwValue %d", bSuccess, dwValue);
		xLog(L"IPCCLIENT_IS_CONNECTED");

		if( pClass->m_pCallbackAddress )
		{
			pClass->m_pCallbackAddress((HANDLE)pCompletionKey, pClass->GetClient(), pClass->m_pObject);
		}

		FlushFileBuffers((HANDLE)pCompletionKey);
		DisconnectNamedPipe((HANDLE)pCompletionKey);
		CloseHandle((HANDLE)pCompletionKey);
	}

	//xErrorLog(L"IPC_REQUEST_FINISHED");

	return 0;
}


