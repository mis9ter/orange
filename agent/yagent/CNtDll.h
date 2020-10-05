#pragma once

#include <winternl.h>
#include "MD5/Global.h"
#include "MD5/MD5.h"

#define NT_SUCCESS(Status)				(((NTSTATUS)(Status)) >= 0)

typedef LONG NTSTATUS;
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)    // ntsubauth
#define STATUS_INFO_LENGTH_MISMATCH		0xC0000004  
#define STATUS_BAD_FUNCTION_TABLE       ((NTSTATUS)0xC00000FFL)
#define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001L)
#define STATUS_INVALID_CID               ((NTSTATUS)0xC000000BL)
#define STATUS_ACCESS_DENIED             ((NTSTATUS)0xC0000022L)
/*
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
*/

typedef NTSTATUS(WINAPI* FN_ZwQuerySystemInformation)
(
	SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength
);
typedef NTSTATUS(NTAPI* FN_ZwQueryInformationProcess)
(
	IN HANDLE           ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID           ProcessInformation,
	IN ULONG            ProcessInformationLength,
	OUT PULONG          ReturnLength
);
typedef NTSTATUS(NTAPI* FN_ZwQueryInformationThread)
(
	HANDLE          ThreadHandle,
	THREADINFOCLASS ThreadInformationClass,
	PVOID           ThreadInformation,
	ULONG           ThreadInformationLength,
	PULONG          ReturnLength
);

//typedef struct _CLIENT_ID {
//	HANDLE UniqueProcess;
//	HANDLE UniqueThread;
//} CLIENT_ID;
typedef CLIENT_ID* PCLIENT_ID;
typedef	NTSTATUS(NTAPI * FN_ZwOpenProcess)
(
	PHANDLE            ProcessHandle,
	ACCESS_MASK        DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PCLIENT_ID         ClientId
);
typedef	NTSTATUS	(NTAPI *FN_ZwClose)
(
	HANDLE Handle
);


#define NTDLL_PROC(ptr, name)	ptr=(FN_##name)GetProcAddress(GetModuleHandle(L"ntdll.dll"), #name)


