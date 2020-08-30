/*++

Module Name:

    .c

Abstract:

    This is the main module of the  miniFilter driver.

Environment:

    Kernel mode

--*/
#include "yfilter.h"

ULONG_PTR		OperationStatusCtx = 1;
ULONG			gTraceFlags = 0;
CThreadPool		g_messageThreadPool;
CThreadPool *	MessageThreadPool()
{
	return &g_messageThreadPool;
}
/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/
void		LegacyDriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
	UNREFERENCED_PARAMETER(pDriverObject);
	DriverUnload(0);
	__log("%-20s", __FUNCTION__);
}
NTSTATUS	CreateFilterPort(
	PFLT_FILTER				pFilter,
	PFLT_PORT				*ppPort, 
	PCWSTR					pName,
	PFLT_CONNECT_NOTIFY		ConnectNotifyCallback,
	PFLT_DISCONNECT_NOTIFY	DisconnectNotifyCallback,
	PFLT_MESSAGE_NOTIFY		MessageNotifyCallback

)
{
	NTSTATUS		status	= STATUS_UNSUCCESSFUL;

	UNICODE_STRING	portName;
	CWSTR			portNameStr(pName);

	__try
	{
		if (portNameStr.CbSize())
		{
			RtlInitUnicodeString(&portName, portNameStr);
		}
		else
		{
			portNameStr.Clear();
			__leave;
		}
		PSECURITY_DESCRIPTOR	sd;
		OBJECT_ATTRIBUTES		oa;

		status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);

		if (NT_FAILED(status))	__leave;



		InitializeObjectAttributes(&oa,
			&portName,
			OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
			NULL,
			sd);
		status = FltCreateCommunicationPort(pFilter, ppPort, 
			&oa,
			NULL,
			ConnectNotifyCallback,
			DisconnectNotifyCallback,
			MessageNotifyCallback, 
			1);
		FltFreeSecurityDescriptor(sd);
		if (NT_FAILED(status))
		{
			__log("FltCreateCommunicationPort() failed. status=%08x", status);
			__leave;
		}
		__log("%s %ws", __FUNCTION__, pName);
	}
	__finally
	{

	}
	return status;
}
void		DestroyFilterPort(PFLT_PORT	*ppPort, PCWSTR pName)
{
	if (*ppPort)
	{
		__log("%s %ws", __FUNCTION__, pName);
		FltCloseCommunicationPort(*ppPort);
		*ppPort = NULL;
	}
}
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT pDriverObject,
    _In_ PUNICODE_STRING pRegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for this miniFilter driver.  This
    registers with FltMgr and initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Routine can return non success error codes.

