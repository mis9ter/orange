#include "pch.h"
#include "framework.h"
#include "crc64.h"

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

BootUID	GetBootUID()
{
	static	BootUID		UID	= 0;
	WCHAR				szBuf[100];
	if( 0 == UID ) {
		int64_t		t	= CTime::GetBootTimestamp() / 100000 * 100000;
		StringCbPrintf(szBuf, sizeof(szBuf), L"%d-%I64d", GetBootId(), t);

		CCRC64	crc64;
		UID	= crc64.GetCrc64(szBuf, (DWORD)(wcslen(szBuf) * sizeof(WCHAR)));	
	}
	return UID;
}
PCWSTR	GetMachineGuid(IN PWSTR pValue, IN DWORD dwSize)
{
	if(CAppRegistry::GetValue(HKEY_LOCAL_MACHINE, REG_MACHINEGUID_KEY, REG_MACHINEGUID_VALUE,
			REG_SZ, pValue, dwSize) )
		return pValue;
	if (pValue && dwSize >= sizeof(WCHAR)) {
		*pValue	= NULL;
	}
	return pValue;
}
YAGENT_COMMON_END