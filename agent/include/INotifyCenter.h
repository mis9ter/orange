#pragma once
/*
	이벤트가 발생되면 이벤트를 감지한 모듈은 Notify를 준다.
	헤당 이벤트에 관심있는 모듈은 그 이벤트를 받는다고 등록하고 발생시 받는다.	
	INotifyCenter는 이런 역할을 수행해준다.
*/
#include <functional>

///////////////////////////////////////////////////////////////////////////////////////////////////
#define NOTIFY_TYPE_AGENT				0
#define NOTIFY_EVENT_PERIODIC			0		//	주기적 호출
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
	//	return값은 Notify가 전달된 개수
};
