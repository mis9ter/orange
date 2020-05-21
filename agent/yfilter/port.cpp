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
		CAutoReleaseSpinLock(&Config()->client.lock);
		if (Config()->client.pPort)
		{
			//	[TODO] 이미 연결된 상태? 이전 연결을 끊어주는 것보다는.. 새로운 연결을 거부하고 싶다. 
			FltCloseClientPort(Config()->pFilter, &Config()->client.pPort);
		}
		FLT_ASSERT(Config()->client.pPort == NULL);
		Config()->client.pPort = ClientPort;
		Config()->client.hProcessId = PsGetCurrentProcessId();
#ifndef _WIN64
		__log("PsGetCurrentProcessId=%d", (int)(Config()->client.hProcessId));
#endif
	}
	return STATUS_SUCCESS;
}
bool	CommandIsConnected()
{
	CAutoReleaseSpinLock(&Config()->client.lock);
	return Config()->client.pPort ? true : false;
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
		CAutoReleaseSpinLock(&Config()->client.lock);
		FltCloseClientPort(Config()->pFilter, &Config()->client.pPort);
		Config()->client.hProcessId = NULL;
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
		CAutoReleaseSpinLock(&Config()->client.lock);
		if (Config()->client.pPort)
		{
			//	[TODO] 이미 연결된 상태? 이전 연결을 끊어주는 것보다는.. 새로운 연결을 거부하고 싶다. 
			FltCloseClientPort(Config()->pFilter, &Config()->client.pPort);
		}
		FLT_ASSERT(Config()->client.pPort == NULL);
		Config()->client.pPort = ClientPort;
		Config()->client.hProcessId = PsGetCurrentProcessId();
#ifndef _WIN64
		__log("PsGetCurrentProcessId=%d", (int)(Config()->client.hProcessId));
#endif
	}
	return STATUS_SUCCESS;
}
bool	EventIsConnected()
{
	CAutoReleaseSpinLock(&Config()->client.lock);
	return Config()->client.pPort ? true : false;
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
		CAutoReleaseSpinLock(&Config()->client.lock);
		FltCloseClientPort(Config()->pFilter, &Config()->client.pPort);
		Config()->client.hProcessId = NULL;
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

	return status;
}
NTSTATUS	SendMessage(IN LPVOID pData, IN size_t nDataSize)
{
	NTSTATUS	status;
	if (KeGetCurrentIrql() > APC_LEVEL)	return STATUS_UNSUCCESSFUL;

	LARGE_INTEGER	nTimeout;

	nTimeout.QuadPart = DRIVER_MESSAGE_TIMEOUT;
	CAutoReleaseSpinLock(&Config()->client.lock);
	status = FltSendMessage(Config()->pFilter, &Config()->client.pPort, pData, (ULONG)nDataSize, NULL, NULL, &nTimeout);
	return status;
}