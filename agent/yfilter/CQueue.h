#pragma once
#include "yfilter.h"

#define TAG_CQUEUE		'euqc'
typedef struct QUEUE_WORKITEM
{
	SLIST_ENTRY		entry;
	char			szCause[32];
	PVOID			pItem;
	ULONG			nSize;
	ULONG			nMode;
} QEUEUE_WORKITEM, *PQUEUE_WORKITEM;

#define USE_SINGLE_LIST		1

typedef	void	(*PCallbackPop)(PQUEUE_WORKITEM pItem, PVOID pCallbackPtr);

#if USE_SINGLE_LIST == 1
class CQueue
{
public:

	void		Create(IN DWORD dwCategory, IN LPCSTR pName)
	{
		//__log("%s %s", __FUNCTION__, pName);
		RtlStringCbCopyA(m_szName, sizeof(m_szName), pName);
		m_dwCategory	= dwCategory;
		KeInitializeSpinLock(&m_lock);
		ExInitializeSListHead(&m_head);
		m_pCallback		= NULL;
		m_pCallbackPtr	= NULL;
		m_bRunning		= true;
	}
	void		Destroy()
	{
		//__log("%s %s", __FUNCTION__, m_szName);
		InterlockedExchange8(&m_bRunning, true);
		while( Pop(__FUNCTION__, NULL, NULL) );
	}
	void		SetCallback(PCallbackPop pCallback, PVOID pCallbackPtr)
	{
		m_pCallback		= pCallback;
		m_pCallbackPtr	= pCallbackPtr;
	}
	bool		Push(
		IN	PCSTR	pCause,
		IN	ULONG	nMode,
		IN	ULONG	nCategory,
		IN	PVOID	pItem,
		IN	ULONG	nSize,
		IN	bool	bLog
	)
	{
		UNREFERENCED_PARAMETER(pCause);
		UNREFERENCED_PARAMETER(nCategory);
		UNREFERENCED_PARAMETER(pItem);
		bool			bRet = false;
		PQUEUE_WORKITEM	p = NULL;

		//__log("%s %d %d %s", __FUNCTION__, PsGetCurrentProcessId(), PsGetCurrentThreadId(), pCause);

		//if( YFilter::Message::Category::Thread == nCategory )	return false;
		UNREFERENCED_PARAMETER(bLog);
		__try
		{
			if( false == m_bRunning )	{
				__dlog("%s m_bRunning is false.", __FUNCTION__);
				__leave;
			}
			p = (PQUEUE_WORKITEM)CMemory::Allocate(NonPagedPoolNx, sizeof(QUEUE_WORKITEM), TAG_CQUEUE);
			if (NULL == p) {
				__dlog("%s allocation failed.", __FUNCTION__);
				__leave;
			}
			RtlZeroMemory(p, sizeof(QUEUE_WORKITEM));
			RtlStringCbCopyA(p->szCause, sizeof(p->szCause), pCause ? pCause : "");
			p->nMode	= nMode;
			p->pItem	= pItem;
			p->nSize	= nSize;

			//PYFILTER_MESSAGE	pMsg = (PYFILTER_MESSAGE)pItem;
			
			if( bLog )	{
				__lock_log(__FUNCTION__, "pushing", (YFilter::Message::Category::Process == nCategory)? "process":"thread", pCause);
			}
			if (ExInterlockedPushEntrySList(&m_head, &p->entry, &m_lock))
			{
				//if (bLog)
				//	__log("%s %s %ws entered", __FUNCTION__, m_szName, pMsg->data.szPath);
				/*
					Return value
					The return value is the previous first item in the list.
					If the list was previously empty, the return value is NULL.

					리턴값을 검사하는 것은 의미가 없다.
					리스트의 이전 제일 첫번째 포인터를 반환하므로
					제일 첫번째 넣을 때는 NULL이 반환된다.

					[TODO]
					왜 이따위로 만든걸까?
					지금 들어간 것을 반환해주지 않고.
				*/
			}
			if( bLog )	__lock_log(__FUNCTION__, "pushed", (YFilter::Message::Category::Process == nCategory) ? "process" : "thread", pCause);
			bRet = true;
		}
		__finally
		{
			if (false == bRet)
			{
				if (p)
				{
					CMemory::Free(p);
				}
			}
		}
		return bRet;
	}
	bool		Pop(IN PCSTR pCause, PCallbackPop pCallback, PVOID pCallbackPtr)
	{
		bool	bRet	= false;
		UNREFERENCED_PARAMETER(pCause);
		PQUEUE_WORKITEM	p = NULL;

		__lock_log(__FUNCTION__, "poping", m_szName, pCause);
		//while(ExQueryDepthSList(&m_head))
		{
			p = (PQUEUE_WORKITEM)ExInterlockedPopEntrySList(&m_head, &m_lock);
			if (p)
			{
				if (m_pCallback)		m_pCallback(p, m_pCallbackPtr);
				if (pCallback)			pCallback(p, pCallbackPtr);
				if (p->pItem)			CMemory::Free(p->pItem);
				CMemory::Free(p, TAG_CQUEUE);
				bRet	= true;
			}
		}
		__lock_log(__FUNCTION__, "poped", m_szName, pCause);
		return bRet;
	}
	USHORT		Count() {
		return ExQueryDepthSList(&m_head);
	}
private:
	volatile	char	m_bRunning;
	char				m_szName[33];
	DWORD				m_dwCategory;
	SLIST_HEADER		m_head;
	KSPIN_LOCK			m_lock;
	PCallbackPop		m_pCallback;
	PVOID				m_pCallbackPtr;
};
#else 


#endif