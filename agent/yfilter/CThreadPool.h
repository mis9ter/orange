#pragma once
#include "yfilter.h"
#include "CQueue.h"
#define TAG_THREADPOOL	'THPO'

class CThreadPool	:
	public	CQueue
{
public:
	void		Alert()
	{
		KeSetEvent(&m_hAlert, 0, FALSE);
	}
	bool		Create(IN DWORD dwThreadCount)
	{
		bool		bRet	= false;
		NTSTATUS	status;
		HANDLE		hThread;

		__log(__FUNCTION__);

		m_pThreadObject	= NULL;
		m_dwThreadCount	= dwThreadCount;
		KeInitializeEvent(&m_hShutdown, NotificationEvent, FALSE);
		KeInitializeEvent(&m_hAlert, SynchronizationEvent, FALSE);
		__try
		{
			m_pThreadObject = (PVOID*)CMemory::Allocate(NonPagedPoolNx, sizeof(PVOID) * dwThreadCount, TAG_THREADPOOL);
			if( NULL == m_pThreadObject )	__leave;
			for (DWORD i = 0; i < dwThreadCount; i++)
			{
				status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL,
					Thread, this);
				if (NT_FAILED(status))
				{
					__leave;
				}
				status	= ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, NULL, KernelMode, &m_pThreadObject[i], NULL);
				if (NT_FAILED(status))
				{
					__leave;				
				}
				ZwClose(hThread);
			}
			CQueue::Create();
			bRet	= true;
		}
		__finally
		{

		}
		return true;
	}
	void		Destroy()
	{
		__log(__FUNCTION__);
		KWAIT_BLOCK		*pWaitBlock	= NULL;
		NTSTATUS		status;
		if (m_pThreadObject)
		{
			KeSetEvent(&m_hShutdown, 0, FALSE);
			__try
			{
				pWaitBlock	= (KWAIT_BLOCK *)CMemory::Allocate(NonPagedPoolNx, sizeof(KWAIT_BLOCK) * m_dwThreadCount, TAG_THREADPOOL);
				if( NULL == pWaitBlock )	__leave;
				status	= KeWaitForMultipleObjects(m_dwThreadCount, m_pThreadObject, WaitAll, Executive, KernelMode, FALSE, NULL, pWaitBlock);
				if( NT_FAILED(status))	__leave;
				for (DWORD i = 0; i < m_dwThreadCount; i++)
				{
					ObDereferenceObject(m_pThreadObject[i]);
				}
			}
			__finally
			{
				CQueue::Destroy();
				if( pWaitBlock )	CMemory::Free(pWaitBlock);
				CMemory::Free(m_pThreadObject);
				m_pThreadObject	= NULL;
			}
		}
	}
private:
	DWORD		m_dwThreadCount;
	PVOID *		m_pThreadObject;
	KEVENT		m_hShutdown;
	KEVENT		m_hAlert;

	static void	Thread(PVOID p)
	{
		NTSTATUS	status;
		CThreadPool	*pClass	= (CThreadPool *)p;
		PVOID		events[2];
		//IoInitializeRemoveLock()

		__log("%s begin", __FUNCTION__);
		events[0]	= &pClass->m_hShutdown;
		events[1]	= &pClass->m_hAlert;
		while (true)
		{
			status	= KeWaitForMultipleObjects(2, events, WaitAny, Executive, KernelMode, FALSE, NULL, NULL );
			if( STATUS_WAIT_0 == status )	break;
			pClass->Pop(NULL, NULL);
		}
		__log("%s end", __FUNCTION__);
		PsTerminateSystemThread(STATUS_SUCCESS);
	}
};
