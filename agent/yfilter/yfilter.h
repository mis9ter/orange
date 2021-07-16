#pragma once

#define _NO_CRT_STDIO_INLINE
//	XP부터 7까지 그리고 8부터 SPIN LOCK 이 구현된 위치가 다르다. 7부터 동작 시키려면 필요.
//	https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-keinitializespinlock
//	https://docs.microsoft.com/ko-kr/windows-hardware/drivers/kernel/spin-locks
#define WIN9X_COMPAT_SPINLOCK	

//#define NTDDI_VERSION	0x0A000000
//#include <sdkddkver.h>

#include <fltKernel.h>
#include <dontuse.h>
#include <devguid.h>
#include <minwindef.h>
#include <ntstrsafe.h>
#include <ntifs.h>
#include <stdlib.h>
#include <wdm.h>

#include "yagent.define.h"
#include "driver.common.h"
#include "Config.h"
#include "CThreadPool.h"
#include "FilterContext.h"
#include "md5.h"
#include "crc64.h"

#pragma comment(lib, "ntstrsafe.lib")
#pragma comment(lib, "fltmgr.lib")
#pragma comment(lib, "wdmsec.lib")

#ifdef _WIN64
#pragma warning(disable: 4390 4311 4302)
#endif

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")
/*
	DbgPrint and DbgPrintEx can be called at IRQL<=DIRQL (Device IRQL).
	However, Unicode format codes (%wc and %ws) can be used only at IRQL = PASSIVE_LEVEL.
	Also, because the debugger uses interprocess interrupts (IPIs) to communicate with other processors,
	calling DbgPrint at IRQL>DIRQL can cause deadlocks.
*/
#define USE_LOG				0
#define	__PRODUCT			"ORANGE"
#define	__MAX_PATH_SIZE		1024
#define __LINE				"----------------------------------------------------------------"
#define NT_FAILED(Status)	(!NT_SUCCESS(Status))
#define TARGET_OBJECT_SIZE					1024
#define PROTECT_FILE_NAME					L"PF"
#define PROTECT_PROCESS_NAME				L"PP"
#define PROTECT_REGISTRY_NAME				L"PR"
#define PROTECT_MEMORY_NAME					L"PM"
#define ALLOW_FILE_NAME						L"AF"
#define ALLOW_PROCESS_NAME					L"AP"
#define ALLOW_REGISTRY_NAME					L"AR"
#define ALLOW_MEMORY_NAME					L"AM"

#define WHITE_PROCESS_NAME					L"WP"
#define GRAY_PROCESS_NAME					L"GP"
#define BLACK_PROCESS_NAME					L"BP"

#ifndef PROCESS_TERMINATE
#define PROCESS_TERMINATE					(0x0001)  
#define PROCESS_CREATE_THREAD				(0x0002)  
#define PROCESS_SET_SESSIONID				(0x0004)  
#define PROCESS_VM_OPERATION				(0x0008)  
#define PROCESS_VM_READ						(0x0010)  
#define PROCESS_VM_WRITE					(0x0020)  
#define PROCESS_DUP_HANDLE					(0x0040)  
#define PROCESS_CREATE_PROCESS				(0x0080)  
#define PROCESS_SET_QUOTA					(0x0100)  
#define PROCESS_SET_INFORMATION				(0x0200)  
#define PROCESS_QUERY_INFORMATION			(0x0400)  
#define PROCESS_SUSPEND_RESUME				(0x0800)  
#define PROCESS_QUERY_LIMITED_INFORMATION	(0x1000)  
#define PROCESS_SET_LIMITED_INFORMATION		(0x2000)  

#if (NTDDI_VERSION >= NTDDI_VISTA)
#define PROCESS_ALL_ACCESS        (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | \
                                   0xFFFF)
#else
#define PROCESS_ALL_ACCESS        (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | \
                                   0xFFF)
#endif
#endif

extern	ULONG_PTR	OperationStatusCtx;
extern	ULONG		gTraceFlags;

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))

extern	CONST FLT_OPERATION_REGISTRATION Callbacks[];
extern	CONST FLT_REGISTRATION FilterRegistration;

