#pragma once
/*
	2020/09/07	Beom

	이전부터 존재하고 있던 프로세스, 모듈, 스레드 정보를 얻는다.
*/
#include <TlHelp32.h>
#include <psapi.h>
#include <functional>
#include "CNtDll.h"

#define __nextptr(base, offset) ((PVOID)((ULONG_PTR) (base) + (ULONG_PTR) (offset))) 

typedef std::function<bool(const char* pData, DWORD dwSize)>	OutputCallback;

#pragma warning(disable:4311)
#pragma warning(disable:4302)
class CPreProcess
	:
	virtual	public	CAppLog,
	virtual	public	CNtDll

{
public:
	CPreProcess() {

	}
	~CPreProcess() {

	}

	void			Check2(
		std::function<void(
			UUID * pProcGuid, DWORD PID, DWORD PPID, PCWSTR pPath
		)> pCallback = NULL
	
	) {
		ULONG		nNeededSize = 0;
		//PVOID		pBuf		= NULL;
		NTSTATUS	status		= ZwQuerySystemInformation(
									SystemProcessInformation, NULL, 0, &nNeededSize);
		if (STATUS_INFO_LENGTH_MISMATCH == status) {
			//	할당 받아 봅시다.
			//	단, 프로세스 정보는 수시로 변한다는거. 이 다음에 호출하면 더 큰 크기를 원할 수도 있다.
			ULONG	nSize		= nNeededSize * 2;
			PVOID	pBuf		= malloc(nSize);
			WCHAR	szPath[AGENT_PATH_SIZE]	= L"";
			if (pBuf) {
				HANDLE			hDebug = SetDebugPrivilege();

				if (NT_SUCCESS(status = ZwQuerySystemInformation(SystemProcessInformation, pBuf, nSize, &nNeededSize))) {
					SYSTEM_PROCESS_INFORMATION* p;
					for (p = (SYSTEM_PROCESS_INFORMATION*)pBuf; p->NextEntryOffset;
						p = (SYSTEM_PROCESS_INFORMATION*)__nextptr(p, p->NextEntryOffset)) {
						HANDLE	PID		= p->UniqueProcessId;
						HANDLE	PPID	= 0;
						
						GetParentProcessId(PID, &PPID);
						PUNICODE_STRING	pImageFileName = NULL;
						NTSTATUS		status;
						if ((HANDLE)0 == PID) {
							StringCbCopy(szPath, sizeof(szPath), L"[System Process]");
						}
						else if ((HANDLE)4 == PID) {
							StringCbCopy(szPath, sizeof(szPath), L"System");
						}	
						else {
							if (NT_SUCCESS(status = GetProcessInfo(PID, ProcessImageFileName, &pImageFileName))) {
								StringCbPrintf(szPath, sizeof(szPath), L"%wZ", pImageFileName);
								free(pImageFileName);
							}
							else {
								Log("  GetProcessInfo() failed. status=%08x", status);
								StringCbCopy(szPath, sizeof(szPath), L"");
							}
						}		
						UUID	ProcGuid;
						GetProcGuid(PID, PPID, szPath, &ProcGuid);
						if( pCallback )
							pCallback(&ProcGuid, (DWORD)PID, (DWORD)PPID, szPath);
					}
				}
				else {
					Log("%s NtQuerySystemInformation() failed. status=%08x", __FUNCTION__, status);
				}
				if (hDebug)	UnsetDebugPrivilege(hDebug);
				free(pBuf);
			}
		}
	}
	void			Check() {

		HANDLE		hSnapshot	= CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (INVALID_HANDLE_VALUE == hSnapshot)	return;
		PROCESSENTRY32	entry	= {0, };
		BOOL			bRet	= FALSE;
		wchar_t			szPath[AGENT_PATH_SIZE]	= L"";

		entry.dwSize	= sizeof(entry);

		HANDLE			hDebug	= SetDebugPrivilege();
		__try
		{
			if( NULL == hDebug )	__leave;
			for (bRet = Process32First(hSnapshot, &entry); bRet; 
				bRet = Process32Next(hSnapshot, &entry)) {

				DWORD	PPID	= entry.th32ParentProcessID;
				DWORD	PID		= entry.th32ProcessID;

				GetProcessPath(PID, szPath, sizeof(szPath));
				Log("%s %6d %6d %ws", __FUNCTION__, PID, PPID, entry.szExeFile);
				Log("  %ws", szPath);
			}			
		}
		__finally {
			if( hDebug )	UnsetDebugPrivilege(hDebug);
			CloseHandle(hSnapshot);
		}
	}
	bool	GetProcessPath(IN DWORD dwProcessId, OUT LPTSTR lpPath, IN DWORD dwPathSize)
	{
		*lpPath = NULL;		
		HANDLE	hProcess = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
								false, dwProcessId);
		if (hProcess)
		{
			// 2010.01.29 김범진
			// GetModuleFileNameEx 함수의 경우 32비트 프로세스에서 64비트 프로세스의 이름을
			// 얻어오는 과정에서 오류가 발생된다. 파샬 뭐라나?
			// 이건 32/64비트 간 문제이다. 젠장 이런 문제가 12년전에도 있었잖냐?
			// 그땐 16비트/32비트간 문제였는데.
			if (0 == ::GetModuleFileNameEx(hProcess, 0, lpPath, dwPathSize / sizeof(TCHAR)))
			{
				TCHAR	szPath[LBUFSIZE] = _T("");
				DWORD	dwSize = sizeof(szPath);

				if (::GetProcessImageFileName(hProcess, lpPath, dwPathSize))
				{

				}
			}
			::CloseHandle(hProcess);
		}
		else
		{
			Log("error: OpenProcess() failed. pId = %d, code = %d", 
				dwProcessId, ::GetLastError());
		}
		return *lpPath ? true : false;
	}
	HANDLE	SetDebugPrivilege()
	{
		HANDLE	hToken = NULL;
		if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
			&hToken))
		{
			Log("error: OpenProcessToken() failed. code = %d.", ::GetLastError());
			return NULL;
		}
		SetPrivilege(hToken, SE_DEBUG_NAME, TRUE);
		return hToken;
	}
	void	UnsetDebugPrivilege(IN HANDLE hToken)
	{
		if (hToken)
		{
			SetPrivilege(hToken, SE_DEBUG_NAME, FALSE);
			::CloseHandle(hToken);
		}
	}
	bool	SetPrivilege(HANDLE hToken, LPCTSTR Privilege, BOOL bEnablePrivilege)
	{
		TOKEN_PRIVILEGES	tp = { 0 };
		// Initialize everything to zero
		LUID				luid;
		DWORD				cb = sizeof(TOKEN_PRIVILEGES);

		if (!LookupPrivilegeValue(NULL, Privilege, &luid))
		{
			return false;
		}
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		if (bEnablePrivilege)
		{
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		}
		else
		{
			tp.Privileges[0].Attributes = 0;
		}
		AdjustTokenPrivileges(hToken, FALSE, &tp, cb, NULL, NULL);
		if (GetLastError() != ERROR_SUCCESS)
		{
			return false;
		}
		return true;
	}

private:

	
};

