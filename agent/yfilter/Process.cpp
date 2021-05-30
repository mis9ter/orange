#include "yfilter.h"
#include <wdm.h>
#include "md5.h"

#define	USE_OBJECT_CALLBACK	0

static	CProcessTable	g_process;

CProcessTable *	ProcessTable()
{
	return &g_process;
}
void			CreateProcessTable()
{
	g_process.Initialize();
}
void			DestroyProcessTable()
{
	g_process.Destroy();
}
bool			IsRegisteredProcess(IN HANDLE h)
{
	if( false == g_process.IsInitialized() )	return false;
	return g_process.IsExisting(h);
}
bool			RegisterProcess(IN HANDLE h)
{
	return g_process.Add(false, h, NULL, NULL, NULL, NULL, NULL);
}
bool			DeregisterProcess(IN HANDLE h)
{
	return g_process.Remove(h, true, NULL, NULL);
}
NTSTATUS		KillProcess(HANDLE pid)
{
	if (pid <= (HANDLE)4)					return STATUS_INVALID_PARAMETER;
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	return STATUS_UNSUCCESSFUL;
	if (NULL == Config()->pZwTerminateProcess)	return STATUS_BAD_FUNCTION_TABLE;

	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	HANDLE		hProcess = NULL;
	__try
	{
		OBJECT_ATTRIBUTES oa = { 0 };
		oa.Length = sizeof(OBJECT_ATTRIBUTES);
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

		CLIENT_ID cid = { 0 };
		cid.UniqueProcess = pid;
		status = ZwOpenProcess(&hProcess, PROCESS_TERMINATE, &oa, &cid);
		if (NT_FAILED(status))	__leave;
		status = Config()->pZwTerminateProcess(hProcess, STATUS_SUCCESS);
	}
	__finally
	{
		if (hProcess)	ZwClose(hProcess);
	}
	return status;
}
static	bool	IsSkip(
	IN HANDLE							currentPID,
	IN HANDLE							targetPID,
	IN POB_PRE_OPERATION_INFORMATION	pOperationInformation
)
{
	bool	bRet	= true;
	__try
	{
		if ((HANDLE)4 == currentPID || currentPID == targetPID)
			__leave;
		if (KernelMode == ExGetPreviousMode())	
			__leave;
		if (!MmIsAddressValid(pOperationInformation->Object))	
			__leave;
		if (OB_OPERATION_HANDLE_CREATE != pOperationInformation->Operation)	
			__leave;
		bRet	= false;
	}
	__finally
	{

	}
	return bRet;
}