inline	bool	UseLog() { return USE_LOG; }
/////////////////////////////////////////////////////////////////////////////////////////
//	보호, 허용 대상 관리
/////////////////////////////////////////////////////////////////////////////////////////
typedef struct OBJECT_TABLE : public UNICODE_PREFIX_TABLE
{
	YFilter::Object::Mode	mode;
	YFilter::Object::Type	type;
	wchar_t			szName[3];
	WORD			wCount;
	FAST_MUTEX		lock;
} OBJECT_TABLE, *POBJECT_TABLE;

typedef struct OBJECT_ENTRY : public UNICODE_PREFIX_TABLE_ENTRY
{
	UNICODE_STRING		data;
	size_t				nAdditionalSize;
	UCHAR				szAdditionalInfo[1];
} OBJECT_ENTRY, *POBJECT_ENTRY;

typedef void(*PEnumObject)(
	YFilter::Object::Mode	mode,
	YFilter::Object::Type	type,
	DWORD					dwTotal,
	DWORD					dwIndex,
	PUNICODE_STRING			str,
	PVOID					ptr,
	DWORD					dwPtr
	);

DWORD		ListObject(
	YFilter::Object::Mode	mode,
	YFilter::Object::Type	type,
	PVOID					ptr,
	DWORD					dwPtr,
	PEnumObject				func
);
bool		AddObject(
	YFilter::Object::Mode	mode,
	YFilter::Object::Type	type,
	PWSTR			pValue
);
bool		RemoveObject(
	YFilter::Object::Mode	mode,
	YFilter::Object::Type	type,
	PWSTR			pValue
);
bool		RemoveAllObject(
	YFilter::Object::Mode	mode,
	YFilter::Object::Type	type
);
bool		IsObject
(
	YFilter::Object::Mode	mode,
	YFilter::Object::Type	type,
	PCWSTR			pValue
);
bool		IsObject(
	YFilter::Object::Mode	mode,
	YFilter::Object::Type	type,
	PUNICODE_STRING			pValue
);
POBJECT_TABLE	ObjectTable(IN YFilter::Object::Mode mode, IN YFilter::Object::Type type);
void			CreateObjectTable(PUNICODE_STRING pRegistry);
void			DestroyObjectTable(PUNICODE_STRING pRegistry);
void			OtInitialize(
	IN PUNICODE_STRING	pRegistry,
	IN YFilter::Object::Mode		mode,
	IN YFilter::Object::Type		type,
	IN PWSTR			pName,
	IN POBJECT_TABLE	pTable
);
void			OtDestroy(IN PUNICODE_STRING pRegistry, IN POBJECT_TABLE pTable);
DWORD			OtList(IN POBJECT_TABLE pTable, LPVOID ptr, DWORD dwPtr, PEnumObject func);
bool			OtAdd(IN POBJECT_TABLE pTable, IN PWSTR p);
bool			OtAdd(IN POBJECT_TABLE pTable, IN PCWSTR p);
bool			OtRemove(IN POBJECT_TABLE pTable, IN PCWSTR p);
bool			OtIsExisting(IN POBJECT_TABLE pTable, IN PCWSTR p);
bool			OtIsExisting(IN POBJECT_TABLE pTable, IN PUNICODE_STRING p);
bool			OtAdd(IN POBJECT_TABLE pTable, IN PCWSTR p);
bool			OtAdd(IN POBJECT_TABLE pTable, IN PUNICODE_STRING p);
bool			OtRemove(IN POBJECT_TABLE pTable, IN PUNICODE_STRING p);
bool			OtRemove(IN POBJECT_TABLE pTable, IN PCWSTR p);
bool			OtRemoveAll(IN POBJECT_TABLE pTable);
bool			OtSave(IN PUNICODE_STRING pRegistry, IN POBJECT_TABLE pTable);
bool			OtRestore(IN POBJECT_TABLE pTable);
ULONG			OtList(IN POBJECT_TABLE pTable);


typedef struct PROCESS_ENTRY
{
	HANDLE				PID;				//	나의 핸들
	HANDLE				PPID;				//	부모의 핸들
	PROCUID				PUID;				//	고유번호
	PROCUID				PPUID;				//	부모의 고유번호
	HANDLE				TID;				//	메인 스레드 핸들
	HANDLE				CPID;				//	나를 생성한 프로세스의 핸들
											//	나를 생성한 프로세스 != 부모 프로세스
	ULONG				SID;				//	프로세스 세션 ID
	UNICODE_STRING		DevicePath;
	UNICODE_STRING		Command;

	PVOID				key;				//	[TODO]	뭐에 쓰는 물건인가요?
	bool				bFree;				//	해제 대상
	bool				bCallback;			//	콜백에 의해 수집
	DWORD64				dwTerminate;		//	종료시 틱
	KERNEL_USER_TIMES	times;
	REG_COUNT			registry;
} PROCESS_ENTRY, * PPROCESS_ENTRY;

