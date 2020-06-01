#include "yfilter.h"


NTSTATUS	CommandConnected(
	_In_ PFLT_PORT ClientPort,
	_In_ PVOID ServerPortCookie,
	_In_reads_bytes_(SizeOfContext) PVOID ConnectionContext,
	_In_ ULONG SizeOfContext,
	_Flt_ConnectionCookie_Outptr_ PVOID *ConnectionCookie
)
{
	//	PASSIVE_LEVEL
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionCookie);

	__function_log;
	{
		CAutoReleaseSpinLock(&Config()->client.command.lock);
		if (Config()->client.command.pPort)
		{
			//	[TODO] 이미 연결된 상태? 이전 연결을 끊어주는 것보다는.. 새로운 연결을 거부하고 싶다. 
			FltCloseClientPort(Config()->pFilter, &Config()->client.command.pPort);
		}
		FLT_ASSERT(Config()->client.client.command.pPort == NULL);
		Config()->client.command.pPort = ClientPort;
		Config()->client.command.hProcess = PsGetCurrentProcessId();
#ifndef _WIN64
		__log("PsGetCurrentProcessId=%d", (int)(Config()->client.hProcessId));
#endif
	}
	return STATUS_SUCCESS;
}
bool	CommandIsConnected()
{
	CAutoReleaseSpinLock(&Config()->client.command.lock);
	return Config()->client.command.pPort ? true : false;
}
VOID	CommandDisconnected(_In_opt_ PVOID ConnectionCookie)
{
	//	PASSIVE_LEVEL
	PAGED_CODE();
	UNREFERENCED_PARAMETER(ConnectionCookie);
	__function_log;
	//
	//  Close our handle
	//
	{
		CAutoReleaseSpinLock(&Config()->client.command.lock);
		FltCloseClientPort(Config()->pFilter, &Config()->client.command.pPort);
		Config()->client.command.pPort	= NULL;
		Config()->client.event.pPort	= NULL;
	}
}
NTSTATUS	CommandMessage
(
	_In_ PVOID ConnectionCookie,
	_In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
	_In_ ULONG InputBufferSize,
	_Out_writes_bytes_to_opt_(OutputBufferSize, *ReturnOutputBufferLength) PVOID OutputBuffer,
	_In_ ULONG OutputBufferSize,
	_Out_ PULONG ReturnOutputBufferLength
)
/*++

Routine Description:

	This is called whenever a user mode application wishes to communicate
	with this minifilter.

Arguments:

	ConnectionCookie - unused

	OperationCode - An identifier describing what type of message this
		is.  These codes are defined by the MiniFilter.
	InputBuffer - A buffer containing input data, can be NULL if there
		is no input data.
	InputBufferSize - The size in bytes of the InputBuffer.
	OutputBuffer - A buffer provided by the application that originated
		the communication in which to store data to be returned to this
		application.
	OutputBufferSize - The size in bytes of the OutputBuffer.
	ReturnOutputBufferSize - The size in bytes of meaningful data
		returned in the OutputBuffer.

Return Value:

	Returns the status of processing the message.

--*/
{
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();
	UNREFERENCED_PARAMETER(ConnectionCookie);
	UNREFERENCED_PARAMETER(InputBuffer);
	UNREFERENCED_PARAMETER(InputBufferSize);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferSize);
	UNREFERENCED_PARAMETER(ReturnOutputBufferLength);

	__function_log;

	return status;
}
NTSTATUS	EventConnected(
	_In_ PFLT_PORT ClientPort,
	_In_ PVOID ServerPortCookie,
	_In_reads_bytes_(SizeOfContext) PVOID ConnectionContext,
	_In_ ULONG SizeOfContext,
	_Flt_ConnectionCookie_Outptr_ PVOID *ConnectionCookie
)
{
	//	PASSIVE_LEVEL
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionCookie);

	__function_log;
	{
		CAutoReleaseSpinLock(&Config()->client.event.lock);
		if (Config()->client.event.pPort)
		{
			//	[TODO] 이미 연결된 상태? 이전 연결을 끊어주는 것보다는.. 새로운 연결을 거부하고 싶다. 
			FltCloseClientPort(Config()->pFilter, &Config()->client.event.pPort);
		}
		FLT_ASSERT(Config()->client.pPort == NULL);
		Config()->client.event.pPort = ClientPort;
		Config()->client.event.hProcess = PsGetCurrentProcessId();
#ifndef _WIN64
		__log("PsGetCurrentProcessId=%d", (int)(Config()->client.hProcessId));
#endif
	}
	return STATUS_SUCCESS;
}
bool	EventIsConnected()
{
	CAutoReleaseSpinLock(&Config()->client.event.lock);
	return Config()->client.event.pPort ? true : false;
}
VOID	EventDisconnected(_In_opt_ PVOID ConnectionCookie)
{
	//	PASSIVE_LEVEL
	PAGED_CODE();
	UNREFERENCED_PARAMETER(ConnectionCookie);
	__function_log;
	//
	//  Close our handle
	//
	{
		CAutoReleaseSpinLock(&Config()->client.event.lock);
		FltCloseClientPort(Config()->pFilter, &Config()->client.event.pPort);
		Config()->client.event.pPort	= NULL;
		Config()->client.event.hProcess	= NULL;
	}
}
NTSTATUS	EventMessage
(
	_In_ PVOID ConnectionCookie,
	_In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
	_In_ ULONG InputBufferSize,
	_Out_writes_bytes_to_opt_(OutputBufferSize, *ReturnOutputBufferLength) PVOID OutputBuffer,
	_In_ ULONG OutputBufferSize,
	_Out_ PULONG ReturnOutputBufferLength
)
/*++

Routine Description:

	This is called whenever a user mode application wishes to communicate
	with this minifilter.

Arguments:

	ConnectionCookie - unused

	OperationCode - An identifier describing what type of message this
		is.  These codes are defined by the MiniFilter.
	InputBuffer - A buffer containing input data, can be NULL if there
		is no input data.
	InputBufferSize - The size in bytes of the InputBuffer.
	OutputBuffer - A buffer provided by the application that originated
		the communication in which to store data to be returned to this
		application.
	OutputBufferSize - The size in bytes of the OutputBuffer.
	ReturnOutputBufferSize - The size in bytes of meaningful data
		returned in the OutputBuffer.

Return Value:

	Returns the status of processing the message.

--*/
{
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();
	UNREFERENCED_PARAMETER(ConnectionCookie);
	UNREFERENCED_PARAMETER(InputBuffer);
	UNREFERENCED_PARAMETER(InputBufferSize);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferSize);
	UNREFERENCED_PARAMETER(ReturnOutputBufferLength);

	__function_log;

	return status;
}
NTSTATUS	SendMessage(
	IN PFLT_CLIENT_PORT p,
	IN PVOID pSendData, IN ULONG nSendDataSize,
	OUT PVOID pRecvData, OUT ULONG* pnRecvDataSize)
{
	NTSTATUS	status	= STATUS_UNSUCCESSFUL;
	if (KeGetCurrentIrql() > APC_LEVEL) {
		__dlog("%s ERROR KeGetCurrentIrql()=%d", __FUNCTION__, KeGetCurrentIrql());
		return status;
	}
	if (NULL == Config() || NULL == Config()->pFilter || NULL == p )
	{
		//	[TODO]	Config() 함수의 동기화는 누가 시켜주지?
		__log("%s ERROR Config()=%p, Config()->pFilter=%p, p=%p", 
			__FUNCTION__, Config(), Config()? Config()->pFilter : NULL, p);
		return status;
	}
	CAutoReleaseSpinLock(&p->lock);
	{
		LARGE_INTEGER	timeout;
		timeout.QuadPart	= DRIVER_MESSAGE_TIMEOUT;
		__try
		{
			if (NULL == p->pPort) 
			{
				__log("%s ERROR p->pPort=%p", __FUNCTION__, p->pPort);
				__leave;
			}
			status = FltSendMessage(Config()->pFilter,
				&p->pPort,
				pSendData, nSendDataSize,
				pRecvData, pnRecvDataSize,
				&timeout);
			if (STATUS_SUCCESS == status)
			{
				__leave;
			}
			__log("%s ERROR FltSendMessage()=%x", __FUNCTION__, status);
		}
		__finally
		{

		}
	}
	return status;
}
