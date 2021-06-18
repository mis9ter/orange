#include "pch.h"
#include <tlhelp32.h>
#include <psapi.h>

YAGENT_COMMON_BEGIN

bool	SetPrivilege(HANDLE hToken, LPCTSTR Privilege, BOOL bEnablePrivilege)
{
	TOKEN_PRIVILEGES	tp = { 0 };
	// Initialize everything to zero
	LUID				luid;
	DWORD				cb	= sizeof(TOKEN_PRIVILEGES);

	if(!LookupPrivilegeValue( NULL, Privilege, &luid ))
	{
		return false;
	}
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if(bEnablePrivilege) 
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
HANDLE	SetDebugPrivilege()
{
	HANDLE	hToken	= NULL;

	if( !::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,
		&hToken) )
	{
		return NULL;
	}
	SetPrivilege(hToken, SE_DEBUG_NAME, TRUE);

	return hToken;
}
void	UnsetDebugPrivilege(IN HANDLE hToken)
{
	if( hToken )
	{
		SetPrivilege(hToken, SE_DEBUG_NAME, FALSE);
		::CloseHandle(hToken);
	}
}
HANDLE	GetProcessHandle(IN DWORD dwProcessId)
{
	bool		bRet		= false;
	HANDLE		hProcess	= NULL;
	HANDLE		hToken		= NULL;;
	__try
	{
		hToken		= SetDebugPrivilege();
		hProcess	= ::OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcessId);
		if( NULL == hProcess )
		{
			__leave;
		}
	}
	__finally
	{
		UnsetDebugPrivilege(hToken);
	}
	return hProcess;
}
YAGENT_COMMON_END
