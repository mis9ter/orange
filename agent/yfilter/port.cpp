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
		CAutoReleaseSpinLock(__FUNCTION__, &Config()->client.command.lock, true);
		if (Config()->client.command.pPort)
		{
			//	[TODO] 이미 연결된 상태? 이전 연결을 끊어주는 것보다는.. 새로운 연결을 거부하고 싶다. 
			FltCloseClientPort(Config()->pFilter, &Config()->client.command.pPort);
		}
		FLT_ASSERT(Config()->client.command.pPort == NULL);
		Config()->client.command.pPort	= ClientPort;
		Config()->client.command.PID	= PsGetCurrentProcessId();
		RtlStringCbCopyW(Config()->client.command.szName, sizeof(Config()->client.command.szName), 
			DRIVER_COMMAND_PORT);
#ifndef _WIN64
		__log("PsGetCurrentProcessId=%d", (int)(Config()->client.hProcessId));
#endif
	}
	return STATUS_SUCCESS;
}
bool	CommandIsConnected()
{
	CAutoReleaseSpinLock(__FUNCTION__, &Config()->client.command.lock, true);
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
		CAutoReleaseSpinLock(__FUNCTION__, &Config()->client.command.lock);
		FltCloseClientPort(Config()->pFilter, &Config()->client.command.pPort);
		Config()->client.command.pPort	= NULL;
		Config()->client.event.pPort	= NULL;
	}
}
NTSTATUS	StartDriver();
NTSTATUS	StopDriver();

void		GetProcessTableList() {
	ProcessTable()->List(NULL, 
		[](bool bSaved, PPROCESS_ENTRY pEntry, PVOID pContext) {
		UNREFERENCED_PARAMETER(bSaved);
		UNREFERENCED_PARAMETER(pContext);

		PY_PROCESS_MESSAGE	pMsg	= NULL;
		if( Config()->client.event.nConnected ) {
			//	DISPATCH_LEVEL 이라 GetProcessTimes 호출 불가.			
			//if( NT_FAILED(GetProcessTimes(pEntry->PID, &pEntry->times, false)) ) {
			//	__dlog("%-32s GetProcessTimes() failed.", "GetProcessTableList");
			//}
			CreateProcessMessage(
				YFilter::Message::SubType::ProcessStart2,
				pEntry,	&pMsg
			);
			if( pMsg ) {
				//__dlog("%-32s %p %d", __func__, pEntry->PUID, pEntry->PID);
				if( MessageThreadPool()->Push(__func__, pMsg->mode, pMsg->category, pMsg, pMsg->wDataSize + pMsg->wStringSize, false) ) {
					pMsg	= NULL;
				}
				else {
					CMemory::Free(pMsg);
				}
			}	
		}
		else {
			__log("%-32s not connected", __func__);
		}
	});
	MessageThreadPool()->Alert(YFilter::Message::Category::Process);
}

NTSTATUS		GetProcessBasicInfo(IN	HANDLE	PID, OUT PPROCESS_BASIC_INFORMATION p)
{
	if (NULL == Config())	return NULL;

	FN_ZwQueryInformationProcess	pZwQueryInformationProcess = Config()->pZwQueryInformationProcess;
	if (NULL == pZwQueryInformationProcess)	return NULL;
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	return NULL;

	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	HANDLE				hProcess = NULL;
	OBJECT_ATTRIBUTES	oa = { 0 };
	CLIENT_ID			cid = { 0 };
	ULONG				nNeedSize = 0;
	PUNICODE_STRING		pStr = NULL;

	__try
	{
		oa.Length = sizeof(OBJECT_ATTRIBUTES);
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
		cid.UniqueProcess = PID;

#ifndef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_INFORMATION (0x0400)
#endif
		status = ZwOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION, &oa, &cid);
		if (NT_FAILED(status)) {
			//	STATUS_INVALID_CID

			__log("%s %d ZwOpenProcess() failed. status=%08x", 
				__FUNCTION__, PID, status);
			__leave;
		}

		if( Config()->pSeLocateProcessImageName ) {
			PEPROCESS	process	= NULL;
			status = PsLookupProcessByProcessId(PID, &process);
			if( NT_SUCCESS(status)) {
				status	= Config()->pSeLocateProcessImageName(process, &pStr);
				if (NT_SUCCESS(status)) {
					__log("%-32s %wZ", __func__, pStr);
				}
				else {
					__log("%-32s SeLocateProcessImageName() failed. status=%x", __func__, status);
				}
				ObDereferenceObject(process);
			}
		}
		status = pZwQueryInformationProcess(hProcess, ProcessBasicInformation, p, sizeof(PROCESS_BASIC_INFORMATION), 
			&nNeedSize);
		__log("%-32s status=%d, nNeedSize=%d", __func__, status, nNeedSize);
		if (NT_SUCCESS(status)) {
			__log("%-32s PID  %d", __func__, p->UniqueProcessId);
			__log("%-32s PPID %d", __func__, p->InheritedFromUniqueProcessId);
			__log("%-32s PEB  %x", __func__, p->PebBaseAddress);
		}
		else {
			__log("%s %d ZwQueryInformationProcess() failed. status=%08x",
				__FUNCTION__, PID, status);
			__leave;
		}
	}
	__finally
	{
		if (hProcess)
		{
			ZwClose(hProcess);
			hProcess = NULL;
		}
		if (pStr) {
			ExFreePool(pStr);
			pStr	= NULL;
		}
	}
	return status;
}