class CNtDll
	:
	virtual	public	CAppLog
{
public:
	CNtDll()		
	{
		ZeroMemory(&m_table, sizeof(m_table));
		NTDLL_PROC(m_table.pZwQuerySystemInformation, ZwQuerySystemInformation);
		NTDLL_PROC(m_table.pZwQueryInformationProcess, ZwQueryInformationProcess);
		NTDLL_PROC(m_table.pZwQueryInformationThread, ZwQueryInformationThread);
		NTDLL_PROC(m_table.pZwOpenProcess, ZwOpenProcess);
		NTDLL_PROC(m_table.pZwClose, ZwClose);

		m_dwBootId	= YAgent::GetBootId();
		YAgent::GetMachineGuid(m_szMachineGuid, sizeof(m_szMachineGuid));

		printf("%d %ws\n", m_dwBootId, m_szMachineGuid);
	}
	~CNtDll() {
	
	}
	NTSTATUS	WINAPI	ZwQuerySystemInformation
	(
		SYSTEM_INFORMATION_CLASS SystemInformationClass,
		PVOID SystemInformation,
		ULONG SystemInformationLength,
		PULONG ReturnLength
	) {
		if( m_table.pZwQuerySystemInformation )
			return m_table.pZwQuerySystemInformation(SystemInformationClass, 
				SystemInformation, SystemInformationLength, ReturnLength);
		return STATUS_BAD_FUNCTION_TABLE;
	}
	NTSTATUS	NTAPI	ZwQueryInformationProcess
	(
		IN HANDLE           ProcessHandle,
		IN PROCESSINFOCLASS ProcessInformationClass,
		OUT PVOID           ProcessInformation,
		IN ULONG            ProcessInformationLength,
		OUT PULONG          ReturnLength
	) {
		if( m_table.pZwQueryInformationProcess)
			return m_table.pZwQueryInformationProcess(ProcessHandle, ProcessInformationClass, 
				ProcessInformation, ProcessInformationLength, ReturnLength);
		return STATUS_BAD_FUNCTION_TABLE;
	}
	NTSTATUS	NTAPI	ZwQueryInformationThread
	(
		HANDLE          ThreadHandle,
		THREADINFOCLASS ThreadInformationClass,
		PVOID           ThreadInformation,
		ULONG           ThreadInformationLength,
		PULONG          ReturnLength
	) {
		if (m_table.pZwQueryInformationThread) {
			return m_table.pZwQueryInformationThread( ThreadHandle, 
				ThreadInformationClass, ThreadInformation, 
				ThreadInformationLength, ReturnLength);
		}
		return STATUS_BAD_FUNCTION_TABLE;
	}
	NTSTATUS	GetProcessInfo
	(
		HANDLE				PID,
		PROCESSINFOCLASS	infoClass,
		PVOID				pValue,
		ULONG				nSize,
		ULONG				*pReturnedSize
	)
	{
		if (NULL == m_table.pZwQueryInformationProcess)	return STATUS_BAD_FUNCTION_TABLE;
		if (NULL == m_table.pZwOpenProcess)				return STATUS_BAD_FUNCTION_TABLE;
		if (NULL == m_table.pZwClose)					return STATUS_BAD_FUNCTION_TABLE;

		NTSTATUS			status = STATUS_UNSUCCESSFUL;
		HANDLE				hProcess = NULL;
		OBJECT_ATTRIBUTES	oa = { 0 };
		CLIENT_ID			cid = { 0 };

		__try
		{
			oa.Length = sizeof(OBJECT_ATTRIBUTES);
			InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
			cid.UniqueProcess = PID;

			status = m_table.pZwOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION, &oa, &cid);
			if (!NT_SUCCESS(status)) {
				Log("%s ZwOpenProcess() failed.", __FUNCTION__);
				__leave;
			}
			status = m_table.pZwQueryInformationProcess(hProcess, infoClass, pValue, nSize, pReturnedSize);
			if (NT_SUCCESS(status)) {

			}
			else {
				Log("%s ZwQueryInformationProcess() failed.", __FUNCTION__);
			}
		}
		__finally
		{
			if (hProcess)
			{
				m_table.pZwClose(hProcess);
				hProcess = NULL;
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
		if (NULL == m_table.pZwQueryInformationProcess)	return STATUS_BAD_FUNCTION_TABLE;
		if (NULL == m_table.pZwOpenProcess)				return STATUS_BAD_FUNCTION_TABLE;
		if (NULL == m_table.pZwClose)					return STATUS_BAD_FUNCTION_TABLE;
		if (NULL == pStr)								return STATUS_UNSUCCESSFUL;

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
			status = m_table.pZwOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION, &oa, &cid);
			if (!NT_SUCCESS(status))	__leave;

			status = m_table.pZwQueryInformationProcess(hProcess, infoClass, NULL, 0, &nNeedSize);
			if (status != STATUS_INFO_LENGTH_MISMATCH)	__leave;
			*pStr = (PUNICODE_STRING)malloc(nNeedSize);
			if (NULL == *pStr)	__leave;
			ZeroMemory(*pStr, nNeedSize);
			status = m_table.pZwQueryInformationProcess(hProcess, infoClass, *pStr, nNeedSize, &nNeedSize);
			if (!NT_SUCCESS(status))	__leave;
		}
		__finally
		{
			if (hProcess)
			{
				m_table.pZwClose(hProcess);
				hProcess = NULL;
			}
			if (NT_SUCCESS(status))
			{

			}
			else
			{
				if (*pStr) {
					free(*pStr);
					*pStr = NULL;
				}
			}
		}
		return status;
	}
	typedef struct PROCESS_BASIC_INFORMATION2 {
		NTSTATUS ExitStatus;
		PPEB PebBaseAddress;
		ULONG_PTR AffinityMask;
		KPRIORITY BasePriority;
		ULONG_PTR UniqueProcessId;
		ULONG_PTR InheritedFromUniqueProcessId;
	} PROCESS_BASIC_INFORMATION2, * PPROCESS_BASIC_INFORMATION2;
	NTSTATUS	GetParentProcessId(IN HANDLE PID, OUT HANDLE* PPID)
	{
		if (NULL == m_table.pZwQueryInformationProcess)	return STATUS_BAD_FUNCTION_TABLE;
		if (NULL == m_table.pZwOpenProcess)				return STATUS_BAD_FUNCTION_TABLE;
		if (NULL == m_table.pZwClose)					return STATUS_BAD_FUNCTION_TABLE;

		NTSTATUS			status = STATUS_UNSUCCESSFUL;
		HANDLE				hProcess = NULL;
		OBJECT_ATTRIBUTES	oa = { 0 };
		CLIENT_ID			cid = { 0 };

		__try
		{
			oa.Length = sizeof(OBJECT_ATTRIBUTES);
			InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
			cid.UniqueProcess = PID;

			status = m_table.pZwOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION, &oa, &cid);
			if (!NT_SUCCESS(status))	__leave;

			PROCESS_BASIC_INFORMATION2	info;
			status = m_table.pZwQueryInformationProcess(hProcess, ProcessBasicInformation, &info, sizeof(info), NULL);
			if (NT_SUCCESS(status)) {
				*PPID = (HANDLE)info.InheritedFromUniqueProcessId;
			}
		}
		__finally
		{
			if (hProcess)
			{
				m_table.pZwClose(hProcess);
				hProcess = NULL;
			}
		}
		return status;
	}
	NTSTATUS	GetProcessParams(IN HANDLE PID, 
		OUT HANDLE	*PPID,
		OUT PWSTR	pPath,	IN	DWORD	dwPathSize,
		OUT PWSTR	pCmdLine, IN DWORD dwCmdLineSize		
	)
	{
		//	winapi - How to query a running process for it's parameters list? (windows, C++) - Stack Overflow
		//	https://stackoverflow.com/questions/6520428/how-to-query-a-running-process-for-its-parameters-list-windows-c
		if (NULL == m_table.pZwQueryInformationProcess)	return STATUS_BAD_FUNCTION_TABLE;
		if (NULL == m_table.pZwOpenProcess)				return STATUS_BAD_FUNCTION_TABLE;
		if (NULL == m_table.pZwClose)					return STATUS_BAD_FUNCTION_TABLE;

		NTSTATUS			status = STATUS_UNSUCCESSFUL;
		HANDLE				hProcess = NULL;
		OBJECT_ATTRIBUTES	oa = { 0 };
		CLIENT_ID			cid = { 0 };

		__try
		{
			oa.Length = sizeof(OBJECT_ATTRIBUTES);
			InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
			cid.UniqueProcess = PID;

			status = m_table.pZwOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION, &oa, &cid);
			if (!NT_SUCCESS(status))	__leave;

			PROCESS_BASIC_INFORMATION2	info;
			status = m_table.pZwQueryInformationProcess(hProcess, ProcessBasicInformation, &info, sizeof(info), NULL);
			if (NT_SUCCESS(status)) {
				*PPID = (HANDLE)info.InheritedFromUniqueProcessId;
				//	[TODO]	PEB를 읽는 도중에 사라지면 어쩌지?
				//	ZwOpenProcess를 하면 ZwClose 전까지유지해주려나?
				if( info.PebBaseAddress && info.PebBaseAddress->ProcessParameters ) {

					//StringCbPrintf(pPath, dwPathSize, L"%Zw", &info.PebBaseAddress->ProcessParameters->ImagePathName);
					//StringCbPrintf(pCmdLine, dwCmdLineSize, L"%Zw", &info.PebBaseAddress->ProcessParameters->CommandLine);
				}
			}
		}
		__finally
		{
			if (hProcess)
			{
				m_table.pZwClose(hProcess);
				hProcess = NULL;
			}
		}
		return status;
	}
	PCWSTR		UUID2String(IN UUID* p, PWSTR pValue, DWORD dwSize) {
		StringCbPrintf(pValue, dwSize,
			L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			p->Data1, p->Data2, p->Data3,
			p->Data4[0], p->Data4[1], p->Data4[2], p->Data4[3],
			p->Data4[4], p->Data4[5], p->Data4[6], p->Data4[7]);
		return pValue;
	}
	NTSTATUS	GetProcGuid(
		IN	HANDLE			PID,
		IN	HANDLE			PPID,
		IN	PCWSTR			pImagePath,
		OUT	UUID			*pGuid)
	{
		NTSTATUS		status = STATUS_SUCCESS;
		WCHAR			procGuid[1024]	= L"";
		/*
			원래 계획은 부모ID까지 넣는 것인데
			부모의 ProcGuid를 구하려 보니 조부모의 존재가 불확실하다.
		*/
		if (NULL == PPID)	GetParentProcessId(PID, &PPID);

		KERNEL_USER_TIMES	times;
		GetProcessInfo((HANDLE)PID, (PROCESSINFOCLASS)4, &times, sizeof(times), NULL);

		StringCbPrintf(procGuid, sizeof(procGuid), L"%ws.%d.%d.%d.%d.%d.%ws",
			m_szMachineGuid,
			m_dwBootId, 	//	bootid
			times.CreateTime.HighPart,
			times.CreateTime.LowPart,
			PPID,
			PID,
			pImagePath);
		int	nSize = 0;
		for (PWSTR p = procGuid; *p; p++) {
			*p = towlower(*p);
			nSize++;
		}
		MD5_CTX			context;
		unsigned char	sum[16];        /* Sum data */

		MD5Init(&context);
		MD5Update(&context, (unsigned char*)(PWSTR)procGuid, nSize * sizeof(WCHAR));
		MD5Final(sum, &context);

		if (pGuid) {
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
		return status;
	}
private:
	WCHAR					m_szMachineGuid[40];
	DWORD					m_dwBootId;

	struct {
		FN_ZwQuerySystemInformation		pZwQuerySystemInformation;
		FN_ZwQueryInformationProcess	pZwQueryInformationProcess;
		FN_ZwQueryInformationThread		pZwQueryInformationThread;
		FN_ZwOpenProcess				pZwOpenProcess;
		FN_ZwClose						pZwClose;
	} m_table;
};
