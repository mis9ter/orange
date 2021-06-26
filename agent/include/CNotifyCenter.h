#pragma once
#include "INotifyCenter.h"
#include "Yagent.common.h"
#include <windef.h>
#include <memory>
#include <map>
#include <list>

typedef std::shared_ptr<NotifyItem>			NotifyItemPtr;
typedef std::list<NotifyItemPtr>			NotifyItemList;
typedef struct NotifyListener {
	NotifyListener() {
	
	}
	CLock			lock;
	NotifyItemList	listener;
} NotifyListener;
typedef std::shared_ptr<NotifyListener>		NotifiListenerPtr;
typedef std::map<DWORD, NotifiListenerPtr>	NotifyItemMap;

class CNotifyCenter
	:
	public	INotifyCenter
{
public:
	CNotifyCenter() 
		:
		m_log(L"NotifyCenter.log")
	{
		//	�̺�Ʈ û�� ���/���� �������� ����ȭ�� ���� �ʰڴ�.
		//	�ʱ�ȭ �������� �ʿ��� �ǵ��� ��� �� ����ϰ�
		//	�����ϴ� �������� ����� �� ����.
		m_bRegister	= true;
	}
	~CNotifyCenter() {
	}
	void			RegisterNotifyCallback(
		PCSTR	pCause,
		WORD	wType, WORD wEvent, 
		DWORD64	dwValue,
		PVOID	pContext, PFUNC_NOTIFY_CALLBACK	pCallback
	) {
		if( false == m_bRegister ) {
			m_log.Log("%s %4d %4d %s : registration failed.", __FUNCTION__, wType, wEvent, pCause);
			return;
		}
		m_log.Log("%s %4d %4d %s", __FUNCTION__, wType, wEvent, pCause);

		DWORD	dwNotify	= MAKELONG(wEvent, wType);
		auto	t	= m_table.find(dwNotify);
		if( m_table.end() == t ) {
			//	���� ��ϵ� ���� �����Ƿ� ���� ����ϴ�.
			NotifiListenerPtr	ptr	= std::make_shared<NotifyListener>();
			__function_lock(ptr->lock.Get());
			ptr->listener.push_back(std::make_shared<NotifyItem>(pCause, dwNotify, dwValue, pContext, pCallback));
			{
				__function_lock(m_tableLock.Get());
				m_table[dwNotify]	= ptr;
			}
		}	
		else {
			//	������ ��ϵ� ���� �ֽ��ϴ�.
			__function_lock(t->second->lock.Get());
			t->second->listener.push_back(std::make_shared<NotifyItem>(pCause, dwNotify, dwValue, pContext, pCallback));
		}
	}
	void			EnableRegistration(IN bool bFlag) {
		m_bRegister	= bFlag;
	}
	DWORD			Notify(
		WORD				wType, 
		WORD				wEvent,
		PVOID				pData,
		ULONG_PTR			nDataSize,
		PFUNC_NOTIFY_FILTER	pFilter
	) {
		m_log.Log("%s %4d %4d", __FUNCTION__, wType, wEvent);
		DWORD	dwNotify	= MAKELONG(wEvent, wType);
		DWORD	dwCount		= 0;
		auto	t			= m_table.find(dwNotify);
		if( m_table.end() != t ) {
			//__function_lock(t->second->lock.Get());
			bool	bCallback	= true;
			for( auto i : t->second->listener ) {
				if( pFilter )	pFilter(i->dwValue, &bCallback);
				if( bCallback )	{
					i->pCallback(wType, wEvent, pData, nDataSize, i->pContext);
					m_log.Log("  %4d %4d %s", wType, wEvent, i->cause.c_str());
				}
				dwCount++;
			}
		}
		return dwCount;
	}

private:
	std::atomic<bool>		m_bRegister;
	CLock					m_tableLock;
	NotifyItemMap			m_table;
	CAppLog					m_log;
};
