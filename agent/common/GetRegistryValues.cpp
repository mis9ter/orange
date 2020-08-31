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
		//	������ ���� ���� �ִٸ� �װ� �����Ѵ�. 
		//	BootId ���� ���� �߿� ������� �ʴ� ���̴�.
	}
	return dwId;
}

YAGENT_COMMON_END