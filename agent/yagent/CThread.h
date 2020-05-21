#pragma once
#include <process.h>

typedef std::function<void(void* pThreadPtr, HANDLE hShutdown)>		PFUNC_THREAD;

class CThread
{
public:
	CThread()
		:
		m_hThread(NULL),
		m_pThreadPtr(NULL),
		m_pThreadFunc(NULL),
		m_hShutdown(CreateEvent(NULL, TRUE, FALSE, NULL))
	{
		
	}
	~CThread()
	{
		if( m_hThread )	
		{
			CloseHandle(m_hThread);
			m_hThread	= NULL;
		}
		CloseHandle(m_hShutdown);
	}
	bool			Start(IN void * pThreadPtr, IN PFUNC_THREAD p)
	{
		printf("%-30s begin\n", __FUNCTION__);
		m_pThreadFunc	= p;
		m_pThreadPtr	= pThreadPtr;
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
	HANDLE			m_hThread;
	HANDLE			m_hShutdown;
	void			*m_pThreadPtr;
	PFUNC_THREAD	m_pThreadFunc;

	static	unsigned	__stdcall	Thread(void* ptr)
	{
		CThread* pClass = (CThread*)ptr;

		if( pClass->m_pThreadFunc )
			pClass->m_pThreadFunc(pClass->m_pThreadPtr, pClass->m_hShutdown);
		return 0;
	}
};