/*************************************************************************
	Prototypes
*************************************************************************/
NTSTATUS	GetProcessSessionId(IN HANDLE PID, OUT PULONG pId);
NTSTATUS	GetSystemRootPath(PUNICODE_STRING p);
NTSTATUS	GetParentProcessId(IN HANDLE hProcessId, OUT HANDLE* PPID);
NTSTATUS	GetProcGuid(IN bool bCreate, IN HANDLE hPID,
	IN	HANDLE				hPPID,
	IN	PCUNICODE_STRING	pImagePath,
	IN	LARGE_INTEGER		*pCreateTime,
	OUT	UUID				*pGuid,
	OUT	PROCUID				*pUID
);
NTSTATUS	GetPUID(
	IN	HANDLE				PID,
	IN	HANDLE				PPID,
	IN	PCUNICODE_STRING	pImagePath,
	IN	LARGE_INTEGER		*pCreateTime,
	OUT	PROCUID				*pUID
);
NTSTATUS	GetProcUID(
	IN	HANDLE				hPID,
	IN	HANDLE				hPPID,
	IN	PCUNICODE_STRING	pImagePath,
	IN	LARGE_INTEGER		*pCreateTime,
	OUT	PROCUID				*pUID);
NTSTATUS	GetProcessTimes(IN HANDLE hProcessId, KERNEL_USER_TIMES* p, bool bLog);
//NTSTATUS	SetProcessTimes(HANDLE PID, PY_PROCESS p);

NTSTATUS	GetProcessImagePathByProcessId
(
	HANDLE							pProcessId,
	PWSTR							pImagePathBuffer,
	size_t							nBufferSize,
	PULONG							pNeedSize
);
HANDLE		GetParentProcessId(IN	HANDLE	pProcessId);
NTSTATUS	GetProcessImagePathByProcessId
(
	HANDLE				pProcessId,
	PUNICODE_STRING		*pStr
);
NTSTATUS	GetProcessInfo
(
	HANDLE				pProcessId,
	PROCESSINFOCLASS	infoClass,
	PUNICODE_STRING		*pStr
);
NTSTATUS	GetProcessCodeSignerByProcessId
(
	HANDLE				pProcessId,
	PS_PROTECTED_SIGNER* pSigner
);
NTSTATUS	GetProcessInfoByProcessId
(
	IN	HANDLE			pProcessId,
	OUT HANDLE			*pParentProcessId,
	OUT	PUNICODE_STRING	*pImageFileName,
	OUT PUNICODE_STRING	*pCommandLine			//	지금은 하지마.
);
void		CreateProcessMessage(
	YFilter::Message::SubType	subType,
	HANDLE						PID,
	HANDLE						PPID,
	HANDLE						CPID,
	PROCUID						*pPUID,
	PROCUID						*pPPUID,
	PUNICODE_STRING				pProcPath,
	PUNICODE_STRING				pCommand,
	PKERNEL_USER_TIMES			pTimes,
	PY_PROCESS_MESSAGE			*pOut
);
void		CreateProcessMessage(
	YFilter::Message::SubType	subType,
	PPROCESS_ENTRY				pEntry,
	PY_PROCESS_MESSAGE			*pOut
);
WORD		GetStringDataSize(PUNICODE_STRING pStr);
void		CopyStringData(PVOID pAddress, WORD wOffset, PY_STRING pDest, PUNICODE_STRING pSrc);

/////////////////////////////////////////////////////////////////////////////////////////
//	프로세스 모니터링/관리
/////////////////////////////////////////////////////////////////////////////////////////
bool			StartProcessFilter();
void			StopProcessFilter();
bool			IsRegisteredProcess(IN HANDLE);
bool			RegisterProcess(IN HANDLE h);
bool			DeregisterProcess(IN HANDLE h);

/////////////////////////////////////////////////////////////////////////////////////////
//	모듈 모니터링/관리
/////////////////////////////////////////////////////////////////////////////////////////
bool			StartModuleFilter();
void			StopModuleFilter();

