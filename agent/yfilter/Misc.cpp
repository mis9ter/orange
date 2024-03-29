﻿#include "yfilter.h"

//ProcessCommandLineInformation

typedef struct _PEB_LDR_DATA {
	BYTE       Reserved1[8];
	PVOID      Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, * PPEB_LDR_DATA;
typedef struct _RTL_USER_PROCESS_PARAMETERS {
	BYTE           Reserved1[16];
	PVOID          Reserved2[10];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, * PRTL_USER_PROCESS_PARAMETERS;
typedef VOID(NTAPI* PPS_POST_PROCESS_INIT_ROUTINE) (VOID);
typedef struct _PEB {
	BYTE                          Reserved1[2];
	BYTE                          BeingDebugged;
	BYTE                          Reserved2[1];
	PVOID                         Reserved3[2];
	PPEB_LDR_DATA                 Ldr;
	PRTL_USER_PROCESS_PARAMETERS  ProcessParameters;
	PVOID                         Reserved4[3];
	PVOID                         AtlThunkSListPtr;
	PVOID                         Reserved5;
	ULONG                         Reserved6;
	PVOID                         Reserved7;
	ULONG                         Reserved8;
	ULONG                         AtlThunkSListPtr32;
	PVOID                         Reserved9[45];
	BYTE                          Reserved10[96];
	PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
	BYTE                          Reserved11[128];
	PVOID                         Reserved12[1];
	ULONG                         SessionId;
} PEB, * PPEB;
bool		AddProcessToTable2(
	PCSTR				pCause,
	bool				bDetectByCallback,
	HANDLE				PID, 
	PUNICODE_STRING		pProcPath,
	PUNICODE_STRING		pCommand,
	PKERNEL_USER_TIMES	pTimes,
	bool				bAddParent,
	PVOID				pContext,
	void				(*pCallback)
	(
		PVOID				pContext,
		HANDLE				PID,
		PROCUID				PUID,
		PUNICODE_STRING		pProcPath,
		PUNICODE_STRING		pCommand,
		PKERNEL_USER_TIMES	pTimes,
		HANDLE				PPID,
		PROCUID				PPUID
	)
) 
{
	HANDLE				PPID		= GetParentProcessId(PID);
	PUNICODE_STRING		_pProcPath	= NULL;
	PUNICODE_STRING		_pCommand	= NULL;
	PUNICODE_STRING		pPProcPath	= NULL;
	KERNEL_USER_TIMES	times		= {0,};

	if( NULL == pTimes ) {
		GetProcessTimes(PID, &times, false);
		pTimes	= &times;
	}
	PROCUID				PUID		= 0;
	PROCUID				PPUID		= 0;

	bool				bRet		= false;
	__try {
		if( NULL == pProcPath ) {
			if( NT_SUCCESS(GetProcessImagePathByProcessId(PID, &_pProcPath))) {		
				pProcPath	= _pProcPath;
			}
			else {
				//__log("%-32s GetProcessImagePathByProcessId(%d) failed.", __func__, (DWORD)PID);
				__leave;
			}
		}
		if( NULL == pCommand ) {
			if( NT_SUCCESS(GetProcessInfo(PID, ProcessCommandLineInformation, &_pCommand))) {
				pCommand	= _pCommand;
			}	
		}
		GetProcUID(PID, PPID, pProcPath, &times.CreateTime, &PUID);
		KERNEL_USER_TIMES	parentTimes	= {0,};
		if( NT_SUCCESS(GetProcessImagePathByProcessId(PPID, &pPProcPath))) {
			GetProcessTimes(PPID, &parentTimes, false);
			GetProcUID(PPID, GetParentProcessId(PPID), pPProcPath, &parentTimes.CreateTime, &PPUID);		
		}			
		//if( PID <= (HANDLE)4 || PPID <= (HANDLE)4) {
		//	__log("PID=%6d PPID=%06d %wZ", (DWORD)PID, (DWORD)PPID, pProcPath);
		//}
		if( ProcessTable()->Add(bDetectByCallback, PID, PPID, PPID, PUID, PPUID, pProcPath, pCommand, &times) ) {	
			//	이미 존재하는 PID라면 실패될 수도 있다.
			//__log("%-32s PID=%06d PPID=%06d", __func__, (DWORD)PID, (DWORD)PPID);
			if( pCallback )
				pCallback(pContext, PID, PUID, pProcPath, pCommand, pTimes, PPID, PPUID);
			bRet	= true;

			UNREFERENCED_PARAMETER(pCause);

			#ifdef __TEST__
			__log(TEXTLINE);
			__log("%s", pCause);
			__log(TEXTLINE);
			__log("PID  :%d", (DWORD)PID);
			__log("PUID :%p", PUID);
			__log("PATH :%wZ", pProcPath);
			__log("CMD  :%wZ", pCommand);
			__log("PPID :%d", (DWORD)PPID);
			__log("PPUID:%p", PPUID);
			#endif
		}			
		//	PID의 Add 성공 여부와 상관없이 부모가 없다면 부모도 넣어주자.
		if( bAddParent && false == ProcessTable()->IsExisting(PPID, NULL, NULL)) {
			//__log("%-32s add PPID %06d", __func__, PPID);
			AddProcessToTable2(__func__, false, PPID, pPProcPath, NULL, &parentTimes, true, pContext, pCallback);		
		}
	}
	__finally {
		if( _pCommand ) 	CMemory::Free(_pCommand);
		if( _pProcPath )	CMemory::Free(_pProcPath);
		if( pPProcPath )	CMemory::Free(pPProcPath);
	}
	return true;
}

/*
NTSTATUS	GetProcessParams(IN HANDLE hProcessId,
	OUT	HANDLE	* PPID,
	OUT	PWSTR	* pPath,	IN DWORD dwPathSize,
	OUT PWSTR	* pCmdLine, IN DWORD dwCmdLineSize
)
{
	if (NULL == Config())	return STATUS_UNSUCCESSFUL;

	FN_ZwQueryInformationProcess	pZwQueryInformationProcess = Config()->pZwQueryInformationProcess;
	if (NULL == pZwQueryInformationProcess)	return STATUS_BAD_FUNCTION_TABLE;
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	return STATUS_UNSUCCESSFUL;

	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	HANDLE				hProcess = NULL;
	OBJECT_ATTRIBUTES	oa = { 0 };
	CLIENT_ID			cid = { 0 };

	__try
	{
		oa.Length = sizeof(OBJECT_ATTRIBUTES);
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
		cid.UniqueProcess = hProcessId;

#ifndef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_INFORMATION (0x0400)
#endif
		status = ZwOpenProcess(&hProcess, PROCESS_QUERY_INFORMATION, &oa, &cid);
		if (!NT_SUCCESS(status))	__leave;

		PROCESS_BASIC_INFORMATION	info;
		status = pZwQueryInformationProcess(hProcess, ProcessBasicInformation, &info, sizeof(info), NULL);
		if (NT_SUCCESS(status)) {
			*PPID = (HANDLE)info.InheritedFromUniqueProcessId;
			if (info.PebBaseAddress) {
				//	[TODO]	PEB를 읽는 도중에 사라지면 어쩌지?
				//	ZwOpenProcess를 하면 ZwClose 전까지유지해주려나?
				StringCbPrintf(pPath, dwPathSize, L"%Zw", info.PebBaseAddress->ProcessParameters->ImagePathName);
				StringCbPrintf(pCmdLine, dwCmdLineSize, L"%Zw", info.PebBaseAddress->ProcessParameters->CommandLine);
			}
		}
	}
	__finally
	{
		if (hProcess)
		{
			ZwClose(hProcess);
			hProcess = NULL;
		}
	}
	return status;
}
*/
NTSTATUS	NTAPI	GetProcessSessionId(
	HANDLE  PID, 
	PULONG  pSessionId
) 
{
	NTSTATUS	status	= STATUS_UNSUCCESSFUL;
	HANDLE		process = NULL;
	OBJECT_ATTRIBUTES oa = {0};
	CLIENT_ID cid = {0};
	ULONG	ReturnLength = 0;

	if (PID <= (HANDLE)4)
	{
		return STATUS_INVALID_PARAMETER;
	}
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	__try
	{
		oa.Length = sizeof(OBJECT_ATTRIBUTES);
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
		cid.UniqueProcess = PID;

#ifndef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_INFORMATION (0x0400)
#endif
		status = ZwOpenProcess(&process, PROCESS_QUERY_INFORMATION, &oa, &cid);
		if (!NT_SUCCESS(status))	__leave;
		status = Config()->pZwQueryInformationProcess(process, ProcessSessionInformation, pSessionId, sizeof(ULONG), &ReturnLength);
		if (NT_SUCCESS(status) == FALSE)	__leave;
	} __finally {
		if (process)
		{
			ZwClose(process);
			process = NULL;
		}
	}
	return status;    
}

NTSTATUS	GetParentProcessId(IN HANDLE hProcessId, OUT HANDLE* PPID)
{
	if (NULL == Config())	return STATUS_UNSUCCESSFUL;

	FN_ZwQueryInformationProcess	pZwQueryInformationProcess = Config()->pZwQueryInformationProcess;
	if (NULL == pZwQueryInformationProcess)	return STATUS_BAD_FUNCTION_TABLE;
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	return STATUS_UNSUCCESSFUL;

	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	HANDLE				hProcess = NULL;
	OBJECT_ATTRIBUTES	oa = { 0 };
	CLIENT_ID			cid = { 0 };

	__try
	{
		oa.Length = sizeof(OBJECT_ATTRIBUTES);
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
		cid.UniqueProcess = hProcessId;

#ifndef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_INFORMATION (0x0400)
#endif
		status = ZwOpenProcess(&hProcess, PROCESS_QUERY_INFORMATION, &oa, &cid);
		if (!NT_SUCCESS(status))	__leave;

		PROCESS_BASIC_INFORMATION	info;
		status = pZwQueryInformationProcess(hProcess, ProcessBasicInformation, &info, sizeof(info), NULL);
		if (NT_SUCCESS(status)) {
			*PPID = (HANDLE)info.InheritedFromUniqueProcessId;
		}
	}
	__finally
	{
		if (hProcess)
		{
			ZwClose(hProcess);
			hProcess = NULL;
		}
	}
	return status;
}
NTSTATUS	GetProcUID(
	IN	HANDLE				PID,
	IN	HANDLE				PPID,
	IN	PCUNICODE_STRING	pImagePath,		//	\Device\HarddiskVolumeX\와 같은 형태로 구성된 경로
	IN	LARGE_INTEGER		*pCreateTime,
	OUT	PROCUID				*pPUID
)
{
	NTSTATUS		status = STATUS_SUCCESS;
	CWSTRBuffer		proc;
	/*
		원래 계획은 부모ID까지 넣는 것인데
		부모의 ProcGuid를 구하려 보니 조부모의 존재가 불확실하다.
	*/
	if( NULL == PID )	{
		if( pPUID )		*pPUID	= 0;
		return status;
	}
	if( NULL == PPID )	GetParentProcessId(PID, &PPID);

	KERNEL_USER_TIMES	times	= {0, };
	if( NULL == pCreateTime ) {
		if( NT_FAILED(GetProcessTimes(PPID, &times, false))) {
			__dlog("%-32s GetProcessTimes() failed.", __func__);
		
		}
		pCreateTime	= &times.CreateTime;
	}
	RtlStringCbPrintfW(proc, proc.CbSize(), L"%wZ.%d.%d.%d.%d.%d.%wZ",
		&Config()->machineGuid,
		Config()->bootId, 	//	bootid
		pCreateTime->HighPart, 
		pCreateTime->LowPart,
		(DWORD)PPID,
		(DWORD)PID,
		pImagePath);

	int	nSize = 0;
	for (PWSTR p = proc; *p; p++) {
		*p = towlower(*p);
		nSize++;
	}
	if( pPUID )	*pPUID	= Path2CRC64(proc);
	return status;
}
NTSTATUS	GetPUID(
	IN	HANDLE				PID,
	IN	HANDLE				PPID,
	IN	PCUNICODE_STRING	pImagePath,
	IN	LARGE_INTEGER		*pCreateTime,
	OUT	PROCUID				*pUID
) {
	NTSTATUS		status = STATUS_SUCCESS;
	CWSTRBuffer		procGuid;
	/*
	원래 계획은 부모ID까지 넣는 것인데
	부모의 ProcGuid를 구하려 보니 조부모의 존재가 불확실하다.
	*/
	if( NULL == PPID )
		GetParentProcessId(PID, &PPID);
	RtlStringCbPrintfW(procGuid, procGuid.CbSize(), L"%wZ.%d.%d.%d.%d.%d.%wZ",
		&Config()->machineGuid,
		Config()->bootId, 	//	bootid
		pCreateTime->HighPart, pCreateTime->LowPart,
		(DWORD)PPID,
		(DWORD)PID,
		pImagePath);

	int	nSize = 0;
	for (PWSTR p = procGuid; *p; p++) {
		*p = towlower(*p);
		nSize++;
	}
	if( pUID )	*pUID	= Path2CRC64(procGuid);
	return status;

}
NTSTATUS	GetProcGuid(IN bool bCreate, 
	IN HANDLE hPID,
	IN	HANDLE				hPPID,
	IN	PCUNICODE_STRING	pImagePath,
	IN	LARGE_INTEGER		*pCreateTime,
	OUT	UUID				*pGuid,
	OUT PROCUID				*pUID
)
{
	UNREFERENCED_PARAMETER(bCreate);
	NTSTATUS		status = STATUS_SUCCESS;
	CWSTRBuffer		procGuid;
	/*
		원래 계획은 부모ID까지 넣는 것인데
		부모의 ProcGuid를 구하려 보니 조부모의 존재가 불확실하다.
	*/
	if( NULL == hPPID )
		GetParentProcessId(hPID, &hPPID);
	RtlStringCbPrintfW(procGuid, procGuid.CbSize(), L"%wZ.%d.%d.%d.%d.%d.%wZ",
		&Config()->machineGuid,
		Config()->bootId, 	//	bootid
		pCreateTime->HighPart, pCreateTime->LowPart,
		(DWORD)hPPID,
		(DWORD)hPID,
		pImagePath);

	if( 388 == (DWORD)hPID )
		__log("%ws", (PWSTR)procGuid);
	int	nSize = 0;
	for (PWSTR p = procGuid; *p; p++) {
		*p = towlower(*p);
		nSize++;
	}
	if( pGuid ) {	
		md5_state_t state;          /* MD5 state info */
		md5_byte_t  sum[16];        /* Sum data */

		md5_init(&state);
		md5_append(&state, (unsigned char*)(PWSTR)procGuid, nSize * sizeof(WCHAR));
		md5_finish(&state, sum);

		pGuid->Data1 = sum[0] << 24 | sum[1] << 16 | sum[2] << 8 | sum[3];
		pGuid->Data2 = sum[4] << 8 | sum[5];
		pGuid->Data3 = sum[6] << 8 | sum[7];
		pGuid->Data4[0] = sum[8];
		pGuid->Data4[1] = sum[9];
		pGuid->Data4[2] = sum[10];
		pGuid->Data4[3] = sum[11];
		pGuid->Data4[4] = sum[12];
		pGuid->Data4[5] = sum[13];
		pGuid->Data4[6] = sum[14];
		pGuid->Data4[7] = sum[15];
	}
	if( pUID )	*pUID	= Path2CRC64(procGuid);
	return status;
}
NTSTATUS	GetProcessTimes(IN HANDLE hProcessId, KERNEL_USER_TIMES* p, bool bLog)
{
	if( p ) {
		RtlZeroMemory(p, sizeof(KERNEL_USER_TIMES));
	}
	else {
		return STATUS_UNSUCCESSFUL;
	}
	if (NULL == Config())	return STATUS_UNSUCCESSFUL;
	if( NULL == hProcessId) {
		RtlZeroMemory(p, sizeof(KERNEL_USER_TIMES));
		return STATUS_SUCCESS;	
	}
	FN_ZwQueryInformationProcess	pZwQueryInformationProcess = Config()->pZwQueryInformationProcess;
	if (NULL == pZwQueryInformationProcess)	return STATUS_BAD_FUNCTION_TABLE;
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	{
		__dlog("%-32s KeGetCurrentIrql() == %d", __func__, KeGetCurrentIrql());
		return STATUS_UNSUCCESSFUL;
	}

	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	HANDLE				hProcess = NULL;
	OBJECT_ATTRIBUTES	oa = { 0 };
	CLIENT_ID			cid = { 0 };

	__try
	{
		oa.Length = sizeof(OBJECT_ATTRIBUTES);
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
		cid.UniqueProcess = hProcessId;

#ifndef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_INFORMATION (0x0400)
#endif
		status = ZwOpenProcess(&hProcess, PROCESS_QUERY_INFORMATION, &oa, &cid);
		if (!NT_SUCCESS(status)) {
			__dlog("%-32s ZwOpenProcess(%d) failed.", __func__, (DWORD)hProcessId);
			__leave;
		}
		status = pZwQueryInformationProcess(hProcess, ProcessTimes, p, sizeof(KERNEL_USER_TIMES), NULL);
		if (NT_SUCCESS(status)) {
			if( bLog )				__dlog("%-32s succeeded.", __func__);
		}
		else {
			__dlog("%-32s ZwQueryInformationProcess(%d) failed.", __func__, (DWORD)hProcessId);
		}
	}
	__finally
	{
		if (hProcess)
		{
			ZwClose(hProcess);
			hProcess = NULL;
		}
	}
	return status;
}
NTSTATUS	GetProcessInfoByProcessId
(
	IN	HANDLE			pProcessId,
	OUT HANDLE			*pParentProcessId,
	OUT	PUNICODE_STRING	*pImageFileName,
	OUT PUNICODE_STRING *pCommandLine			//	지금은 하지마.
)
{
	if (NULL == Config())	return STATUS_UNSUCCESSFUL;

	FN_ZwQueryInformationProcess	pZwQueryInformationProcess = Config()->pZwQueryInformationProcess;
	if (NULL == pZwQueryInformationProcess)	return STATUS_BAD_FUNCTION_TABLE;
	if (pProcessId <= (HANDLE)4)			return STATUS_INVALID_PARAMETER;
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	return STATUS_UNSUCCESSFUL;

	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	HANDLE				hProcess = NULL;
	OBJECT_ATTRIBUTES	oa = { 0 };
	CLIENT_ID			cid = { 0 };
	ULONG				nNeedSize = 0;

	__try
	{	
		if( pImageFileName	)	*pImageFileName	= NULL;
		if( pCommandLine	)	*pCommandLine	= NULL;
		if( pParentProcessId)	*pParentProcessId	= NULL;
		
		oa.Length = sizeof(OBJECT_ATTRIBUTES);
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
		cid.UniqueProcess = pProcessId;

#ifndef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_INFORMATION (0x0400)
#endif
		status = ZwOpenProcess(&hProcess, PROCESS_QUERY_INFORMATION, &oa, &cid);
		if (!NT_SUCCESS(status))	__leave;

		if( pCommandLine || pParentProcessId )
		{
			PROCESS_BASIC_INFORMATION	info	= {0,};
			status = pZwQueryInformationProcess(hProcess, ProcessBasicInformation, &info, sizeof(info), &nNeedSize);
			if (status != STATUS_INFO_LENGTH_MISMATCH)	__leave;

			if( pParentProcessId )
				*pParentProcessId	= (HANDLE)info.InheritedFromUniqueProcessId;
			/*
			if( pCommandLine && info.PebBaseAddress->ProcessParameters )
			{
				UNICODE_STRING;
				nNeedSize	= info.PebBaseAddress->ProcessParameters->CommandLine.MaximumLength;
				if( nNeedSize )
				{
					*pCommandLine = (PUNICODE_STRING)CMemory::AllocateString(NonPagedPoolNx, nNeedSize, 'GPIP');
					if (NULL == *pCommandLine)	__leave;
					RtlZeroMemory(*pCommandLine, nNeedSize);
			status = pZwQueryInformationProcess(hProcess, ProcessImageFileName, *pImageFileName, nNeedSize, &nNeedSize);
			if (!NT_SUCCESS(status))	__leave;


			info.PebBaseAddress;
			*/
		}
		if (pImageFileName)
		{
			PUNICODE_STRING	p	= NULL;
			status = pZwQueryInformationProcess(hProcess, ProcessImageFileName, NULL, 0, &nNeedSize);
			if (status != STATUS_INFO_LENGTH_MISMATCH)	__leave;
			if( NULL == (p = (PUNICODE_STRING)CMemory::Allocate(NonPagedPoolNx, nNeedSize, 'GPIP')) )
				__leave;
			RtlZeroMemory(p, nNeedSize);
			status = pZwQueryInformationProcess(hProcess, ProcessImageFileName, p, nNeedSize, &nNeedSize);
			if (NT_FAILED(status)) 
			{
				CMemory::Free(p);
				__leave;
			}
			__log("%s ----", __FUNCTION__);
			*pImageFileName	= p;
		}
	}
	__finally
	{
		if (hProcess)
		{
			ZwClose(hProcess);
			hProcess = NULL;
		}
	}
	return status;
}
HANDLE		GetParentProcessId(IN	HANDLE	pProcessId)
{
	if (NULL == Config())	return NULL;

	FN_ZwQueryInformationProcess	pZwQueryInformationProcess = Config()->pZwQueryInformationProcess;
	if (NULL == pZwQueryInformationProcess)	return NULL;
	if (pProcessId <= (HANDLE)4)			return NULL;
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	return NULL;

	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	HANDLE				hProcess = NULL;
	OBJECT_ATTRIBUTES	oa = { 0 };
	CLIENT_ID			cid = { 0 };
	ULONG				nNeedSize = 0;
	HANDLE				hParentProcessId	= NULL;
	__try
	{
		oa.Length = sizeof(OBJECT_ATTRIBUTES);
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
		cid.UniqueProcess = pProcessId;

#ifndef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_INFORMATION (0x0400)
#endif
		status = ZwOpenProcess(&hProcess, PROCESS_QUERY_INFORMATION, &oa, &cid);
		if (NT_FAILED(status))	{
			//__log("%s %d ZwOpenProcess() failed. status=%08x", 
			//	__FUNCTION__, pProcessId, status);
			__leave;
		}
		PROCESS_BASIC_INFORMATION	info = { 0, };
		status = pZwQueryInformationProcess(hProcess, ProcessBasicInformation, &info, sizeof(info), &nNeedSize);
		if (NT_FAILED(status))	{
			__log("%s %d ZwQueryInformationProcess() failed. status=%08x", 
				__FUNCTION__, pProcessId, status);
			__leave;
		}
		hParentProcessId = (HANDLE)info.InheritedFromUniqueProcessId;
	}
	__finally
	{
		if (hProcess)
		{
			ZwClose(hProcess);
			hProcess = NULL;
		}
	}
	return hParentProcessId;
}

NTSTATUS	GetProcessCodeSignerByProcessId
(
	HANDLE				pProcessId,
	PS_PROTECTED_SIGNER	*pSigner
)
{
	if (NULL == Config())	return STATUS_UNSUCCESSFUL;
	if (NULL == pSigner)	return STATUS_UNSUCCESSFUL;

	FN_ZwQueryInformationProcess	pZwQueryInformationProcess = Config()->pZwQueryInformationProcess;
	if (NULL == pZwQueryInformationProcess)	return STATUS_BAD_FUNCTION_TABLE;
	if (pProcessId <= (HANDLE)4)			return STATUS_INVALID_PARAMETER;
	if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
		__dlog("%s KeGetCurrentIrql()=%d", __FUNCTION__, KeGetCurrentIrql());
		return STATUS_UNSUCCESSFUL;
	}	

	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	HANDLE				hProcess = NULL;
	OBJECT_ATTRIBUTES	oa = { 0 };
	CLIENT_ID			cid = { 0 };
	ULONG				nNeedSize = 0;
	PS_PROTECTION		protection;

	protection.Level	= 0;
	__try
	{
		*pSigner = PsProtectedSignerNone;
		oa.Length = sizeof(OBJECT_ATTRIBUTES);
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
		cid.UniqueProcess = pProcessId;

#ifndef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_INFORMATION (0x0400)
#endif
		status = ZwOpenProcess(&hProcess, PROCESS_QUERY_INFORMATION, &oa, &cid);
		if (NT_FAILED(status))	{
			__log("%s ZwOpenProcess() failed. status=%08x", __FUNCTION__, status);
			__leave;
		}
		status = pZwQueryInformationProcess(hProcess, ProcessProtectionInformation, &protection, 
					sizeof(protection), &nNeedSize);
		if (NT_FAILED(status))	{
			__log("%s ZwQueryInformationProcess() failed. status=%08x, sizeof(protection)=%d, nNeedSize=%d", 
				__FUNCTION__, status, sizeof(protection), nNeedSize);
			__leave;
		}
		*pSigner	= (PS_PROTECTED_SIGNER)protection.Flags.Signer;
	}
	__finally
	{
		if (hProcess)
		{
			ZwClose(hProcess);
			hProcess = NULL;
		}
		if (NT_SUCCESS(status))
		{

		}
	}
	return status;
}
NTSTATUS	GetProcessInfo
(
	HANDLE				pProcessId,
	PROCESSINFOCLASS	infoClass,
	PUNICODE_STRING		*pStr
)
{
	if (NULL == Config())	return STATUS_UNSUCCESSFUL;
	if( NULL == pStr)		return STATUS_INVALID_PARAMETER;

	FN_ZwQueryInformationProcess	pZwQueryInformationProcess = Config()->pZwQueryInformationProcess;
	if (NULL == pZwQueryInformationProcess)	return STATUS_BAD_FUNCTION_TABLE;
	if (pProcessId <= (HANDLE)4)			return STATUS_INVALID_PARAMETER;
	if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
		__dlog("%s KeGetCurrentIrql()=%d", __FUNCTION__, KeGetCurrentIrql());
		return STATUS_UNSUCCESSFUL;
	}
	if (NULL == pStr)	return STATUS_UNSUCCESSFUL;

	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	HANDLE				hProcess = NULL;
	OBJECT_ATTRIBUTES	oa = { 0 };
	CLIENT_ID			cid = { 0 };
	ULONG				nNeedSize = 0;

	__try
	{
		*pStr = NULL;
		oa.Length = sizeof(OBJECT_ATTRIBUTES);
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
		cid.UniqueProcess = pProcessId;

#ifndef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_INFORMATION (0x0400)
#endif
		status = ZwOpenProcess(&hProcess, PROCESS_QUERY_INFORMATION, &oa, &cid);
		if (!NT_SUCCESS(status))	__leave;

		status = pZwQueryInformationProcess(hProcess, infoClass, NULL, 0, &nNeedSize);
		if (status != STATUS_INFO_LENGTH_MISMATCH)	__leave;
		*pStr = (PUNICODE_STRING)CMemory::Allocate(NonPagedPoolNx, nNeedSize, 'GPIP');
		if (NULL == *pStr)	__leave;
		RtlZeroMemory(*pStr, nNeedSize);
		status = pZwQueryInformationProcess(hProcess, infoClass, *pStr, nNeedSize, &nNeedSize);
		if (!NT_SUCCESS(status))	__leave;
	}
	__finally
	{
		if (hProcess)
		{
			ZwClose(hProcess);
			hProcess = NULL;
		}
		if (NT_SUCCESS(status))
		{

		}
		else
		{
			if (*pStr) {
				CMemory::Free(*pStr);
				*pStr = NULL;
			}
		}
	}
	return status;
}
NTSTATUS	GetProcessImagePathByProcessId
(
	HANDLE				pProcessId,
	PUNICODE_STRING		*pStr
)
{
	if (NULL == Config())	return STATUS_UNSUCCESSFUL;

	FN_ZwQueryInformationProcess	pZwQueryInformationProcess = Config()->pZwQueryInformationProcess;
	if (NULL == pZwQueryInformationProcess)	return STATUS_BAD_FUNCTION_TABLE;
	if (pProcessId <= (HANDLE)4)	{
		if( (HANDLE)0 == pProcessId ) {
			if( CMemory::AllocateUnicodeString(NonPagedPoolNx, pStr, L"System idle process") )
				return STATUS_SUCCESS;
		}
		if( (HANDLE)4 == pProcessId ) {
			if (CMemory::AllocateUnicodeString(NonPagedPoolNx, pStr, L"System"))
				return STATUS_SUCCESS;		
		}
		return STATUS_INVALID_PARAMETER;
	}
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	{
		__dlog("%s KeGetCurrentIrql()=%d", __FUNCTION__, KeGetCurrentIrql());
		return STATUS_UNSUCCESSFUL;
	}
	if( NULL == pStr )	return STATUS_INVALID_PARAMETER;

	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	HANDLE				hProcess = NULL;
	OBJECT_ATTRIBUTES	oa = { 0 };
	CLIENT_ID			cid = { 0 };
	ULONG				nNeedSize = 0;

	__try
	{
		*pStr		= NULL;
		oa.Length   = sizeof(OBJECT_ATTRIBUTES);
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
		cid.UniqueProcess = pProcessId;

#ifndef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_INFORMATION (0x0400)
#endif
		status = ZwOpenProcess(&hProcess, PROCESS_QUERY_INFORMATION, &oa, &cid);
		if (!NT_SUCCESS(status))	{
			__log("%s ZwOpenProcess() failed. PID=%d", __FUNCTION__, pProcessId);
			__leave;
		}
		status = pZwQueryInformationProcess(hProcess, ProcessImageFileName, NULL, 0, &nNeedSize);
		if (status != STATUS_INFO_LENGTH_MISMATCH)	{
			__log("%s pZwQueryInformationProcess() failed. PID=%d", __FUNCTION__, pProcessId);
			__leave;
		}
		*pStr = (PUNICODE_STRING)CMemory::Allocate(NonPagedPoolNx, nNeedSize, 'GPIP');
		if ( NULL == *pStr )	__leave;
		RtlZeroMemory(*pStr, nNeedSize);
		status = pZwQueryInformationProcess(hProcess, ProcessImageFileName, *pStr, nNeedSize, &nNeedSize);
		if (!NT_SUCCESS(status))	{
			__log("%s pZwQueryInformationProcess() failed. PID=%d", __FUNCTION__, pProcessId); 
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
		if (NT_SUCCESS(status))
		{

		}
		else
		{
			if( *pStr )	{
				CMemory::Free(*pStr);
				*pStr	= NULL;
			}
		}
	}
	return status;
}
NTSTATUS	GetProcessImagePathByProcessId
(
	HANDLE							pProcessId,
	PWSTR							pImagePathBuffer,
	size_t							nBufferSize,
	PULONG							pNeedSize
)
{
	if( NULL == Config() )	return STATUS_UNSUCCESSFUL;

	FN_ZwQueryInformationProcess	pZwQueryInformationProcess = Config()->pZwQueryInformationProcess;
	if (NULL == pZwQueryInformationProcess)	return STATUS_BAD_FUNCTION_TABLE;
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	return STATUS_UNSUCCESSFUL;

	if (pProcessId <= (HANDLE)4) {
		RtlStringCbCopyW(pImagePathBuffer, nBufferSize, L"SYSTEM");
		return STATUS_SUCCESS;//STATUS_INVALID_PARAMETER;
	}

	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	HANDLE				hProcess = NULL;
	OBJECT_ATTRIBUTES	oa = { 0 };
	CLIENT_ID			cid = { 0 };
	ULONG				nNeedSize = 0;
	PUNICODE_STRING		pTmpImagePath = NULL;

	__try
	{
		oa.Length = sizeof(OBJECT_ATTRIBUTES);
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
		cid.UniqueProcess = pProcessId;

#ifndef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_INFORMATION (0x0400)
#endif
		status = ZwOpenProcess(&hProcess, PROCESS_QUERY_INFORMATION, &oa, &cid);
		if (!NT_SUCCESS(status))	__leave;

		status = pZwQueryInformationProcess(hProcess, ProcessImageFileName, NULL, 0, &nNeedSize);
		if (status != STATUS_INFO_LENGTH_MISMATCH)	__leave;
		if (NULL == pImagePathBuffer || nBufferSize < nNeedSize)
		{
			if (pNeedSize)	*pNeedSize = nNeedSize;
			status = STATUS_INFO_LENGTH_MISMATCH;
			__leave;
		}
		pTmpImagePath = (PUNICODE_STRING)ExAllocatePoolWithTag(NonPagedPoolNx, nNeedSize, 'GPIP');
		if (pTmpImagePath == NULL)	__leave;
		RtlZeroMemory(pTmpImagePath, nNeedSize);

		status = pZwQueryInformationProcess(hProcess, ProcessImageFileName, pTmpImagePath, nNeedSize, &nNeedSize);
		if (!NT_SUCCESS(status))	__leave;
		status = RtlStringCbCopyNW(pImagePathBuffer, nBufferSize, pTmpImagePath->Buffer, pTmpImagePath->Length);
	}
	__finally
	{
		if (pTmpImagePath)
		{
			ExFreePoolWithTag(pTmpImagePath, 'GPIP');
			pTmpImagePath = NULL;
		}
		if (hProcess)
		{
			ZwClose(hProcess);
			hProcess = NULL;
		}
	}
	return status;
}