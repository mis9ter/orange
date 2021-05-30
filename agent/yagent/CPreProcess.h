#pragma once
/*
	2020/09/07	Beom

	�������� �����ϰ� �ִ� ���μ���, ���, ������ ������ ��´�.
*/
#include <TlHelp32.h>
#include <psapi.h>
#include <functional>
#include "CNtDll.h"

#define __nextptr(base, offset) ((PVOID)((ULONG_PTR) (base) + (ULONG_PTR) (offset))) 
#define ProcessCommandLineInformation	60

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
			UUID * pProcGuid, PROCUID * pUID, DWORD PID, DWORD PPID, PCWSTR pPath, PCWSTR pCmdLine
		)> pCallback = NULL
	
	) {
		ULONG		nNeededSize = 0;
		//PVOID		pBuf		= NULL;
		NTSTATUS	status		= ZwQuerySystemInformation(
									SystemProcessInformation, NULL, 0, &nNeededSize);
		if (STATUS_INFO_LENGTH_MISMATCH == status) {
			//	�Ҵ� �޾� ���ô�.
			//	��, ���μ��� ������ ���÷� ���Ѵٴ°�. �� ������ ȣ���ϸ� �� ū ũ�⸦ ���� ���� �ִ�.
			ULONG	nSize		= nNeededSize * 2;
			PVOID	pBuf		= malloc(nSize);
			WCHAR	szPath[AGENT_PATH_SIZE]	= L"";
			WCHAR	szCmdLine[AGENT_PATH_SIZE] = L"";
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
						PUNICODE_STRING	pCmdLine	= NULL;
						NTSTATUS		status;
						if ((HANDLE)0 == PID) {
							StringCbCopy(szPath, sizeof(szPath), L"System idle process");
						}
						else if ((HANDLE)4 == PID) {
							StringCbCopy(szPath, sizeof(szPath), L"System");
						}	
						else {
							if (NT_SUCCESS(status = GetProcessParams(PID, &PPID, 
									szPath, sizeof(szPath), szCmdLine, sizeof(szCmdLine)))) 
							{

							}
							/*
							if (NT_SUCCESS(status = GetProcessInfo(PID, ProcessImageFileName, &pImageFileName))) {
								StringCbPrintf(szPath, sizeof(szPath), L"%wZ", pImageFileName);
								free(pImageFileName);
							}
							else {
								Log("  GetProcessInfo() failed. status=%08x", status);
								StringCbCopy(szPath, sizeof(szPath), L"");
							}
							if (NT_SUCCESS(status = GetProcessInfo(PID, (PROCESSINFOCLASS)ProcessCommandLineInformation, &pImageFileName))) {
								StringCbPrintf(szCmdLine, sizeof(szCmdLine), L"%wZ", pCmdLine);
								free(pCmdLine);
							}
							else {
								Log("  GetProcessInfo() failed. status=%08x", status);
								StringCbCopy(szCmdLine, sizeof(szCmdLine), L"");
							}
							*/
						}		
						UUID	ProcGuid;
						PROCUID	ProcUID;
						GetProcGuid(PID, PPID, szPath, &ProcGuid, &ProcUID);
						if( pCallback )
							pCallback(&ProcGuid, &ProcUID, (DWORD)PID, (DWORD)PPID, szPath, szCmdLine);
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
			// 2010.01.29 �����
			// GetModuleFileNameEx �Լ��� ��� 32��Ʈ ���μ������� 64��Ʈ ���μ����� �̸���
			// ������ �������� ������ �߻��ȴ�. �ļ� ����?
			// �̰� 32/64��Ʈ �� �����̴�. ���� �̷� ������ 12�������� �־��ݳ�?
			// �׶� 16��Ʈ/32��Ʈ�� �������µ�.
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

