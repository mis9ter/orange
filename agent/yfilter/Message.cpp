#include "yfilter.h"

void		MessageThreadCallback(PQUEUE_WORKITEM p, PVOID pCallbackPtr)
{
	UNREFERENCED_PARAMETER(p);
	UNREFERENCED_PARAMETER(pCallbackPtr);
	if (p)
	{
		//__log("%s [category:%d][bCreationSaved:%d][%ws]", 
		//		__FUNCTION__, 
		//		pMsg->header.category, pMsg->data.bCreationSaved,
		//		pMsg->data.szPath);
		PYFILTER_MESSAGE	pMsg	= (PYFILTER_MESSAGE)p->pItem;
		bool				bLog	= false;;

		if( YFilter::Message::Category::Thread == pMsg->header.category )
			bLog	= true;
		if( YFilter::Message::Mode::Event == p->nMode ) {
			if (NT_FAILED(SendMessage(p->szCause, &Config()->client.event, p->pItem, p->nSize, true)) )
			{

			}
		}
		else {
			__log("unknown mode %d", p->nMode);
		}
	}
}
void		SetMessageThreadCallback(PVOID pCallbackPtr)
{
	MessageThreadPool()->SetCallback(MessageThreadCallback, pCallbackPtr);
}

NTSTATUS	SendMessage(
	IN	PCSTR				pCause,
	IN	PFLT_CLIENT_PORT	p,
	IN	PVOID				pSendData, 
	IN	ULONG				nSendDataSize,
	IN	bool				bLog
)
{
	/*
		응답 필요 없으니 그냥 보내라.
		하지만 내부적으로는 늘 응답을 받는 구조로 되어 있다.
	*/
	FILTER_REPLY		reply = { 0, };
	ULONG				nSize = sizeof(FILTER_REPLY);
	NTSTATUS			status = SendMessage(pCause, p, pSendData, nSendDataSize, &reply, &nSize, bLog);
	if (NT_SUCCESS(status))
	{
		//__log("%s reply.bRet=%d", __FUNCTION__, reply.bRet);
	}
	return status;
}
NTSTATUS	SendMessage(
	IN	PCSTR				pCause,
	IN	PFLT_CLIENT_PORT	p,
	IN	PVOID pSendData, IN ULONG nSendDataSize,
	OUT	PVOID pRecvData, OUT ULONG* pnRecvDataSize,
	IN	bool	bLog
)
{
	//__log(__FUNCTION__);
	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	if (KeGetCurrentIrql() > APC_LEVEL) {
		__dlog("%s ERROR KeGetCurrentIrql()=%d", __FUNCTION__, KeGetCurrentIrql());
		return status;
	}
	if (NULL == Config() || NULL == Config()->pFilter || NULL == p)
	{
		//	[TODO]	Config() 함수의 동기화는 누가 시켜주지?
		//__log("%s ERROR Config()=%p, Config()->pFilter=%p, p=%p",
		//	__FUNCTION__, Config(), Config() ? Config()->pFilter : NULL, p);
		return status;
	}
	//__log("%s %ws %p %d", __FUNCTION__, p->szName, pSendData, nSendDataSize);
	CAutoReleaseSpinLock(__FUNCTION__, &p->lock, bLog);
	{
		LARGE_INTEGER	timeout;
		timeout.QuadPart = 3 * 1000 * 1000;
		__try
		{
			if (NULL == p->pPort)
			{
				//__log("%s ERROR p->pPort=%p", __FUNCTION__, p->pPort);
				__leave;
			}
			status = FltSendMessage(Config()->pFilter,
				&p->pPort,
				pSendData, nSendDataSize,
				pRecvData, pnRecvDataSize, NULL);
			STATUS_BUFFER_OVERFLOW;
			STATUS_TIMEOUT;
			if (STATUS_SUCCESS == status)
			{
				if (pRecvData && pnRecvDataSize)
				{
					//__log("%s STATUS_SUCCESS %p, %d", __FUNCTION__,
					//	pRecvData, *pnRecvDataSize);
				}
				__leave;
			}
			else if (STATUS_TIMEOUT == status)
			{
				if (0 == timeout.QuadPart)
				{
					__leave;
				}
				else
				{

				}
			}
			__log("%s ERROR FltSendMessage(%s)=%x, pRecvData=%p,pnRecvDataSize=%p(%d)",
				__FUNCTION__, pCause ? pCause : "", status,
				pRecvData, pnRecvDataSize, (pnRecvDataSize ? *pnRecvDataSize : -1));
			PYFILTER_MESSAGE	pMsg = (PYFILTER_MESSAGE)pSendData;

			__log("%ws", pMsg->data.szPath);
		}
		__finally
		{

		}
	}
	return status;
}