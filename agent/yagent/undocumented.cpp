#include "Framework.h"

#include <cstdio>
#include <vector>
#include <Windows.h>

#include "undocumented.h"

using pNtQueryInformationProcess = NTSTATUS(NTAPI *)(HANDLE ProcessHandle,
	PROCESS_INFORMATION_CLASS_FULL ProcessInformationClass, PVOID ProcessInformation,
	ULONG ProcessInformationLength, PULONG ReturnLength);
pNtQueryInformationProcess NtQueryInformationProcess = nullptr;

void Modules(const HANDLE hProcess, PVOID pContext, ModuleListCallback pCallback)
{
	PROCESS_BASIC_INFORMATION procBasicInfo = { 0 };
	ULONG ulRetLength = 0;
	NTSTATUS ntStatus = NtQueryInformationProcess(hProcess,
		PROCESS_INFORMATION_CLASS_FULL::ProcessBasicInformation, &procBasicInfo,
		sizeof(PROCESS_BASIC_INFORMATION), &ulRetLength);
	if (ntStatus != STATUS_SUCCESS)
	{
		//p->Log("Could not get process information. Status = %X",
		//	ntStatus);
		return;
	}

	PEB procPeb = { 0 };
	SIZE_T ulBytesRead = 0;
	bool bRet = BOOLIFY(ReadProcessMemory(hProcess, (LPCVOID)procBasicInfo.PebBaseAddress, &procPeb,
		sizeof(PEB), &ulBytesRead));
	if (!bRet)
	{
		//p->Log("Failed to read PEB from process. Error = %X",
		//	GetLastError());
		return;
	}

	PEB_LDR_DATA pebLdrData = { 0 };
	bRet = BOOLIFY(ReadProcessMemory(hProcess, (LPCVOID)procPeb.Ldr, &pebLdrData, sizeof(PEB_LDR_DATA),
		&ulBytesRead));
	if (!bRet)
	{
		//p->Log("Failed to read module list from process. Error = %X",
		//	GetLastError());
		return;
	}

	LIST_ENTRY *pLdrListHead = (LIST_ENTRY *)pebLdrData.InLoadOrderModuleList.Flink;
	LIST_ENTRY *pLdrCurrentNode = pebLdrData.InLoadOrderModuleList.Flink;
	do
	{
		LDR_DATA_TABLE_ENTRY lstEntry = { 0 };
		bRet = BOOLIFY(ReadProcessMemory(hProcess, (LPCVOID)pLdrCurrentNode, &lstEntry,
			sizeof(LDR_DATA_TABLE_ENTRY), &ulBytesRead));
		if (!bRet)
		{
			//p->Log("Could not read list entry from LDR list. Error = %X",
			//	GetLastError());
			return;
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
				if( false == pCallback(pContext, &module) ) 
					break;			
			}
		}
	} while (pLdrListHead != pLdrCurrentNode);
}
void	GetModules(DWORD PID, PVOID pContext, ModuleListCallback pCallback)
{
	HMODULE hModule = GetModuleHandle(L"ntdll.dll");
	NtQueryInformationProcess =
		(pNtQueryInformationProcess)GetProcAddress(hModule, "NtQueryInformationProcess");
	if (NtQueryInformationProcess == nullptr)
	{
		return;
	}
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, PID);
	if (hProcess == INVALID_HANDLE_VALUE)
	{
		return;
	}
	Modules(hProcess, pContext, pCallback);
	CloseHandle(hProcess);
}