/////////////////////////////////////////////////////////////////////////////////////////
//	스레드 모니터링/관리
/////////////////////////////////////////////////////////////////////////////////////////
bool			StartThreadFilter();
void			StopThreadFilter();

/////////////////////////////////////////////////////////////////////////////////////////
//	레지스트리 모니터링/관리
/////////////////////////////////////////////////////////////////////////////////////////
bool			StartRegistryFilter(IN PDRIVER_OBJECT pDriverObject);
bool			StopRegistryFilter();

uint64_t		Path2CRC64(PUNICODE_STRING pValue);
uint64_t		Path2CRC64(PCWSTR pValue);

/////////////////////////////////////////////////////////////////////////////////////////
//	도라이버-앱 간 통신
/////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS	SendMessage(
	IN	PCSTR				pCause, 
	IN	PFLT_CLIENT_PORT	p,
	IN	PVOID				pSendData, 
	IN	ULONG				nSendDataSize,
	OUT PVOID				pRecvData, 
	OUT ULONG				*pnRecvDataSize,
	IN	bool	bLog);
NTSTATUS	SendMessage(
	IN	PCSTR				pCause,
	IN	PFLT_CLIENT_PORT	p,
	IN	PVOID pSendData, IN ULONG nSendDataSize,
	IN	bool	bLog
);
CThreadPool*	MessageThreadPool();
void			SetMessageThreadCallback(PVOID pCallbackPtr);

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
);
NTSTATUS
DriverUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
);
NTSTATUS
InstanceSetup(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
	_In_ DEVICE_TYPE VolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
);

VOID
InstanceTeardownStart(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

VOID
InstanceTeardownComplete(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

NTSTATUS
Unload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

NTSTATUS
InstanceQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
PreCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

VOID
OperationStatusCallback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
	_In_ NTSTATUS OperationStatus,
	_In_ PVOID RequesterContext
);

FLT_POSTOP_CALLBACK_STATUS
PostCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS	PreCleanup(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);
FLT_POSTOP_CALLBACK_STATUS
PostCleanup(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS	PreRead(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);
FLT_POSTOP_CALLBACK_STATUS
PostRead(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);
FLT_PREOP_CALLBACK_STATUS	PreWrite(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);
FLT_POSTOP_CALLBACK_STATUS
PostWrite(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
PreOperationNoPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

BOOLEAN
DoRequestOperationStatus(
	_In_ PFLT_CALLBACK_DATA Data
);

NTSTATUS	CommandConnected(
	_In_ PFLT_PORT ClientPort,
	_In_ PVOID ServerPortCookie,
	_In_reads_bytes_(SizeOfContext) PVOID ConnectionContext,
	_In_ ULONG SizeOfContext,
	_Flt_ConnectionCookie_Outptr_ PVOID *ConnectionCookie
);
bool		IsCommandConnected();
VOID		CommandDisconnected(_In_opt_ PVOID ConnectionCookie);
NTSTATUS	CommandMessage
(
	_In_ PVOID ConnectionCookie,
	_In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
	_In_ ULONG InputBufferSize,
	_Out_writes_bytes_to_opt_(OutputBufferSize, *ReturnOutputBufferLength) PVOID OutputBuffer,
	_In_ ULONG OutputBufferSize,
	_Out_ PULONG ReturnOutputBufferLength
);
NTSTATUS	EventConnected(
	_In_ PFLT_PORT ClientPort,
	_In_ PVOID ServerPortCookie,
	_In_reads_bytes_(SizeOfContext) PVOID ConnectionContext,
	_In_ ULONG SizeOfContext,
	_Flt_ConnectionCookie_Outptr_ PVOID *ConnectionCookie
);
bool		IsEventConnected();
VOID		EventDisconnected(_In_opt_ PVOID ConnectionCookie);
NTSTATUS	EventMessage
(
	_In_ PVOID ConnectionCookie,
	_In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
	_In_ ULONG InputBufferSize,
	_Out_writes_bytes_to_opt_(OutputBufferSize, *ReturnOutputBufferLength) PVOID OutputBuffer,
	_In_ ULONG OutputBufferSize,
	_Out_ PULONG ReturnOutputBufferLength
);


EXTERN_C_END

#include "CProcessTable.h"
#include "CFileTable.h"
#include "CRegistryTable.h"
//
//  Assign text sections for each routine.
//


//
//  operation registration
//