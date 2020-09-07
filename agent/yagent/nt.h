#pragma once

#include <winternl.h>

#define NT_SUCCESS(Status)				(((NTSTATUS)(Status)) >= 0)

typedef LONG NTSTATUS;
#define STATUS_INFO_LENGTH_MISMATCH		0xC0000004  
#define STATUS_BAD_FUNCTION_TABLE       ((NTSTATUS)0xC00000FFL)

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

typedef NTSTATUS(WINAPI* FN_NtQuerySystemInformation)
(
	SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength
);
typedef NTSTATUS(NTAPI* FN_NtQueryInformationProcess)
(
	IN HANDLE           ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID           ProcessInformation,
	IN ULONG            ProcessInformationLength,
	OUT PULONG          ReturnLength
);
typedef NTSTATUS(NTAPI* FN_NtQueryInformationThread)
(
	HANDLE          ThreadHandle,
	THREADINFOCLASS ThreadInformationClass,
	PVOID           ThreadInformation,
	ULONG           ThreadInformationLength,
	PULONG          ReturnLength
);

#define NTDLL_PROC(ptr, name)	ptr=(FN_##name)GetProcAddress(GetModuleHandle(L"ntdll.dll"), #name)


class CNtDll
{
public:
	CNtDll() :
		m_pNtQuerySystemInformation(NULL)
		
	{
		NTDLL_PROC(m_pNtQuerySystemInformation, NtQuerySystemInformation);
		NTDLL_PROC(m_pNtQueryInformationProcess, NtQueryInformationProcess);
		NTDLL_PROC(m_pNtQueryInformationThread, NtQueryInformationThread);
	}
	~CNtDll() {
	
	}
	NTSTATUS	WINAPI	NtQuerySystemInformation
	(
		SYSTEM_INFORMATION_CLASS SystemInformationClass,
		PVOID SystemInformation,
		ULONG SystemInformationLength,
		PULONG ReturnLength
	) {
		if( m_pNtQuerySystemInformation )
			return m_pNtQuerySystemInformation(SystemInformationClass, 
				SystemInformation, SystemInformationLength, ReturnLength);
		return STATUS_BAD_FUNCTION_TABLE;
	}
	NTSTATUS	NTAPI	NtQueryInformationProcess
	(
		IN HANDLE           ProcessHandle,
		IN PROCESSINFOCLASS ProcessInformationClass,
		OUT PVOID           ProcessInformation,
		IN ULONG            ProcessInformationLength,
		OUT PULONG          ReturnLength
	) {
		if( m_pNtQueryInformationProcess)
			return m_pNtQueryInformationProcess(ProcessHandle, ProcessInformationClass, 
				ProcessInformation, ProcessInformationLength, ReturnLength);
		return STATUS_BAD_FUNCTION_TABLE;
	}
	NTSTATUS	NTAPI	NtQueryInformationThread
	(
		HANDLE          ThreadHandle,
		THREADINFOCLASS ThreadInformationClass,
		PVOID           ThreadInformation,
		ULONG           ThreadInformationLength,
		PULONG          ReturnLength
	) {
		if (m_pNtQueryInformationThread) {
			return m_pNtQueryInformationThread( ThreadHandle, 
				ThreadInformationClass, ThreadInformation, 
				ThreadInformationLength, ReturnLength);
		}
		return STATUS_BAD_FUNCTION_TABLE;
	}


private:
	FN_NtQuerySystemInformation		m_pNtQuerySystemInformation;
	FN_NtQueryInformationProcess	m_pNtQueryInformationProcess;
	FN_NtQueryInformationThread		m_pNtQueryInformationThread;
};