OB_PREOP_CALLBACK_STATUS	ProcessObjectPreCallback(
	PVOID							pRegistrationContext,
	POB_PRE_OPERATION_INFORMATION	pOperationInformation
)
{
	//	This routine is called at PASSIVE_LEVEL in an arbitrary thread context 
	//	with normal kernel APCs disabled. 
	//	Special kernel APCs are not disabled. For more information about APCs
	UNREFERENCED_PARAMETER(pRegistrationContext);
	UNREFERENCED_PARAMETER(pOperationInformation);
	__try
	{
		HANDLE	currentPID = PsGetCurrentProcessId();
		HANDLE	targetPID = PsGetProcessId((PEPROCESS)pOperationInformation->Object);

		if( IsSkip(currentPID, targetPID, pOperationInformation) )		__leave;

		__dlog("%s CPID=%d TPID=%d", __func__, currentPID, targetPID);
	}
	__finally
	{

	}
	return OB_PREOP_SUCCESS;
}
void	ProcessObjectPostCallback(
	PVOID							pRegistrationContext,
	POB_POST_OPERATION_INFORMATION	pOperationInformation
)
{
	//	This routine is called at PASSIVE_LEVEL in an arbitrary thread context 
	//	with normal kernel APCs disabled. 
	//	Special kernel APCs are not disabled. For more information about APCs
	UNREFERENCED_PARAMETER(pRegistrationContext);
	UNREFERENCED_PARAMETER(pOperationInformation);

	HANDLE	currentPID = PsGetCurrentProcessId();
	HANDLE	targetPID = PsGetProcessId((PEPROCESS)pOperationInformation->Object);

	__dlog("%s CPID=%d TPID=%d", __func__, currentPID, targetPID);

}
void	ThreadObjectPostCallback(
	PVOID							pRegistrationContext,
	POB_POST_OPERATION_INFORMATION	pOperationInformation
)
{
	//	This routine is called at PASSIVE_LEVEL in an arbitrary thread context 
	//	with normal kernel APCs disabled. 
	//	Special kernel APCs are not disabled. For more information about APCs
	UNREFERENCED_PARAMETER(pRegistrationContext);
	UNREFERENCED_PARAMETER(pOperationInformation);
}
OB_PREOP_CALLBACK_STATUS
ThreadObjectPreCallback(
	PVOID							pRegistrationContext,
	POB_PRE_OPERATION_INFORMATION	pOperationInformation
)
{
	UNREFERENCED_PARAMETER(pRegistrationContext);
	UNREFERENCED_PARAMETER(pOperationInformation);

	__try
	{
		HANDLE	currentPID = PsGetCurrentProcessId();
		HANDLE	targetPID = PsGetThreadProcessId((PETHREAD)pOperationInformation->Object);
		if (IsSkip(currentPID, targetPID, pOperationInformation))	__leave;

	
	}
	__finally
	{

	}
	return OB_PREOP_SUCCESS;
}
wchar_t *	wcsistr(const wchar_t *String, const wchar_t *Pattern)
{
	wchar_t *pptr, *sptr, *start;

	for (start = (wchar_t *)String; *start != NULL; ++start)
	{
		while (((NULL != *start) &&
			(RtlUpcaseUnicodeChar(*start) != RtlUpcaseUnicodeChar(*Pattern))))
		{
			++start;
		}
		if (NULL == *start)
			return NULL;
		pptr = (wchar_t *)Pattern;
		sptr = (wchar_t *)start;
		while (RtlUpcaseUnicodeChar(*sptr) == RtlUpcaseUnicodeChar(*pptr))
		{
			sptr++;
			pptr++;
			if (NULL == *pptr)
				return (start);
		}
	}
	return NULL;
}
#define GINSTALLER_SIGNATURE	L"\\$GNPACK_"
inline	bool	IsGenianInstaller(IN PCWSTR pProcPath)
{
	if( pProcPath )
	{
		//	[TODO] 자체 보호로 인해 업데이트, 설치가 되지 않는다면?
		//	\??\C:\Users\TigerJ\APPDATA\LOCAL\TEMP\$GNPACK_3B99496A$\Installer.exe
		//	이와 같은 경로로 실행되면 우리 회사의 설치본이 동작한 것이므로 
		//	자동으로 허용 목록에 넣어준다. 
		//	원래 구상한 방법은 Installer.exe를 수정, Installer.exe가 스스로 자체보호에 
		//	등록하는 것이나 과도기적 단계에서 Installer.exe가 변경되지 않은 경우를 
		//	고려해서 한시적으로 이와 같은 홀을 두도록 한다.
		if (wcsistr(pProcPath, GINSTALLER_SIGNATURE))
		{
			__log("%s GENIAN_INSTALLER %ws", __FUNCTION__, pProcPath);
			return true;
		}
	}
	return false;
}
PCWSTR			Uuid2String(IN UUID* p, OUT PWSTR pStr, IN ULONG nStrSize)
{
	/*
	typedef struct _GUID {
		unsigned long  Data1;
		unsigned short Data2;
		unsigned short Data3;
		unsigned char  Data4[8];
	} GUID;
	*/
	if( NULL == p )	return NULL;
	RtlStringCbPrintfW(pStr, nStrSize, 
		L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		p->Data1, p->Data2, p->Data3, 
		p->Data4[0], p->Data4[1], p->Data4[2], p->Data4[3], 
		p->Data4[4], p->Data4[5], p->Data4[6], p->Data4[7]);
	return pStr;
}
NTSTATUS		AddEarlyModule(HANDLE hPID)
{
	NTSTATUS	status	= STATUS_UNSUCCESSFUL;

	__log("%s %d", __FUNCTION__, hPID);
	return status;
}
NTSTATUS		AddEarlyProcess(HANDLE hPID)
{
	HANDLE			hPPID = GetParentProcessId(hPID);
	PUNICODE_STRING	pImageFileName = NULL;
	NTSTATUS		status			= STATUS_UNSUCCESSFUL;
	KERNEL_USER_TIMES	times;
	UUID			ProcGuid;
	PUNICODE_STRING	pCmdLine = NULL;
	PYFILTER_MESSAGE	pMsg = NULL;

	bool			bRun	= false;
	if( false == bRun )	return status;

	if( NULL == hPPID )	return status;
	AddEarlyProcess(hPPID);
	UNREFERENCED_PARAMETER(ProcGuid);
	__try {
		if (NT_FAILED(status = GetProcessImagePathByProcessId(hPID, &pImageFileName)))	{
			__log("%s %d GetProcessImagePathByProcessId() failed.", __FUNCTION__, hPID);
			__leave;
		}
		if (NT_FAILED(GetProcessTimes(hPID, &times)))	{
			__log("%s %d GetProcessTimes() failed.", __FUNCTION__, hPID);
			__leave;
		}
		if (NT_FAILED(GetProcessInfo(hPID, ProcessCommandLineInformation, &pCmdLine))) {
			__log("%s %d GetProcessInfo() failed.", __FUNCTION__, hPID);
			__leave;
		}
		pMsg = (PYFILTER_MESSAGE)CMemory::Allocate(PagedPool, sizeof(YFILTER_MESSAGE), TAG_PROCESS);
		if (pMsg) 
		{
			PROCUID		ProcUID;
			RtlZeroMemory(pMsg, sizeof(YFILTER_MESSAGE));
			RtlStringCbCopyUnicodeString(pMsg->data.szPath, sizeof(pMsg->data.szPath), pImageFileName);
			if( NT_SUCCESS(GetProcGuid(true, hPID, hPPID,
				pImageFileName, &times.CreateTime, &pMsg->data.ProcGuid, &ProcUID)) ) 
			{
				pMsg->data.ProcUID	= ProcUID;
				PUNICODE_STRING	pParentImageFileName = NULL;
				if (NT_SUCCESS(GetProcessImagePathByProcessId(hPPID, &pParentImageFileName)))
				{
					KERNEL_USER_TIMES	ptimes;
					PROCUID				PProcUID;
					if (NT_SUCCESS(GetProcessTimes(hPPID, &ptimes))) {
						if (NT_SUCCESS(GetProcGuid(true, hPPID, GetParentProcessId(hPPID),
							pParentImageFileName, &ptimes.CreateTime, &pMsg->data.PProcGuid, &PProcUID))) {
							pMsg->data.PProcUID	= PProcUID;
						}
						else {
							__log("%s PProcGuid - failed.", __FUNCTION__);
							__log("  PPID:%d", hPPID);
							__log("  GetProcGuid() failed.");
						}
					}
					else {
						__log("%s PProcGuid - failed.", __FUNCTION__);
						__log("  PPID:%d", hPPID);
						__log("  GetProcessTimes() failed.");
					}
					CMemory::Free(pParentImageFileName);
				}
				else {
					//	부모 프로세스가 종료된 상태라면 당연히 여기로.
					//__log("%s PProcGuid - failed.", __FUNCTION__);
					//__log("  PPID:%d", hPPID);
					//__log("  GetProcessImagePathByProcessId() failed.");
				}										
			}
			else {
				//	ProcGuid 생성 실패 -> 저장할 필요 없습니다.
				__leave;
			}
			pMsg->data.times.CreateTime = times.CreateTime;
			ProcessTable()->Add(true, hPID, hPPID,
				&pMsg->data.ProcGuid, pMsg->data.ProcUID, pImageFileName, pCmdLine);
			pMsg->header.mode	= YFilter::Message::Mode::Event;
			pMsg->header.category = YFilter::Message::Category::Process;
			pMsg->header.size = sizeof(YFILTER_MESSAGE);

			pMsg->data.subType = YFilter::Message::SubType::ProcessStart2;
			pMsg->data.bCreationSaved	= true;
			pMsg->data.PID				= (DWORD)hPID;
			pMsg->data.PPID				= (DWORD)hPPID;
			RtlStringCbCopyUnicodeString(pMsg->data.szPath, sizeof(pMsg->data.szPath), pImageFileName);
			RtlStringCbCopyUnicodeString(pMsg->data.szCommand, sizeof(pMsg->data.szCommand), pCmdLine);
			if (MessageThreadPool()->Push(__FUNCTION__,
				YFilter::Message::Mode::Event,
				YFilter::Message::Category::Process,
				pMsg, sizeof(YFILTER_MESSAGE), false))
			{
				//	pMsg는 SendMessage 성공 후 해제될 것이다. 
				MessageThreadPool()->Alert(YFilter::Message::Category::Process);
				pMsg	= NULL;
			}
			else {
				CMemory::Free(pMsg);
				pMsg	= NULL;
			}
			//	이전 프로세스에 대한 모듈 정보도 올린다.
			//AddEarlyModule(hPID);
		}
	}
	__finally {
		if( pImageFileName )	CMemory::Free(pImageFileName);
		if( pCmdLine )			CMemory::Free(pCmdLine);
		if( pMsg )				CMemory::Free(pMsg);
	}
	return status;
}
NTSTATUS		SendPreCreatedProcess(HANDLE hPID)
{
	HANDLE			hPPID = GetParentProcessId(hPID);
	PUNICODE_STRING	pImageFileName = NULL;
	NTSTATUS		status = STATUS_UNSUCCESSFUL;
	KERNEL_USER_TIMES	times;
	UUID			ProcGuid;
	PUNICODE_STRING	pCmdLine = NULL;
	PYFILTER_MESSAGE	pMsg = NULL;

	bool			bRun = false;
	if (false == bRun)	return status;

	if (NULL == hPPID)	return status;
	AddEarlyProcess(hPPID);
	UNREFERENCED_PARAMETER(ProcGuid);
	__try {
		if (NT_FAILED(status = GetProcessImagePathByProcessId(hPID, &pImageFileName))) {
			__log("%s %d GetProcessImagePathByProcessId() failed.", __FUNCTION__, hPID);
			__leave;
		}
		if (NT_FAILED(GetProcessTimes(hPID, &times))) {
			__log("%s %d GetProcessTimes() failed.", __FUNCTION__, hPID);
			__leave;
		}
		if (NT_FAILED(GetProcessInfo(hPID, ProcessCommandLineInformation, &pCmdLine))) {
			__log("%s %d GetProcessInfo() failed.", __FUNCTION__, hPID);
			__leave;
		}
		pMsg = (PYFILTER_MESSAGE)CMemory::Allocate(NonPagedPoolNx, sizeof(YFILTER_MESSAGE), TAG_PROCESS);
		if (pMsg)
		{
			PROCUID		ProcUID;
			RtlZeroMemory(pMsg, sizeof(YFILTER_MESSAGE));
			RtlStringCbCopyUnicodeString(pMsg->data.szPath, sizeof(pMsg->data.szPath), pImageFileName);
			if (NT_SUCCESS(GetProcGuid(true, hPID, hPPID,
				pImageFileName, &times.CreateTime, &pMsg->data.ProcGuid, &ProcUID)))
			{
				pMsg->data.ProcUID	= ProcUID;
				PUNICODE_STRING	pParentImageFileName = NULL;
				if (NT_SUCCESS(GetProcessImagePathByProcessId(hPPID, &pParentImageFileName)))
				{
					KERNEL_USER_TIMES	ptimes;
					PROCUID				PProcUID;
					if (NT_SUCCESS(GetProcessTimes(hPPID, &ptimes))) {
						if (NT_SUCCESS(GetProcGuid(true, hPPID, GetParentProcessId(hPPID),
							pParentImageFileName, &ptimes.CreateTime, &pMsg->data.PProcGuid, &PProcUID))) {
							pMsg->data.PProcUID	= PProcUID;
						}
						else {
							__log("%s PProcGuid - failed.", __FUNCTION__);
							__log("  PPID:%d", hPPID);
							__log("  GetProcGuid() failed.");
						}
					}
					else {
						__log("%s PProcGuid - failed.", __FUNCTION__);
						__log("  PPID:%d", hPPID);
						__log("  GetProcessTimes() failed.");
					}
					CMemory::Free(pParentImageFileName);
				}
				else {
					//	부모 프로세스가 종료된 상태라면 당연히 여기로.
					//__log("%s PProcGuid - failed.", __FUNCTION__);
					//__log("  PPID:%d", hPPID);
					//__log("  GetProcessImagePathByProcessId() failed.");
				}
			}
			else {
				//	ProcGuid 생성 실패 -> 저장할 필요 없습니다.
				__leave;
			}
			pMsg->data.times.CreateTime = times.CreateTime;
			ProcessTable()->Add(true, hPID, hPPID,
				&pMsg->data.ProcGuid, pMsg->data.ProcUID, pImageFileName, pCmdLine);
			pMsg->header.mode = YFilter::Message::Mode::Event;
			pMsg->header.category = YFilter::Message::Category::Process;
			pMsg->header.size = sizeof(YFILTER_MESSAGE);

			pMsg->data.subType = YFilter::Message::SubType::ProcessStart2;
			pMsg->data.bCreationSaved = true;
			pMsg->data.PID = (DWORD)hPID;
			pMsg->data.PPID = (DWORD)hPPID;
			RtlStringCbCopyUnicodeString(pMsg->data.szPath, sizeof(pMsg->data.szPath), pImageFileName);
			RtlStringCbCopyUnicodeString(pMsg->data.szCommand, sizeof(pMsg->data.szCommand), pCmdLine);
			if (MessageThreadPool()->Push(__FUNCTION__,
				YFilter::Message::Mode::Event,
				YFilter::Message::Category::Process,
				pMsg, sizeof(YFILTER_MESSAGE), false))
			{
				//	pMsg는 SendMessage 성공 후 해제될 것이다. 
				MessageThreadPool()->Alert(YFilter::Message::Category::Process);
				pMsg = NULL;
			}
			else {
				CMemory::Free(pMsg);
				pMsg = NULL;
			}
			//	이전 프로세스에 대한 모듈 정보도 올린다.
			//AddEarlyModule(hPID);
		}
	}
	__finally {
		if (pImageFileName)	CMemory::Free(pImageFileName);
		if (pCmdLine)			CMemory::Free(pCmdLine);
		if (pMsg)				CMemory::Free(pMsg);
	}
	return status;
}
void	__stdcall	ProcessNotifyCallbackRoutineEx(
	_Inout_		PEPROCESS Process,
	_In_		HANDLE ProcessId,
	_Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
	UNREFERENCED_PARAMETER(Process);
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(CreateInfo);

	if( KeGetCurrentIrql() > PASSIVE_LEVEL )
		__function_log;

	CFltObjectReference		filter(Config()->pFilter);
	//FILTER_REPLY_DATA		reply;
	//ULONG					nReplySize = 0;

	if (filter.Reference())
	{
		KERNEL_USER_TIMES	times;
		GetProcessTimes(ProcessId, &times);

		if (CreateInfo)
		{
			CreateInfo->FileOpenNameAvailable;	
			//	[TODO] 이거 뭐지?
			//	A Boolean value that specifies whether 
			//	the ImageFileName member contains the exact file name 
			//	that is used to open the process executable file.
			CUnicodeString		procPath(CreateInfo->ImageFileName, PagedPool);
			CUnicodeString		command(CreateInfo->CommandLine, PagedPool);

			if( CreateInfo->FileOpenNameAvailable )
			{
				PYFILTER_MESSAGE	pMsg = NULL;
				pMsg = (PYFILTER_MESSAGE)CMemory::Allocate(NonPagedPoolNx, sizeof(YFILTER_MESSAGE), TAG_PROCESS);
				if (pMsg) {
					RtlZeroMemory(pMsg, sizeof(YFILTER_MESSAGE));
					PUNICODE_STRING	pImageFileName = NULL;
					if (NT_SUCCESS(GetProcessImagePathByProcessId(ProcessId, &pImageFileName)))
					{
						PROCUID		ProcUID;
						RtlStringCbCopyUnicodeString(pMsg->data.szPath, sizeof(pMsg->data.szPath), pImageFileName);
						GetProcGuid(true, ProcessId, GetParentProcessId(ProcessId), 
							//CreateInfo->ParentProcessId, 
							pImageFileName, &times.CreateTime, &pMsg->data.ProcGuid, &ProcUID);
						pMsg->data.ProcUID	= ProcUID;
						
						PUNICODE_STRING	pParentImageFileName = NULL;
						if (NT_SUCCESS(GetProcessImagePathByProcessId(CreateInfo->ParentProcessId, &pParentImageFileName)))
						{
							KERNEL_USER_TIMES	ptimes;
							PROCUID				PProcUID;
							if (NT_SUCCESS(GetProcessTimes(CreateInfo->ParentProcessId, &ptimes))) {
								if (NT_SUCCESS(GetProcGuid(true, CreateInfo->ParentProcessId, 
									GetParentProcessId(CreateInfo->ParentProcessId),
									pParentImageFileName, &ptimes.CreateTime,
									&pMsg->data.PProcGuid, &PProcUID)) ) {
									pMsg->data.PProcUID	= PProcUID;
								}
								else {
									__log("%s PProcGuid - failed.", __FUNCTION__);
									__log("  PPID:%d", CreateInfo->ParentProcessId);
									__log("  GetProcGuid() failed.");
								}
							}
							else {
								__log("%s PProcGuid - failed.", __FUNCTION__);
								__log("  PPID:%d", CreateInfo->ParentProcessId);
								__log("  GetProcessTimes() failed.");
							}
							CMemory::Free(pParentImageFileName);
						}
						else {
							__log("%s PProcGuid - failed.", __FUNCTION__);
							__log("  PPID:%d", CreateInfo->ParentProcessId);
							__log("  GetProcessImagePathByProcessId() failed.");
						}
						CMemory::Free(pImageFileName);
					}
					pMsg->data.times.CreateTime	= times.CreateTime;
					ProcessTable()->Add(true, ProcessId, CreateInfo->ParentProcessId,
						&pMsg->data.ProcGuid, pMsg->data.ProcUID, CreateInfo->ImageFileName, CreateInfo->CommandLine);
					pMsg->header.mode = YFilter::Message::Mode::Event;
					pMsg->header.category = YFilter::Message::Category::Process;
					pMsg->header.size = sizeof(YFILTER_MESSAGE);

					pMsg->data.subType = YFilter::Message::SubType::ProcessStart;
					pMsg->data.bCreationSaved	= true;
					pMsg->data.PID				= (DWORD)ProcessId;
					pMsg->data.PPID				= (DWORD)CreateInfo->ParentProcessId;
					RtlStringCbCopyUnicodeString(pMsg->data.szPath, sizeof(pMsg->data.szPath), CreateInfo->ImageFileName);
					RtlStringCbCopyUnicodeString(pMsg->data.szCommand, sizeof(pMsg->data.szCommand), CreateInfo->CommandLine);
					if (MessageThreadPool()->Push(__FUNCTION__,
						YFilter::Message::Mode::Event,
						YFilter::Message::Category::Process,
						pMsg, sizeof(YFILTER_MESSAGE),
						false))
					{
						//	pMsg는 SendMessage 성공 후 해제될 것이다. 
						MessageThreadPool()->Alert(YFilter::Message::Category::Process);
					}
					else {
						CMemory::Free(pMsg);
					}
				}
			}
			else
			{
				__log("CreateInfo->ImageFileName is null.");
			}
		}
		else
		{
			//	프로세스 종료
			UNICODE_STRING		null	= {0,};
			PYFILTER_MESSAGE	pMsg	= NULL;

			pMsg	= (PYFILTER_MESSAGE)CMemory::Allocate(NonPagedPoolNx, sizeof(YFILTER_MESSAGE), TAG_PROCESS);
			if( pMsg )
			{
				RtlZeroMemory(pMsg, sizeof(YFILTER_MESSAGE));
				pMsg->header.mode			= YFilter::Message::Mode::Event;
				pMsg->header.category		= YFilter::Message::Category::Process;
				pMsg->header.size			= sizeof(YFILTER_MESSAGE);
				pMsg->data.subType			= YFilter::Message::SubType::ProcessStop;
				pMsg->data.bCreationSaved	= false;
				pMsg->data.PID				= (DWORD)ProcessId;
				pMsg->data.PPID				= (DWORD)GetParentProcessId(ProcessId);
				pMsg->data.szCommand[0]		= NULL;
				pMsg->data.szPath[0]		= NULL;
				
				KeQuerySystemTime(&times.ExitTime);
				pMsg->data.times.CreateTime = times.CreateTime;
				pMsg->data.times.ExitTime = times.ExitTime;
				pMsg->data.times.KernelTime = times.KernelTime;
				pMsg->data.times.UserTime = times.UserTime;

				if (ProcessTable()->Remove(ProcessId, true, pMsg, [](
					bool bCreationSaved, PPROCESS_ENTRY pEntry, PVOID pCallbackPtr
					)
					{
						//	이 람다 함수 안은 DISPATCH_LEVEL 입니다.
						//	락을 걸고 있거든요.
						PASSIVE_LEVEL;
						PYFILTER_MESSAGE	pMsg = (PYFILTER_MESSAGE)pCallbackPtr;
						if (pMsg)
						{
							pMsg->data.bCreationSaved	= bCreationSaved;
							if (pEntry && bCreationSaved)
							{						
								RtlStringCbCopyUnicodeString(pMsg->data.szPath, sizeof(pMsg->data.szPath), &pEntry->path);
								RtlStringCbCopyUnicodeString(pMsg->data.szCommand, sizeof(pMsg->data.szCommand), &pEntry->command);
								RtlCopyMemory(&pMsg->data.ProcGuid, &pEntry->uuid, sizeof(pMsg->data.ProcGuid));
							}
						}
					})
				)
				{
					//	이전 생성 정보를 기록해둔 상태.							
				}
				else
				{
					//	이 프로세스는 생성시점 드라이버가 실행 중이지 않아 
					//	해당 프로세스 정보를 가지고 있지 않다. 
					//	[TODO]
					__log("%s %6d NOT FOUND", __FUNCTION__, ProcessId);
					PUNICODE_STRING	pImageFileName		= NULL;
					if (NT_SUCCESS(GetProcessImagePathByProcessId(ProcessId, &pImageFileName)))
					{
						PROCUID		ProcUID;
						RtlStringCbCopyUnicodeString(pMsg->data.szPath, sizeof(pMsg->data.szPath), pImageFileName);
						GetProcGuid(false, ProcessId, GetParentProcessId(ProcessId), pImageFileName, &times.CreateTime, 
							&pMsg->data.ProcGuid, &ProcUID);
						pMsg->data.ProcUID	= ProcUID;
						CMemory::Free(pImageFileName);
					}					
				}
				if (MessageThreadPool()->Push(__FUNCTION__, 
						YFilter::Message::Mode::Event,
						YFilter::Message::Category::Process,
						pMsg, sizeof(YFILTER_MESSAGE), 
						false))
				{
					//	pMsg는 SendMessage 성공 후 해제될 것이다. 
					MessageThreadPool()->Alert(YFilter::Message::Category::Process);
				}
				else
				{
					CMemory::Free(pMsg);
				}
			}
			else
			{
				if (ProcessTable()->Remove(ProcessId, true, NULL, 
					[](bool bCreationSaved, PPROCESS_ENTRY pEntry, PVOID pCallbackPtr) 
						{
						UNREFERENCED_PARAMETER(pCallbackPtr);
						UNREFERENCED_PARAMETER(bCreationSaved);
						if (pEntry)
						{
							__log("%s YFILTER_MESSAGE_DATA allocation failed.", __FUNCTION__);
						}
					})
				)
				{
					//	이전 생성 정보를 기록해둔 상태.							
				}
			}
		}
	}
	else {
		__log("%s filter.Reference() failed.", __FUNCTION__);
	}
}
typedef struct _SYSTEM_PROCESS_INFORMATION {
	ULONG NextEntryOffset;
	ULONG NumberOfThreads;
	BYTE Reserved1[48];
	PVOID Reserved2[3];
	HANDLE UniqueProcessId;
	PVOID Reserved3;
	ULONG HandleCount;
	BYTE Reserved4[4];
	PVOID Reserved5[11];
	SIZE_T PeakPagefileUsage;
	SIZE_T PrivatePageCount;
	LARGE_INTEGER Reserved6[6];
} SYSTEM_PROCESS_INFORMATION;

