﻿#include "yfilter.h"
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
	g_process.Initialize(false);
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
bool			RegisterProcess(IN HANDLE PID)
{
	return g_process.Add(false, PID, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
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

	UNREFERENCED_PARAMETER(currentPID);
	UNREFERENCED_PARAMETER(targetPID);
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

	UNREFERENCED_PARAMETER(hPID);
	__log("%s %d", __FUNCTION__, hPID);
	return status;
}
/*
NTSTATUS	SetProcessTimes(HANDLE PID, PY_PROCESS p) {
	KERNEL_USER_TIMES	times;
	NTSTATUS	status	= GetProcessTimes(PID, &times, false);
	if( NT_FAILED(status) )	{
		__log("%-32s NT_FAILED", __func__);
		return status;
	}
	p->times.CreateTime	= times.CreateTime;
	p->times.ExitTime	= times.ExitTime;
	p->times.KernelTime	= times.KernelTime;
	p->times.UserTime	= times.UserTime;

	return STATUS_SUCCESS;
}
*/

#ifdef __DEV
void	GetProcessModules(HANDLE PID) {

	PEPROCESS	pProcess = NULL;
	PsLookupProcessByProcessId(PID, &pProcess);

	PPEB pPeb = PsGetProcessPeb(pProcess);
	KAPC_STATE state;

	KeStackAttachProcess(pProcess, &state);

	PLIST_ENTRY pListEntry = pPeb->Ldr; //this problem

	KeUnstackDetachProcess(&state);
}
#endif

void	__stdcall	ProcessNotifyCallbackRoutineEx(
	_Inout_		PEPROCESS Process,
	_In_		HANDLE ProcessId,
	_Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(Process);
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(CreateInfo);

	if( KeGetCurrentIrql() > PASSIVE_LEVEL )
		__function_log;

	if( NULL == Config() || 0 == Config()->flag.nRun || 0 == Config()->flag.nProcess ) {
		if( NULL == Config() )	{	__log("%-32s NULL == Config()", __func__);	}
		else
		{
			__log("%-32s %d %d", __func__, Config()->flag.nRun, Config()->flag.nProcess);
		}
		return;
	}

	CFltObjectReference		filter(Config()->pFilter);
	//FILTER_REPLY_DATA		reply;
	//ULONG					nReplySize = 0;

	/*
		PS_CREATE_NOTIFY_INFO

		ParentProcessId

		The process ID of the parent process for the new process. 
		Note that the parent process is not necessarily the same process as the process that created the new process. 
		The new process can inherit certain properties of the parent process, such as handles or shared memory. 
		(The process ID of the process creator is given by CreatingThreadId->UniqueProcess.)

		--> Remote Thread 를 만드는 경우를 판단??

		CreatingThreadId

		The process ID and thread ID of the process and thread that created the new process. 
		CreatingThreadId->UniqueProcess contains the process ID, 
		and CreatingThreadId->UniqueThread contains the thread ID.

		https://scorpiosoftware.net/2021/01/10/parent-process-vs-creator-process/
	
		윈도 파일명에 대한 탐구:
		

		(1) "\\?\" 로 시작:	
		문자열 관련 조작 필요 없이 그대로 파일 시스템으로 전달 가능한 경로.

		(2) \\.\PhysicalDiskX"

		https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file
	
	*/
	
	if (filter.Reference())
	{
		if (CreateInfo)
		{
			HANDLE	PID		= ProcessId;
			HANDLE	PPID	= CreateInfo->ParentProcessId;
			HANDLE	CPID	= CreateInfo->CreatingThreadId.UniqueProcess;

			if( PPID != CPID ) {
				__log("%-32s PID:%d PPID:%d RPID:%d", __func__, PID, PPID, CPID);

				PUNICODE_STRING		ProcPath	= NULL;
				PUNICODE_STRING		PProcPath	= NULL;
				PUNICODE_STRING		CProcPath	= NULL;

				if( NT_SUCCESS(GetProcessImagePathByProcessId(PID, &ProcPath)) ) {
					__log("PID :%d %wZ", PID, ProcPath);
					CMemory::Free(ProcPath);				
				}
				if( NT_SUCCESS(GetProcessImagePathByProcessId(PPID, &PProcPath)) ) {
					__log("PPID:%d %wZ", PPID, PProcPath);
					CMemory::Free(PProcPath);				
				}
				if( NT_SUCCESS(GetProcessImagePathByProcessId(CPID, &CProcPath)) ) {
					__log("CPID:%d %wZ", CPID, CProcPath);
					CMemory::Free(CProcPath);				
				}					
				if( CreateInfo->FileOpenNameAvailable && CreateInfo->ImageFileName )
					__log(" %wZ", CreateInfo->ImageFileName);
				if( CreateInfo->CommandLine )
					__log(" %wZ", CreateInfo->CommandLine);
			}
			CreateInfo->FileOpenNameAvailable;	
			//	[TODO] 이거 뭐지?
			//	A Boolean value that specifies whether 
			//	the ImageFileName member contains the exact file name 
			//	that is used to open the process executable file.
			//CUnicodeString		procPath(CreateInfo->ImageFileName, PagedPool);
			//CUnicodeString		command(CreateInfo->CommandLine, PagedPool);
			//__log("%ws %ws", (PCWSTR)procPath, (PCWSTR)command);
			if( CreateInfo->FileOpenNameAvailable && CreateInfo->ImageFileName )
			{
				//	CreateInfo->ImageFileName
				//	If IsSubsystemProcess is TRUE, this value maybe NULL.
				//	pImageFileName	-> \Device\HarddiskVolume2\...
				//	CreateInfo->ImageFileName	-> \??\c:\windows..
				/*
				PEPROCESS		proc	= NULL;
				if( NT_SUCCESS(PsLookupProcessByProcessId(ProcessId, &proc)) ) {
				
					PsGetProcessCreateTimeQuadPart()
					PACCESS_TOKEN	t	= PsReferencePrimaryToken(proc);
					if( t ) {
						LUID	luid;
						SeQueryAuthenticationIdToken(t, &luid);

					ObDereferenceObject(proc);
				}
				*/
				
				__log("%10s %wZ", CProcessTable::ProcessIsWow64(PID)? "WOW64":"NATIVE", 
						CreateInfo->ImageFileName);
				AddProcessToTable2(__func__, true, PID, NULL, NULL, NULL, 
					true, &CPID, 
					[](PVOID pContext, HANDLE PID, PROCUID PUID, 
						PUNICODE_STRING pProcPath, PUNICODE_STRING pCommand, PKERNEL_USER_TIMES pTimes,
						HANDLE PPID, PROCUID PPUID) 
					{
						UNREFERENCED_PARAMETER(pContext);
						UNREFERENCED_PARAMETER(PID);
						UNREFERENCED_PARAMETER(PUID);
						UNREFERENCED_PARAMETER(pProcPath);
						UNREFERENCED_PARAMETER(pCommand);
						UNREFERENCED_PARAMETER(PPID);
						UNREFERENCED_PARAMETER(PPUID);

						if( Config()->client.event.nConnected ) {
							PY_PROCESS_MESSAGE	pMsg	= NULL;
							HANDLE				CPID	= *(HANDLE *)pContext;
							CreateProcessMessage(
								YFilter::Message::SubType::ProcessStart,
								PID, PPID, CPID, &PUID, &PPUID, pProcPath, pCommand, pTimes,
								&pMsg
							);
							if( pMsg ) {
								if( MessageThreadPool()->Push(__func__, pMsg->mode, pMsg->category, 
										pMsg, pMsg->wDataSize+pMsg->wStringSize, false) ) {
									MessageThreadPool()->Alert(pMsg->category);
									pMsg	= NULL;
								}
								else {
									CMemory::Free(pMsg);
								}
							}	
						}
				});
			}
			else
			{
				__log("CreateInfo->ImageFileName is null.");
			}
		}
		else
		{
			struct __ARG { 
				PY_PROCESS_MESSAGE	pMsg;
				KERNEL_USER_TIMES	times;
			} arg;
			RtlZeroMemory(&arg, sizeof(arg));
			GetProcessTimes(ProcessId, &arg.times, false);
			if (ProcessTable()->Remove(ProcessId, true, &arg, [](
				bool bCreationSaved, PPROCESS_ENTRY pEntry, PVOID pContext
				)
			{
				//	이 람다 함수 안은 DISPATCH_LEVEL 입니다.
				//	락을 걸고 있거든요.
				PASSIVE_LEVEL;
				pEntry->times	= ((struct __ARG *)pContext)->times;
				if( bCreationSaved ) {
					//	프로세스 정보를 기록해둔 경우 				
					CreateProcessMessage(
						YFilter::Message::SubType::ProcessStop,
						pEntry,
						&((struct __ARG *)pContext)->pMsg
					);
				}
				else {
					__dlog("%-32s PID:%d bCreationSaved == false", __func__, (DWORD)pEntry->PID);				
				}
			}))
			{
				
			}
			else {
				__log("%-32s PID:%d is not found.", __func__, (DWORD)ProcessId);			
			}
			PY_PROCESS_MESSAGE			pMsg	= arg.pMsg;
			if( pMsg ) {
				if( false ) {
					__log(TEXTLINE);
					__log("%s", __func__);
					__log(TEXTLINE);
					__log("PID  :%d",	(DWORD)pMsg->PID);
					__log("CPID :%d",	(DWORD)PsGetCurrentProcessId());
					__log("TID  :%d",	(DWORD)pMsg->TID);
					__log("CTID :%d",	(DWORD)PsGetCurrentThreadId());
					__log("PUID :%p",	pMsg->PUID);
					__log("PATH :%ws",	pMsg->DevicePath.pBuf);
					__log("CMD  :%ws",	pMsg->Command.pBuf);
					__log("PPID :%d",	(DWORD)pMsg->PPID);
					__log("PPUID:%p",	pMsg->PPUID);
					__log("CREAT:%p",	pMsg->times.CreateTime.QuadPart);
					__log("EXIT :%p",	pMsg->times.ExitTime.QuadPart);
					__log("Kerne:%p",	pMsg->times.KernelTime.QuadPart);
					__log("User :%p",	pMsg->times.UserTime.QuadPart);		
				}
				if( MessageThreadPool()->Push(__func__, pMsg->mode, pMsg->category, pMsg, 
						arg.pMsg->wDataSize+arg.pMsg->wStringSize, false) ) {
					MessageThreadPool()->Alert(pMsg->category);
					pMsg	= NULL;
				}
				else {
					CMemory::Free(pMsg);
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
	PAGED_CODE();

	bool		bRet = false;
	NTSTATUS	status = STATUS_SUCCESS;
	if( NULL == Config() )	return false;

	//	이제 새로 생성되는 프로세스에 대해선 콜백을 받아 수집할 것이다.
	//	이미 생성되어 있던 프로세스에 대한 정보는 콜백을 받기전 미리 수집한다.
	GetPreCreatedProcess([](HANDLE PID, PUNICODE_STRING pProcPath) {
		AddProcessToTable2(__func__, false, 
			PID, pProcPath, NULL, NULL, 
			false, NULL,
			[](PVOID pContext, HANDLE PID, PROCUID PUID, 
				PUNICODE_STRING pProcPath, PUNICODE_STRING pCommand, PKERNEL_USER_TIMES pTimes,
				HANDLE PPID, PROCUID PPUID) 
			{
				UNREFERENCED_PARAMETER(pContext);
				if( Config()->client.event.nConnected ) {
					PY_PROCESS_MESSAGE	pMsg	= NULL;
					CreateProcessMessage(
						YFilter::Message::SubType::ProcessStart2,
						PID, PPID, PPID, &PUID, &PPUID, pProcPath, pCommand, pTimes,
						&pMsg
					);
					if( pMsg ) {
						if( MessageThreadPool()->Push(__func__, pMsg->mode, pMsg->category, pMsg, 
								pMsg->wDataSize+pMsg->wStringSize, false) ) {
							MessageThreadPool()->Alert(pMsg->category);
							pMsg	= NULL;
						}
						else {
							CMemory::Free(pMsg);
						}
					}	
				}
			});
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