NTSTATUS	OpenProcess(PHANDLE pHandle, ACCESS_MASK DesiredAccess, HANDLE PID, KPROCESSOR_MODE mode) {
	if (KernelMode != mode) {
		__try {
			ProbeForRead(pHandle, sizeof(PHANDLE), sizeof(UCHAR));
			ProbeForWrite(pHandle, sizeof(PHANDLE), sizeof(UCHAR));
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return GetExceptionCode();
		}
	}

	PEPROCESS		process;
	NTSTATUS		status;

	status	= PsLookupProcessByProcessId(PID, &process);
	if( NT_FAILED(status))	return status;
	
	HANDLE	p	= NULL;
	status	= ObOpenObjectByPointer(process, 0, NULL, DesiredAccess, *PsProcessType, KernelMode, &p);
	if (NT_SUCCESS(status)) {
		if( KernelMode == mode ) {
			*pHandle	= p;
		}
		else {
			__try {
				*pHandle	= p;
				__log("%-32s %d => %p", __func__, PID, p);
			}
			__except( EXCEPTION_EXECUTE_HANDLER) {
				status	= GetExceptionCode();
			}
		}
	}
	ObDereferenceObject(process);
	return STATUS_SUCCESS;
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

	bool	bRet = false;

	__try {
		if( InputBuffer && InputBufferSize ) {
			ProbeForRead(InputBuffer, InputBufferSize, sizeof(UCHAR));
			ProbeForWrite(InputBuffer, InputBufferSize, sizeof(UCHAR));
		}
		if( OutputBuffer && OutputBufferSize ) {
			ProbeForRead(OutputBuffer, OutputBufferSize, sizeof(UCHAR));
			ProbeForWrite(OutputBuffer, OutputBufferSize, sizeof(UCHAR));
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		__log("%-32s exception", __func__);
		return GetExceptionCode();
	}


	__log("%-32s InputBuffer:%p InputBufferSize:%d OutputBuffer:%p OutputBufferSize:%d ReturnOutputBufferLength:%p",
			__func__, InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, ReturnOutputBufferLength);


	KPROCESSOR_MODE accessMode	= ExGetPreviousMode();

	if (InputBuffer && InputBufferSize >= sizeof(Y_COMMAND) ) {
		PY_COMMAND	pCommand = (PY_COMMAND)InputBuffer;
		switch (pCommand->dwCommand) {
		case Y_COMMAND_OPENPROCESS:
			if (sizeof(COMMAND_OPENPROCESS) == pCommand->nSize) {
				
				PCOMMAND_OPENPROCESS	pp	= (PCOMMAND_OPENPROCESS)pCommand;
				__log("%s COMMAND_OPENPROCESS:%d", __FUNCTION__, pp->PID);
				pp->status	= OpenProcess(pp->pOpenHandle, pp->DesiredAccess, pp->PID, accessMode);

				PROCESS_BASIC_INFORMATION	info	= {0,};
				GetProcessBasicInfo(pp->PID, &info);
			}
			break;
		case Y_COMMAND_ENABLE_THREAD:
			if( sizeof(COMMAND_ENABLE) == pCommand->nSize ) {
				PCOMMAND_ENABLE	pp	= (PCOMMAND_ENABLE)pCommand;
				__log("%s YFILTER_COMMAND_ENABLE_THREAD:%d", __FUNCTION__, pp->bEnable);
				_InterlockedExchange((LONG*)&Config()->flag.nThread, pp->bEnable);
				pp->bRet	= true;
			}
			else {
				__log("%s YFILTER_COMMAND_ENABLE_THREAD: size(%d) mismatch", __func__, pCommand->nSize);

			}
			break;
			case Y_COMMAND_ENABLE_MODULE:
				if (sizeof(COMMAND_ENABLE) == pCommand->nSize) {
					PCOMMAND_ENABLE	pp = (PCOMMAND_ENABLE)pCommand;
					__log("%s YFILTER_COMMAND_ENABLE_MODULE:%d", __FUNCTION__, pp->bEnable);
					_InterlockedExchange((LONG *)&Config()->flag.nModule, pp->bEnable);
					pp->bRet = true;
				}
				break;

			case Y_COMMAND_ENABLE_FILE:
				if (sizeof(COMMAND_ENABLE) == pCommand->nSize) {
					PCOMMAND_ENABLE	pp = (PCOMMAND_ENABLE)pCommand;
					__log("%s YFILTER_COMMAND_ENABLE_FILE:%d", __FUNCTION__, pp->bEnable);
					_InterlockedExchange((LONG*)&Config()->flag.nFile, pp->bEnable);
					pp->bRet = true;
				}
				break;

			case Y_COMMAND_START:
				__log("%s YFILTER_COMMAND_START", __FUNCTION__);
				//if( NT_SUCCESS(StartDriver()) )	
				bRet	= true;
				//ExInterlockedExchangeUlong(&Config()->nRun, 1, &Config()->lock);
				_InterlockedExchange((LONG *)&Config()->flag.nRun, 1);
				_InterlockedExchange((LONG *)&Config()->flag.nProcess, 1);
				AddProcessToTable2(__func__, false, PsGetCurrentProcessId(), NULL, NULL, NULL, false, NULL, NULL);
				break;
			case Y_COMMAND_STOP:
				__log("%s YFILTER_COMMAND_STOP", __FUNCTION__);
				//if (NT_SUCCESS(StopDriver()))	
				bRet = true;
				_InterlockedExchange((LONG *)&Config()->flag.nRun, 0);
				break;

			case Y_COMMAND_GET_PROCESS_LIST:
				__log("%s Y_COMMAND_GET_PROCESS_LIST", __FUNCTION__);
				GetProcessTableList();
				break;
		}
	}
	if (OutputBuffer && OutputBufferSize) {
		//__log("%s OutputBuffer=%p, OutputBufferSize=%d", __FUNCTION__, OutputBuffer, OutputBufferSize);
		if( sizeof(Y_REPLY) == OutputBufferSize )
			((PY_REPLY)OutputBuffer)->bRet = bRet;
	}
	if (ReturnOutputBufferLength)
		*ReturnOutputBufferLength = OutputBufferSize;
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

	{
		CAutoReleaseSpinLock(__FUNCTION__, &Config()->client.event.lock, true);
		if (Config()->client.event.pPort)
		{
			//	[TODO] 이미 연결된 상태? 이전 연결을 끊어주는 것보다는.. 새로운 연결을 거부하고 싶다. 
			FltCloseClientPort(Config()->pFilter, &Config()->client.event.pPort);
		}
		FLT_ASSERT(Config()->client.event.pPort == NULL);
		Config()->client.event.pPort	= ClientPort;
		Config()->client.event.PID		= PsGetCurrentProcessId();
		Config()->client.event.nConnected++;
		RtlStringCbCopyW(Config()->client.event.szName, sizeof(Config()->client.event.szName),
			DRIVER_EVENT_PORT);
#ifndef _WIN64
		__log("PsGetCurrentProcessId=%d", (int)(Config()->client.hProcessId));
#endif
	}
	__log("%-32s %d", __func__, Config()->client.event.nConnected);
	return STATUS_SUCCESS;
}
bool	EventIsConnected()
{
	CAutoReleaseSpinLock(__FUNCTION__, &Config()->client.event.lock, true);
	return Config()->client.event.pPort ? true : false;
}
VOID	EventDisconnected(_In_opt_ PVOID ConnectionCookie)
{
	//	PASSIVE_LEVEL
	PAGED_CODE();
	UNREFERENCED_PARAMETER(ConnectionCookie);
	//
	//  Close our handle
	//
	{
		CAutoReleaseSpinLock(__FUNCTION__, &Config()->client.event.lock, true);
		FltCloseClientPort(Config()->pFilter, &Config()->client.event.pPort);
		Config()->client.event.pPort	= NULL;
		Config()->client.event.PID		= NULL;
		Config()->client.event.nConnected--;
	}
	__log("%-32s %d", __func__, Config()->client.event.nConnected);
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