#define __nextptr(base, offset) ((PVOID)((ULONG_PTR) (base) + (ULONG_PTR) (offset))) 
typedef void	(*PPreCreatedProcessCallback)(HANDLE PID, PUNICODE_STRING pImagePath);

void		GetPreCreatedProcess(PPreCreatedProcessCallback pCallback)
{
	if( NULL == Config() || NULL == Config()->pZwQuerySystemInformation )	return;

	ULONG		nNeededSize	= 0;
	//PVOID		pBuf		= NULL;
	NTSTATUS	status		= Config()->pZwQuerySystemInformation(SystemProcessInformation, NULL, 0, &nNeededSize);
	
	if (STATUS_INFO_LENGTH_MISMATCH == status) {
		//	할당 받아 봅시다.
		//	단, 프로세스 정보는 수시로 변한다는거. 이 다음에 호출하면 더 큰 크기를 원할 수도 있다.
		ULONG	nSize	= nNeededSize * 2;
		PVOID	pBuf	= CMemory::Allocate(PagedPool, nSize, 'GPCP');
		if (pBuf) {
			if (NT_SUCCESS(status = Config()->pZwQuerySystemInformation(SystemProcessInformation, pBuf, nSize, &nNeededSize))) {
				SYSTEM_PROCESS_INFORMATION	*p;
				for (p = (SYSTEM_PROCESS_INFORMATION*)pBuf; p->NextEntryOffset ; 
					p = (SYSTEM_PROCESS_INFORMATION *)__nextptr(p, p->NextEntryOffset)) {
					PUNICODE_STRING	pImageFileName = NULL;
					if (NT_SUCCESS(GetProcessImagePathByProcessId(p->UniqueProcessId, &pImageFileName)))
					{
						if( pCallback )	pCallback(p->UniqueProcessId, pImageFileName);
						CMemory::Free(pImageFileName);
					}
					else {
						if( pCallback )	pCallback(p->UniqueProcessId, NULL);
					}
				}
			}			
			else {
				__log("%s ZwQuerySystemInformation() failed. status=%08x", __FUNCTION__, status);
			}
			CMemory::Free(pBuf);
		}
	}
}
bool		StartProcessFilter()
{
	__log("%s", __FUNCTION__);
	bool		bRet = false;
	NTSTATUS	status = STATUS_SUCCESS;
	if( NULL == Config() )	return false;

	//	이제 새로 생성되는 프로세스에 대해선 콜백을 받아 수집할 것이다.
	//	이미 생성되어 있던 프로세스에 대한 정보는 콜백을 받기전 미리 수집한다.
	GetPreCreatedProcess([](HANDLE PID, PUNICODE_STRING pImageFileName) {
		if (pImageFileName) {
			HANDLE	PPID = GetParentProcessId(PID);
			PUNICODE_STRING		pCmdLine = NULL;
			PYFILTER_MESSAGE	pMsg = NULL;
			KERNEL_USER_TIMES	times;
			pMsg = (PYFILTER_MESSAGE)CMemory::Allocate(NonPagedPoolNx, sizeof(YFILTER_MESSAGE), TAG_PROCESS);
			if (pMsg) {
				RtlZeroMemory(pMsg, sizeof(YFILTER_MESSAGE));
				pMsg->data.PID = (DWORD)PID;
				pMsg->data.PPID = (DWORD)PPID;
				GetProcessTimes(PID, &times);
				pMsg->data.times.CreateTime = times.CreateTime;
				pMsg->data.times.ExitTime = times.ExitTime;
				pMsg->data.times.KernelTime = times.KernelTime;
				pMsg->data.times.UserTime = times.UserTime;

				PROCUID		ProcUID;
				GetProcGuid(true, PID, PPID,
					pImageFileName, &times.CreateTime, &pMsg->data.ProcGuid, &ProcUID);
				pMsg->data.ProcUID	= ProcUID;
				RtlStringCbCopyUnicodeString(pMsg->data.szPath, sizeof(pMsg->data.szPath), pImageFileName);
				GetProcessInfo(PID, ProcessCommandLineInformation, &pCmdLine);
				if (pCmdLine)
					RtlStringCbCopyUnicodeString(pMsg->data.szCommand, sizeof(pMsg->data.szCommand), pCmdLine);

				PUNICODE_STRING	pParentImageFileName = NULL;
				if (PPID && NT_SUCCESS(GetProcessImagePathByProcessId(PPID, &pParentImageFileName)))
				{
					KERNEL_USER_TIMES	ptimes;
					PROCUID				PProcUID;
					if (NT_SUCCESS(GetProcessTimes(PPID, &ptimes))) {
						if (NT_SUCCESS(GetProcGuid(true, PPID,
							GetParentProcessId(PPID),
							pParentImageFileName, &ptimes.CreateTime,
							&pMsg->data.PProcGuid, &PProcUID))) {
							pMsg->data.PProcUID	= PProcUID;
						}
						else {
							__log("%s PProcGuid - failed.", __FUNCTION__);
							__log("  PPID:%d", PPID);
							__log("  GetProcGuid() failed.");
						}
					}
					else {
						__log("%s PProcGuid - failed.", __FUNCTION__);
						__log("  PPID:%d", PPID);
						__log("  GetProcessTimes() failed.");
					}
					CMemory::Free(pParentImageFileName);
				}
				if( pMsg->data.PID <= 4 || pMsg->data.PPID <= 4) {
					__log("%6d %ws", pMsg->data.PID, pMsg->data.szPath);
					__log("       %ws", pMsg->data.szCommand);
				}

				ProcessTable()->Add(false, PID, PPID, &pMsg->data.ProcGuid, pMsg->data.ProcUID, pImageFileName, pCmdLine);
				pMsg->header.mode		= YFilter::Message::Mode::Event;
				pMsg->header.category	= YFilter::Message::Category::Process;
				pMsg->header.size		= sizeof(YFILTER_MESSAGE);

				pMsg->data.subType		= YFilter::Message::SubType::ProcessStart;
				pMsg->data.bCreationSaved = true;
				pMsg->data.PID			= (DWORD)PID;
				pMsg->data.PPID			= (DWORD)PPID;
				if (MessageThreadPool()->Push(__FUNCTION__,
					YFilter::Message::Mode::Event,
					YFilter::Message::Category::Process,
					pMsg, sizeof(YFILTER_MESSAGE),
					false))
				{
					//	pMsg는 SendMessage 성공 후 해제될 것이다. 
					//__log("pushed");
					MessageThreadPool()->Alert(YFilter::Message::Category::Process);
				}
				else {
					CMemory::Free(pMsg);
				}
			}			
			if( pCmdLine )	CMemory::Free(pCmdLine);				
		}
		else 
			__log("%6d null", PID);
	});
	if( USE_OBJECT_CALLBACK )
	{
		OB_OPERATION_REGISTRATION	obOpRegs[2] = { 0, };
		obOpRegs[0].ObjectType = PsProcessType;
		obOpRegs[0].Operations = OB_OPERATION_HANDLE_CREATE;
		obOpRegs[0].PreOperation = ProcessObjectPreCallback;
		obOpRegs[0].PostOperation = ProcessObjectPostCallback;

		obOpRegs[1].ObjectType = PsThreadType;
		obOpRegs[1].Operations = OB_OPERATION_HANDLE_CREATE;
		obOpRegs[1].PreOperation = ThreadObjectPreCallback;
		obOpRegs[1].PostOperation = ThreadObjectPostCallback;

		OB_CALLBACK_REGISTRATION	obCbReg = { 0, };
		obCbReg.Version = OB_FLT_REGISTRATION_VERSION;
		obCbReg.OperationRegistrationCount = 2;
		obCbReg.OperationRegistration = obOpRegs;
		obCbReg.RegistrationContext = NULL;

		LARGE_INTEGER		lTickCount = { 0, };
		WCHAR				szAltitude[40] = L"";

		KeQueryTickCount(&lTickCount);
		RtlStringCbPrintfW(szAltitude, sizeof(szAltitude), L"%ws.%d", DRIVER_STRING_ALTITUDE, lTickCount.LowPart);
		RtlInitUnicodeString(&obCbReg.Altitude, szAltitude);

		status = Config()->pObRegisterCallbacks(&obCbReg, (PVOID *)&Config()->pObCallbackHandle);
		if (NT_SUCCESS(status))	bRet = true;
		else
		{
			STATUS_ACCESS_DENIED;
			__log("[%s:%d][FAILED] ObRegisterCallbacks status %x ", __FUNCTION__, __LINE__, status);
		}
	}
	status = PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCallbackRoutineEx, FALSE);
	if (NT_SUCCESS(status))
	{
		Config()->bProcessNotifyCallback	= true;
	}
	else
	{
		__log("[%s:%d][FAILED] PsSetCreateProcessNotifyRoutineEx status %x ", __FUNCTION__, __LINE__, status);
	}
	return bRet;
}
void		StopProcessFilter(void)
{
	__log("%s", __FUNCTION__);
	if( NULL == Config() )	return;
	if( Config()->bProcessNotifyCallback ) {
		PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCallbackRoutineEx, TRUE);
		Config()->bProcessNotifyCallback	= false;
	}
	if (USE_OBJECT_CALLBACK)
	{
		if (Config()->pObRegisterCallbacks && Config()->pObUnRegisterCallbacks)
		{
			if (Config()->pObCallbackHandle)
			{
				Config()->pObUnRegisterCallbacks(Config()->pObCallbackHandle);
				Config()->pObCallbackHandle = NULL;
			}
		}
	}
}