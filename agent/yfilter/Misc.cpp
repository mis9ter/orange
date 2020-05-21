#include "yfilter.h"

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
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	return STATUS_UNSUCCESSFUL;
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