#include "yfilter.h"

WORD		GetStringDataSize(PUNICODE_STRING pStr) {
	if( NULL == pStr || 0 == pStr->Length )	return sizeof(WCHAR);
	return (pStr->Length + sizeof(WCHAR));
}
void		CopyStringData(PVOID pAddress, WORD wOffset, PY_STRING pDest, PUNICODE_STRING pSrc) {
	pDest->wOffset		= wOffset;
	pDest->wSize		= GetStringDataSize(pSrc);
	pDest->pBuf			= (WCHAR *)((char *)pAddress + wOffset);
	if( pSrc )
		RtlStringCbCopyUnicodeString(pDest->pBuf, pDest->wSize, pSrc);
	else 
		RtlStringCbCopyW(pDest->pBuf, pDest->wSize, L"");
}
void		CreateProcessMessage(
	YFilter::Message::SubType	subType,
	PPROCESS_ENTRY				pEntry,
	PY_PROCESS					*pOut
) {
	WORD	wSize	= sizeof(Y_PROCESS);
	wSize	+= GetStringDataSize(&pEntry->DevicePath);
	wSize	+= GetStringDataSize(&pEntry->Command);

	PY_PROCESS		pMsg	= NULL;

	pMsg	= (PY_PROCESS)CMemory::Allocate(PagedPool, wSize, TAG_PROCESS);
	if( pMsg ) {
		RtlZeroMemory(pMsg, wSize);
		pMsg->mode		= YFilter::Message::Mode::Event;
		pMsg->category	= YFilter::Message::Category::Process;
		pMsg->subType	= subType;
		pMsg->wSize		= wSize;
		pMsg->wRevision	= Y_MESSAGE_REVISION;

		pMsg->times		= pEntry->times;
		pMsg->pImsageSize	= 0;
		pMsg->PUID		= pEntry->PUID;
		pMsg->PPUID		= pEntry->PPUID;
		pMsg->PID		= (DWORD)pEntry->PID;
		pMsg->CPID		= (DWORD)pEntry->CPID;
		pMsg->RPID		= (WORD)0;
		pMsg->PPID		= (DWORD)pEntry->PPID;
		pMsg->CTID		= (DWORD)pEntry->TID;
		pMsg->TID		= (DWORD)pEntry->TID;
		pMsg->registry	= pEntry->registry;

		WORD		wOffset	= (WORD)(sizeof(Y_PROCESS));
		CopyStringData(pMsg, wOffset, &(pMsg->DevicePath), &pEntry->DevicePath);
		CopyStringData(pMsg, (wOffset += pMsg->DevicePath.wSize), &(pMsg->Command), &pEntry->Command);
		if( pOut )
			*pOut	= pMsg;
		else 
			CMemory::Free(pMsg);	
	}
}
void		CreateProcessMessage(
	YFilter::Message::SubType	subType,
	HANDLE						PID,
	HANDLE						PPID,
	HANDLE						CPID,
	PROCUID						*pPUID,
	PROCUID						*pPPUID,
	PUNICODE_STRING				pDevicePath,
	PUNICODE_STRING				pCommand,
	PKERNEL_USER_TIMES			pTimes,
	PY_PROCESS					*pOut
) {
	WORD	wSize	= sizeof(Y_PROCESS);
	wSize	+= GetStringDataSize(pDevicePath);
	wSize	+= GetStringDataSize(pCommand);

	PY_PROCESS		pMsg	= NULL;

	pMsg	= (PY_PROCESS)CMemory::Allocate(PagedPool, wSize, TAG_PROCESS);
	if( pMsg ) {
		RtlZeroMemory(pMsg, wSize);
		pMsg->mode		= YFilter::Message::Mode::Event;
		pMsg->category	= YFilter::Message::Category::Process;
		pMsg->subType	= subType;
		pMsg->wSize		= wSize;
		pMsg->wRevision	= Y_MESSAGE_REVISION;

		if( pTimes )
			pMsg->times	= *pTimes;
		else 
			GetProcessTimes(PID, &pMsg->times, false);
		pMsg->pImsageSize	= 0;
		pMsg->PUID		= *pPUID;
		pMsg->PPUID		= pPPUID? *pPPUID : 0;
		pMsg->PID		= (DWORD)PID;
		pMsg->CPID		= (DWORD)CPID;
		pMsg->RPID		= (WORD)0;
		pMsg->PPID		= (DWORD)PPID;
		pMsg->CTID		= (DWORD)PsGetCurrentThreadId();
		pMsg->TID		= pMsg->CTID;

		WORD		wOffset	= (WORD)(sizeof(Y_PROCESS));
		CopyStringData(pMsg, wOffset, &(pMsg->DevicePath), pDevicePath);
		CopyStringData(pMsg, (wOffset += pMsg->DevicePath.wSize), &(pMsg->Command), pCommand);
		if( pOut )
			*pOut	= pMsg;
		else 
			CMemory::Free(pMsg);	
	}
}

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
		PY_HEADER	pMsg	= (PY_HEADER)p->pItem;
		bool		bLog	= false;;

		if( YFilter::Message::Category::Thread == pMsg->category )
			bLog	= true;
		if( YFilter::Message::Mode::Event == p->nMode ) {
			if( YFilter::Message::Category::Registry == pMsg->category ) {
				//__dlog("%-32s PID:%d", __func__, pMsg->PID);
			}
			if (NT_FAILED(SendMessage(p->szCause, &Config()->client.event, p->pItem, p->nSize, true)) )
			{

			}
		}
		else {
			__dlog("unknown mode %d", p->nMode);
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
	Y_REPLY		reply = { 0, };
	ULONG		nSize = sizeof(Y_REPLY);
	NTSTATUS	status = SendMessage(pCause, p, pSendData, nSendDataSize, &reply, &nSize, bLog);
	if (NT_SUCCESS(status))
	{
		//__log("%s reply.bRet=%d", __FUNCTION__, reply.bRet);
		//__dlog("%-32s SendMessage() succeeded. ", __func__);
	}
	else {
		//__dlog("%-32s SendMessage() failed. STATUS=%08x", __func__, status);
	
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
	UNREFERENCED_PARAMETER(pCause);
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
		//__dlog("%-32s Config()=%p, Config()->pFilter=%p", __func__, Config(), Config()->pFilter);
		return status;
	}
	//__log("%s %ws %p %d", __FUNCTION__, p->szName, pSendData, nSendDataSize);
	pRecvData		= NULL;
	pnRecvDataSize	= NULL;
	CAutoReleaseSpinLock(__FUNCTION__, &p->lock, bLog);
	{
		LARGE_INTEGER	timeout;
		timeout.QuadPart = 3 * 1000 * 1000;
		__try
		{
			if (NULL == p->pPort)
			{
				//__dlog("%-32s p->pPort=%p", __FUNCTION__, p->pPort);
				__leave;
			}
			status = FltSendMessage(Config()->pFilter,
				&p->pPort,
				pSendData, nSendDataSize,
				pRecvData, pnRecvDataSize, NULL);

			STATUS_PORT_DISCONNECTED;
			if( NT_FAILED(status)) {
				__dlog("%-32s FltSendMessage(%d):%08x", __func__, nSendDataSize, status);
			}
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
			//__dlog("%-32s FltSendMessage(%s)=%x",
			//	__FUNCTION__, pCause ? pCause : "", status);
		}
		__finally
		{

		}
	}
	return status;
}