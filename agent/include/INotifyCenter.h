#pragma once
/*
	�̺�Ʈ�� �߻��Ǹ� �̺�Ʈ�� ������ ����� Notify�� �ش�.
	��� �̺�Ʈ�� �����ִ� ����� �� �̺�Ʈ�� �޴´ٰ� ����ϰ� �߻��� �޴´�.	
	INotifyCenter�� �̷� ������ �������ش�.
*/
#include <functional>

///////////////////////////////////////////////////////////////////////////////////////////////////
#define NOTIFY_TYPE_AGENT				0
#define NOTIFY_EVENT_PERIODIC			0		//	�ֱ��� ȣ��
///////////////////////////////////////////////////////////////////////////////////////////////////
#define NOTIFY_TYPE_SYSTEM				1
#define	NOTIFY_EVENT_SHUTDOWN			0
#define	NOTIFY_EVENT_PRESHUTDOWN		1
///////////////////////////////////////////////////////////////////////////////////////////////////
#define NOTIFY_TYPE_SESSION				2
#define NOTIFY_EVENT_SESSION			2
///////////////////////////////////////////////////////////////////////////////////////////////////

typedef std::function<void(WORD wType, WORD wEvent, 
						PVOID pData, ULONG_PTR nData, PVOID pContext)>	PFUNC_NOTIFY_CALLBACK;
typedef std::function<void(DWORD64 dwValue, bool * bCallback)>			PFUNC_NOTIFY_FILTER;

typedef struct NotifyItem {
	NotifyItem(PCSTR _s, DWORD _n, DWORD64 _v, PVOID _p, PFUNC_NOTIFY_CALLBACK _c) {
		dwNotify	= _n;
		dwValue		= _v;
		pContext	= _p;
		pCallback	= _c;
		cause		= _s;
	}
	DWORD					dwNotify;
	DWORD64					dwValue;
	PFUNC_NOTIFY_CALLBACK	pCallback;
	PVOID					pContext;	
	std::string				cause;
} NotifyItem, *PNotifyItem;

__interface INotifyCenter
{
	virtual	void			RegisterNotifyCallback(
		PCSTR					pCause,
		WORD					wType,		
		WORD					wEvent, 
		DWORD64					dwValue,
		PVOID					pContext,	PFUNC_NOTIFY_CALLBACK	pCallback
	)	= NULL;
	virtual	DWORD			Notify(
		WORD					wType, 
		WORD					wEvent,
		PVOID					pData,
		ULONG_PTR				nData,
		PFUNC_NOTIFY_FILTER		pFilter
	)	= NULL;
	//	return���� Notify�� ���޵� ����
};
