#include "pch.h"
#include "framework.h"

#define	REG_MACHINEGUID_KEY		L"SOFTWARE\\Microsoft\\Cryptography"
#define REG_MACHINEGUID_VALUE	L"MachineGuid"
#define REG_BOOTID_KEY			L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PrefetchParameters"
#define REG_BOOTID_VALUE		L"BootId"

YAGENT_COMMON_BEGIN

DWORD	GetBootId()
{
	static	DWORD	dwId = 0;
	if (0 == dwId) {
		CAppRegistry::GetValue(HKEY_LOCAL_MACHINE, REG_BOOTID_KEY, REG_BOOTID_VALUE,
			REG_DWORD, &dwId, sizeof(dwId));
	}
	else {
		//	기존에 읽은 값이 있다면 그걸 리턴한다. 
		//	BootId 값은 실행 중엔 변경되지 않는 값이다.
	}
	return dwId;
}

YAGENT_COMMON_END