#pragma once
#include <process.h>

typedef std::function<void(void* pContext, HANDLE hShutdown)>		PFUNC_THREAD;

typedef struct Thread
{
	DWORD		dwTID;
	HANDLE		hThread;
	HANDLE		hShutdown;
} Thread, *PThread;

class CThread
{
public:
	CThread()
		:
		m_dwTID(0),
		m_hThread(NULL),
		m_hShutdown(NULL),
		m_bCloseShutdownHandle(false),
		m_pContext(NULL),
		m_pCallback(NULL)
	{
		
	}
	~CThread()
	{
		if( m_hThread )	
		{
			CloseHandle(m_hThread);
			m_hThread	= NULL;
		}
		if( m_bCloseShutdownHandle )	CloseHandle(m_hShutdown);
	}
	bool			Start(
		IN	void *			pContext, 
		IN	HANDLE			pShutdown,		
		IN	PFUNC_THREAD	p
	)
	{
		printf("%-30s begin\n", __FUNCTION__);
		//m_pThreadFunc	= p;
		//m_pThreadPtr	= pContext;
		m_hThread = (HANDLE)_beginthreadex(NULL, 0, Thread, this, 0, NULL);
		printf("  m_hThread=%p\n", m_hThread);
		if (NULL == m_hThread)	return false;
		return true;
	}
	void			Stop()
	{
		if (m_hThread && m_hShutdown)
		{
			SetEvent(m_hShutdown);
		}
	}
	bool			Wait(IN DWORD dwWaitingSeconds = 0)
	{
		DWORD	dwRet	= WAIT_OBJECT_0;
		if (m_hThread)
		{
			dwRet = WaitForSingleObject(m_hThread, dwWaitingSeconds ? dwWaitingSeconds * 1000 : INFINITE);
			CloseHandle(m_hThread);
			m_hThread	= NULL;
		}
		return (WAIT_OBJECT_0 == dwRet)? true : false;
	}
private:
	DWORD			m_dwTID;
	HANDLE			m_hThread;
	bool			m_bCloseShutdownHandle;
	HANDLE			m_hShutdown;
	PVOID			m_pContext;
	PFUNC_THREAD	m_pCallback;

	static	unsigned	__stdcall	Thread(void* ptr)
	{
		CThread* pClass = (CThread*)ptr;

		//if( pClass->m_pThreadFunc )
		//	pClass->m_pThreadFunc(pClass->m_pThreadPtr, pClass->m_hShutdown);
		return 0;
	}
};
