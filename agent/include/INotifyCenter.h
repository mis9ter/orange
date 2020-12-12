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

typedef std::function<void(WORD wType, WORD wEvent, PVOID pData, ULONG_PTR nDataSize, PVOID pContext)>	PFUNC_NOTIFY_CALLBACK;

__interface INotifyCenter
{
	virtual	void			RegisterNotifyCallback(
		PCSTR					pCause,
		WORD					wType, 
		WORD					wEvent, 
		PVOID					pContext, 
		PFUNC_NOTIFY_CALLBACK	pCallback
	)	= NULL;
	virtual	DWORD			Notify(
		WORD					wType, 
		WORD					wEvent,
		PVOID					pData,
		ULONG_PTR				nDataSize	
	)	= NULL;
	//	return���� Notify�� ���޵� ����
};