--*/
{
	__function_log;
	//__dlog("built at %s %s", __DATE__, __TIME__);

    NTSTATUS status	= STATUS_UNSUCCESSFUL;
    UNREFERENCED_PARAMETER( pRegistryPath );

	__try
	{
		MessageThreadPool()->Create(2);
		SetMessageThreadCallback(NULL);
		if (false == CreateConfig(pRegistryPath)) {
			status = STATUS_FAILED_DRIVER_ENTRY;
			__leave;
		}
		//pDriverObject->DriverUnload = LegacyDriverUnload;
		status	= IoCreateDevice(pDriverObject, 0, &Config()->deviceName,
			FILE_DEVICE_UNKNOWN, 0, TRUE, &Config()->pDeviceObject);
		if (!NT_SUCCESS(status))
		{
			__log("IoCreateDevice() failed. status=%08x", status);
			__leave;
		}
		status = IoCreateSymbolicLink(&Config()->dosDeviceName, &Config()->deviceName);
		if (!NT_SUCCESS(status))
		{
			__log("IoCreateSymbolicLink() failed. status=%08x", status);
			__leave;
		}
		status = FltRegisterFilter(pDriverObject,
			&FilterRegistration,
			&Config()->pFilter);
		FLT_ASSERT(NT_SUCCESS(status));
		if (NT_FAILED(status))
		{
			__log("FltRegisterFilter() failed. status=%08x", status);
			__leave;
		}
		RtlStringCbCopyW(Config()->server.command.szName, sizeof(Config()->server.command.szName), 
			DRIVER_COMMAND_PORT);
		status	= CreateFilterPort(Config()->pFilter, &Config()->server.command.pPort,
			Config()->server.command.szName,
			CommandConnected, CommandDisconnected, CommandMessage);
		if (NT_FAILED(status)) {
			__log("CreateFilterPort() failed. name=%ws, status=%08x", DRIVER_COMMAND_PORT, status);
			__leave;
		}
		RtlStringCbCopyW(Config()->server.event.szName, sizeof(Config()->server.event.szName),
			DRIVER_EVENT_PORT);
		status = CreateFilterPort(Config()->pFilter, &Config()->server.event.pPort,
			DRIVER_EVENT_PORT,
			EventConnected, EventDisconnected, EventMessage);
		if (NT_FAILED(status)) {
			__log("CreateFilterPort() failed. name=%ws status=%08x", DRIVER_EVENT_PORT, status);
			__leave;
		}
		//	FILE  보호를 위한 콜백 받기 시작.
		status = FltStartFiltering(Config()->pFilter);
		if (NT_FAILED(status))
		{
			__log("FltStartFiltering() failed. status=%08x", status);
			__leave;
		}
		if (StartThreadFilter()) {
			__log("thread filtering ..");
		}
		if (StartModuleFilter())
		{
			__log("module filtering ..");
		}
		if (StartProcessFilter())
		{
			__log("process filtering ..");
		}
		status	= STATUS_SUCCESS;
	}
	__finally
	{
		if (NT_SUCCESS(status))
		{
			__dlog("SUCESS");
		}
		else
		{
			DriverUnload((FLT_FILTER_UNLOAD_FLAGS)-1);
		}
		__dlog("%-20s status=%d", __FUNCTION__, status);
	}
    return status;
}

NTSTATUS
DriverUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
    when the minifilter is about to be unloaded. We can fail this unload
    request if this is not a mandatory unload indicated by the Flags
    parameter.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
	__function_log;
	//__dlog("built at %s %s", __DATE__, __TIME__);

    UNREFERENCED_PARAMETER( Flags );
    PAGED_CODE();

	if (Config())
	{
		StopModuleFilter();
		StopProcessFilter();
		StopThreadFilter();
		if (Config()->pDeviceObject)
		{
			IoDeleteSymbolicLink(&Config()->dosDeviceName);
			IoDeleteDevice(Config()->pDeviceObject);
			Config()->pDeviceObject = NULL;
		}
		if (Config()->pFilter)
		{
			CAutoReleaseSpinLock(__FUNCTION__, &Config()->client.event.lock);
			CAutoReleaseSpinLock(__FUNCTION__, &Config()->client.command.lock);

			DestroyFilterPort(&Config()->server.command.pPort, Config()->server.command.szName);
			DestroyFilterPort(&Config()->server.event.pPort, Config()->server.event.szName);
			//	FltCloseCommunicationPort를 하지 않고 FltUnregisterFilter를 하면 어떤일이 벌어지게?
			//	hang이 걸리더라. 
			//	까칠이. 그래 커널단엔 배려란 없으면서 멋지구나. 
			//	xserver/xagent는 플러그인 때문에 해제시 행 걸리면 사람들이 지랄지랄해서 
			//	내가 고치는데.
			FltUnregisterFilter(Config()->pFilter);
			Config()->pFilter = NULL;
		}
		DestroyConfig();
		MessageThreadPool()->Destroy();
		if ((FLT_FILTER_UNLOAD_FLAGS)-1 != Flags)
		{
			//	Load 실패로 Unload가 수행되는 경우엔 로그 출력하지 않음. 
			__log("BYE BYE");
		}
	}
    return STATUS_SUCCESS;
}
