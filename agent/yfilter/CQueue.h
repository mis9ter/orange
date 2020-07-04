#pragma once
#include "yfilter.h"

#define TAG_CQUEUE		'euqc'
typedef struct QUEUE_WORKITEM
{
	SLIST_ENTRY		entry;
	PVOID			pItem;
	ULONG			nSize;
	ULONG			nType;
} QEUEUE_WORKITEM, *PQUEUE_WORKITEM;

typedef	void	(*PCallbackPop)(PQUEUE_WORKITEM pItem, PVOID pCallbackPtr);
class CQueue
{
protected:

	void		Create()
	{
		__log(__FUNCTION__);
		KeInitializeSpinLock(&m_lock);
		ExInitializeSListHead(&m_head);
		m_pCallback		= NULL;
		m_pCallbackPtr	= NULL;
	}
	void		Destroy()
	{
		__log(__FUNCTION__);
		Pop(NULL, NULL);
	}

public:
	void		SetCallback(PCallbackPop pCallback, PVOID pCallbackPtr)
	{
		m_pCallback		= pCallback;
		m_pCallbackPtr	= pCallbackPtr;
	}
	bool		Push(IN ULONG nType, IN PVOID pItem, IN ULONG nSize)
	{
		//__log(__FUNCTION__);
		UNREFERENCED_PARAMETER(pItem);
		bool			bRet	= false;
		PQUEUE_WORKITEM	p	= NULL;
		
		__try
		{
			p	= (PQUEUE_WORKITEM)CMemory::Allocate(NonPagedPoolNx, sizeof(QUEUE_WORKITEM), TAG_CQUEUE);
			if( NULL == p )	__leave;

			RtlZeroMemory(p, sizeof(QUEUE_WORKITEM));
			p->nType	= nType;
			p->pItem	= pItem;
			p->nSize	= nSize;
			if (ExInterlockedPushEntrySList(&m_head, &p->entry, &m_lock))
			{
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
			bRet		= true;
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
	void		Pop(PCallbackPop pCallback, PVOID pCallbackPtr)
	{
		//__log(__FUNCTION__);
		PQUEUE_WORKITEM	p = NULL;
		while (ExQueryDepthSList(&m_head))
		{
			p	= (PQUEUE_WORKITEM)ExInterlockedPopEntrySList(&m_head, &m_lock);
			if( p )
			{
				if( m_pCallback )	m_pCallback(p, m_pCallbackPtr);
				if( pCallback )		pCallback(p, pCallbackPtr);
				if( p->pItem )	CMemory::Free(p->pItem);
				CMemory::Free(p, TAG_CQUEUE);
			}
		}
	}


private:
	SLIST_HEADER		m_head;
	KSPIN_LOCK			m_lock;
	PCallbackPop		m_pCallback;
	PVOID				m_pCallbackPtr;
};
