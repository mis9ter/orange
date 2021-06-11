#pragma once
#include "yfilter.h"
#include "CQueue.h"
#define TAG_THREADPOOL	'THPO'

/*
inline	PCSTR GetCategoryName(YFilter::Message::Category category) {
	static	PCSTR	pNames[] = {
		"Process",
		"Thread",
		"Module",
		"N/A",
	};
	if (category >= 0 && category < YFilter::Message::Category::Count)
		return pNames[category];
	return pNames[YFilter::Message::Category::Count];
}
*/
class CThreadPool
{
public:
	void		Alert(IN YFilter::Message::Category nCategory)
	{
		if( nCategory < _countof(m_hAlert) ) {
			KeSetEvent(&m_hAlert[nCategory], 0, FALSE);
		}
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
		for( auto i = 0 ; i < YFilter::Message::Category::Count ; i++ ) {
			KeInitializeEvent(&m_hAlert[i], SynchronizationEvent, FALSE);
		}
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
			for (auto i = 0; i < YFilter::Message::Category::Count; i++) {
				m_queue[i].Create(i, "");
			}
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
				for (auto i = 0; i < YFilter::Message::Category::Count; i++) {
					m_queue[i].Destroy();
				}
				if( pWaitBlock )	CMemory::Free(pWaitBlock);
				CMemory::Free(m_pThreadObject);
				m_pThreadObject	= NULL;
			}
		}
	}
	void		SetCallback(PCallbackPop pCallback, PVOID pCallbackPtr)
	{
		for( auto i = 0 ; i < YFilter::Message::Category::Count ; i++ ) {
			m_queue[i].SetCallback(pCallback, pCallbackPtr);
		}
	}
	bool		Push(
		IN	PCSTR						pCause,
		IN	YFilter::Message::Mode		mode,
		IN	YFilter::Message::Category	category,
		IN	PVOID	pItem,
		IN	ULONG	nSize,
		IN	bool	bLog
	)
	{	
		if (category < YFilter::Message::Category::Count) {
			return m_queue[category].Push(pCause, mode, category, pItem, nSize, bLog);
		}
		return false;
	}
private:
	DWORD		m_dwThreadCount;
	PVOID *		m_pThreadObject;
	KEVENT		m_hShutdown;
	KEVENT		m_hAlert[YFilter::Message::Category::Count];
	CQueue		m_queue[YFilter::Message::Category::Count];

	static void	Thread(PVOID p)
	{
		NTSTATUS	status;
		CThreadPool	*pClass	= (CThreadPool *)p;
		PVOID		events[YFilter::Message::Category::Count + 1];
		//IoInitializeRemoveLock()

		//__log("%s %d %d begin", __FUNCTION__, PsGetCurrentProcessId(), PsGetCurrentThreadId());

		MAXIMUM_WAIT_OBJECTS;
		THREAD_WAIT_OBJECTS;
		for( auto i = 0 ; i < YFilter::Message::Category::Count ; i++ ) {
			events[i]	= &pClass->m_hAlert[i];
		}
		events[YFilter::Message::Category::Count]	= &pClass->m_hShutdown;

		PKWAIT_BLOCK	pWaitBlock	= (PKWAIT_BLOCK)CMemory::Allocate(NonPagedPoolNx, 
						sizeof(KWAIT_BLOCK) * _countof(events), 'wait');		
		if (pWaitBlock) {
			bool	bLoop = true;
			while (bLoop)
			{
				/*
					WaitBlockArray
					[out, optional] A pointer to a caller-allocated KWAIT_BLOCK array. 
					If Count <= THREAD_WAIT_OBJECTS, then WaitBlockArray can be NULL. 
					Otherwise, this parameter must point to a memory buffer of 
					sizeof(KWAIT_BLOCK) * Count bytes. 
					
					The routine uses this buffer for record-keeping while performing the wait operation. 
					The WaitBlockArray buffer must reside in nonpaged system memory. 
					For more information, see Remarks.
				*/
				status = KeWaitForMultipleObjects(_countof(events), events, WaitAny, 
							Executive, KernelMode, FALSE, NULL, pWaitBlock);
				if (STATUS_WAIT_0 + YFilter::Message::Category::Count == status) {
					bLoop = false;
				}
				else {
					if (status < STATUS_WAIT_0 + YFilter::Message::Category::Count) {
						if (pClass->m_queue[status - STATUS_WAIT_0].Pop(__FUNCTION__, NULL, NULL)) {
							if (pClass->m_queue[status - STATUS_WAIT_0].Count()) {
								//	queue에 더 남아 있다. 
								pClass->Alert((YFilter::Message::Category)(status - STATUS_WAIT_0));
							}
						}
					}
				}
			}
			CMemory::Free(pWaitBlock);
		}	
		PsTerminateSystemThread(STATUS_SUCCESS);
	}
};
