#include "Framework.h"

#include <cstdio>
#include <vector>
#include <Windows.h>

#include "undocumented.h"
#include <functional>
#include "CAppLog.h"
#include "yagent.string.h"

CAppLog	g_log(L"orange.log");

using pNtQueryInformationProcess = NTSTATUS(NTAPI *)(HANDLE ProcessHandle,
	PROCESS_INFORMATION_CLASS_FULL ProcessInformationClass, PVOID ProcessInformation,
	ULONG ProcessInformationLength, PULONG ReturnLength);
pNtQueryInformationProcess NtQueryInformationProcess = nullptr;

using	pNtReadVirtualMemory	= NTSTATUS(NTAPI *)(
	_In_ HANDLE ProcessHandle,
	_In_opt_ PVOID BaseAddress,
	_Out_writes_bytes_(BufferSize) PVOID Buffer,
	_In_ SIZE_T BufferSize,
	_Out_opt_ PSIZE_T NumberOfBytesRead
);
pNtReadVirtualMemory	NtReadVirtualMemory	= nullptr;

#define PTR_ADD_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) + (ULONG_PTR)(Offset)))

bool Modules(const HANDLE hProcess, PVOID pContext, ModuleListCallback pCallback)
{
	
	PROCESS_BASIC_INFORMATION procBasicInfo = { 0 };
	ULONG ulRetLength = 0;
	NTSTATUS ntStatus = NtQueryInformationProcess(hProcess,
		PROCESS_INFORMATION_CLASS_FULL::ProcessBasicInformation, &procBasicInfo,
		sizeof(PROCESS_BASIC_INFORMATION), &ulRetLength);
	if (ntStatus != STATUS_SUCCESS)
	{
		g_log.Log("Could not get process information. Status = %X", ntStatus);
		return false;
	}

	PEB procPeb = { 0 };
	SIZE_T ulBytesRead = 0;

	PEB_LDR_DATA pebLdrData = { 0 };

	bool bRet = BOOLIFY(ReadProcessMemory(hProcess, (LPCVOID)procBasicInfo.PebBaseAddress, &procPeb,
		sizeof(PEB), &ulBytesRead));
	if (!bRet)
	{
		g_log.Log("Failed to read PEB from process. Error = %X\n", GetLastError());
		return false;
	}

	
	bRet = BOOLIFY(ReadProcessMemory(hProcess, (LPCVOID)procPeb.Ldr, &pebLdrData, sizeof(PEB_LDR_DATA),
		&ulBytesRead));
	if (!bRet)
	{
		g_log.Log("Failed to read module list from process. Error = %X\n",	GetLastError());
		return false;
	}

	LIST_ENTRY *pLdrListHead = (LIST_ENTRY *)pebLdrData.InLoadOrderModuleList.Flink;
	LIST_ENTRY *pLdrCurrentNode = pebLdrData.InLoadOrderModuleList.Flink;
	for( ULONG i = 0; i < 1024 ; i++ )
	{
		LDR_DATA_TABLE_ENTRY lstEntry = { 0 };
		bRet = BOOLIFY(ReadProcessMemory(hProcess, (LPCVOID)pLdrCurrentNode, &lstEntry,
			sizeof(LDR_DATA_TABLE_ENTRY), &ulBytesRead));
		if (!bRet)
		{
			g_log.Log("Could not read list entry from LDR list. Error = %X\n", GetLastError());
			return false;
		}
		pLdrCurrentNode = lstEntry.InLoadOrderLinks.Flink;

		MODULE	module;
		if (lstEntry.FullDllName.Length > 0)
		{
			bRet = BOOLIFY(ReadProcessMemory(hProcess, (LPCVOID)lstEntry.FullDllName.Buffer, &module.FullName,
				lstEntry.FullDllName.Length, &ulBytesRead));
		}
		if (lstEntry.BaseDllName.Length > 0)
		{
			bRet = BOOLIFY(ReadProcessMemory(hProcess, (LPCVOID)lstEntry.BaseDllName.Buffer, &module.BaseName,
				lstEntry.BaseDllName.Length, &ulBytesRead));
		}
		if (lstEntry.DllBase != nullptr && lstEntry.SizeOfImage != 0)
		{
			if( pCallback ) {
				module.ImageBase	= lstEntry.DllBase;
				module.EntryPoint	= lstEntry.EntryPoint;
				module.ImageSize	= lstEntry.SizeOfImage;
				if( false == pCallback(i, pContext, &module) ) 
					break;			
			}
		}
		if (pLdrListHead == pLdrCurrentNode)	break;
	}
	return true;
}
typedef struct _OBJECT_ATTRIBUTES
{
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor; // PSECURITY_DESCRIPTOR;
	PVOID SecurityQualityOfService; // PSECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

typedef const OBJECT_ATTRIBUTES* PCOBJECT_ATTRIBUTES;

#define InitializeObjectAttributes(p, n, a, r, s) { \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
    (p)->RootDirectory = r; \
    (p)->Attributes = a; \
    (p)->ObjectName = n; \
    (p)->SecurityDescriptor = s; \
    (p)->SecurityQualityOfService = NULL; \
    }
typedef struct _CLIENT_ID
{
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
} CLIENT_ID, * PCLIENT_ID;

