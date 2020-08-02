#include "yfilter.h"

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
		if (!NT_SUCCESS(status))	__leave;

		PROCESS_BASIC_INFORMATION	info = { 0, };
		status = pZwQueryInformationProcess(hProcess, ProcessBasicInformation, &info, sizeof(info), &nNeedSize);
		if (status != STATUS_INFO_LENGTH_MISMATCH)	__leave;

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
NTSTATUS	GetProcessImagePathByProcessId
(
	HANDLE				pProcessId,
	PUNICODE_STRING		*pStr
)
{
	if (NULL == Config())	return STATUS_UNSUCCESSFUL;

	FN_ZwQueryInformationProcess	pZwQueryInformationProcess = Config()->pZwQueryInformationProcess;
	if (NULL == pZwQueryInformationProcess)	return STATUS_BAD_FUNCTION_TABLE;
	if (pProcessId <= (HANDLE)4)			return STATUS_INVALID_PARAMETER;
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	{
		__dlog("%s KeGetCurrentIrql()=%d", __FUNCTION__, KeGetCurrentIrql());
		return STATUS_UNSUCCESSFUL;
	}
	if( NULL == pStr )	return STATUS_UNSUCCESSFUL;

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
		if (!NT_SUCCESS(status))	__leave;

		status = pZwQueryInformationProcess(hProcess, ProcessImageFileName, NULL, 0, &nNeedSize);
		if (status != STATUS_INFO_LENGTH_MISMATCH)	__leave;
		*pStr = (PUNICODE_STRING)CMemory::Allocate(NonPagedPoolNx, nNeedSize, 'GPIP');
		if ( NULL == *pStr )	__leave;
		RtlZeroMemory(*pStr, nNeedSize);
		status = pZwQueryInformationProcess(hProcess, ProcessImageFileName, *pStr, nNeedSize, &nNeedSize);
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
	if (pProcessId <= (HANDLE)4)			return STATUS_INVALID_PARAMETER;
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	return STATUS_UNSUCCESSFUL;

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