using pNtOpenProcess	= NTSTATUS(NTAPI *)(
	_Out_ PHANDLE ProcessHandle,
	_In_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_opt_ PCLIENT_ID ClientId
);

pNtOpenProcess	NtOpenProcess = nullptr;

bool	GetModules(DWORD PID, PVOID pContext, ModuleListCallback pCallback)
{
	puts(__func__);
	HMODULE hModule = GetModuleHandle(L"ntdll.dll");
	NtQueryInformationProcess =
		(pNtQueryInformationProcess)GetProcAddress(hModule, "NtQueryInformationProcess");
	NtOpenProcess	= (pNtOpenProcess)GetProcAddress(hModule, "NtOpenProcess");

	if (NtQueryInformationProcess == nullptr)
	{
		puts("NtQueryInformationProcess is null.");
		return false;
	}
	OBJECT_ATTRIBUTES objectAttributes;
	InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);

	/*
	HANDLE		hProcess	= NULL;
	NTSTATUS	status;
	CLIENT_ID	clientId;

	clientId.UniqueProcess	= (HANDLE)PID;
	clientId.UniqueThread	= 0;
	status	= NtOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_VM_READ, 
				&objectAttributes, &clientId);
	if(STATUS_SUCCESS != status )	{
		printf("NtOpenProcess() failed. status=%X.\n", status);
		return false;
	}
	*/
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, PID);
	if (hProcess == INVALID_HANDLE_VALUE)
	{
		printf("OpenProcess() failed. code=%d.\n", GetLastError());
		return false;
	}
	//Modules(hProcess, pContext, pCallback);
	CloseHandle(hProcess);
	return true;
}

using	pNtQuerySystemInformation	= NTSTATUS(NTAPI*)(
	_In_ SYSTEM_INFORMATION_CLASS_FULL  SystemInformationClass,
	_Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
	_In_ ULONG SystemInformationLength,
	_Out_opt_ PULONG ReturnLength
	);
pNtQuerySystemInformation	NtQuerySystemInformation = nullptr;

typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
	HANDLE Section;
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, * PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES
{
	ULONG NumberOfModules;
	RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, * PRTL_PROCESS_MODULES;

#define STATUS_INFO_LENGTH_MISMATCH	0xC0000004
#define STATUS_UNSUCCESSFUL         ((NTSTATUS)0xC0000001L)

int		GetSystemModules(PVOID pContext, ModuleListCallback pCallback) {
	NTSTATUS	status	= STATUS_UNSUCCESSFUL;
	PVOID		pBuffer	= NULL;
	ULONG		nBufferSize	= 1024 * 64;

	HMODULE hModule = GetModuleHandle(L"ntdll.dll");
	NtQuerySystemInformation =
		(pNtQuerySystemInformation)GetProcAddress(hModule, "NtQuerySystemInformation");

	try {
		pBuffer	= new char[nBufferSize];
		status = NtQuerySystemInformation(
			SystemModuleInformation, pBuffer, nBufferSize, &nBufferSize
		);
		if (status == STATUS_INFO_LENGTH_MISMATCH) {
			delete pBuffer;
			pBuffer	= new char[nBufferSize];
			g_log.Log("%-32s nBufferSize:%d", __func__, nBufferSize);
			status = NtQuerySystemInformation(
				SystemModuleInformation, pBuffer, nBufferSize, &nBufferSize
			);
		}
		if (NT_SUCCESS(status)) {

		}
		else {
			g_log.Log("%-32s NtQuerySystemInformation() failed. status=%x", __func__, status);
		}
	}
	catch (std::exception& e) {
		g_log.Log("%-32s %s", __func__, e.what());
	}
	int			nCount	= 0;
	if( NT_SUCCESS(status))	{
		PRTL_PROCESS_MODULES	p	= (PRTL_PROCESS_MODULES)pBuffer;
		g_log.Log("%-32s NumberOfModules:%d", __func__, p->NumberOfModules);
		for (ULONG i = 0; i < p->NumberOfModules; i++) {
			MODULE	m;
			ZeroMemory(&m, sizeof(m));
			PCWSTR	pName;
			StringCbCopy(m.FullName, sizeof(m.FullName), __utf16((char *)p->Modules[i].FullPathName));
			pName	= wcsrchr(m.FullName, L'\\');
			pName	= pName? pName + 1 : m.FullName;
			StringCbCopy(m.BaseName, sizeof(m.BaseName), pName);
			m.ImageBase		= p->Modules[i].ImageBase;
			m.ImageSize		= p->Modules[i].ImageSize;

			if( pCallback )	pCallback(i, pContext, &m);
		}	
	}
	if( pBuffer )	delete pBuffer;
	return nCount;
}
int		GetProcessModules(HANDLE hProcess, PVOID pContext, ModuleListCallback pCallback)
{
	HMODULE hModule = GetModuleHandle(L"ntdll.dll");
	NtQueryInformationProcess =
		(pNtQueryInformationProcess)GetProcAddress(hModule, "NtQueryInformationProcess");
	NtReadVirtualMemory =
		(pNtReadVirtualMemory)GetProcAddress(hModule, "NtReadVirtualMemory");
	if (NtQueryInformationProcess == nullptr)
	{
		g_log.Log("NtQueryInformationProcess is null.");
		return 0;
	}
	return Modules(hProcess, pContext, pCallback);
}