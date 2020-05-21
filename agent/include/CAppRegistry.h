#pragma once
#include "CAppLog.h"
class CAppRegistry
	: virtual public CAppLog
{
public:
	static	bool	IsExistingKey(IN HKEY hRootKey, IN PCWSTR pKey, IN PCWSTR pSubKey = NULL)
	{
		bool	bRet = false;
		HKEY	hKey = NULL;
		CString	strPath;

		if (pSubKey)	strPath.Format(L"%s\\%s", pKey, pSubKey);
		if (ERROR_SUCCESS == RegOpenKeyEx(hRootKey, pSubKey ? (LPCWSTR)strPath : pKey, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKey))
		{
			bRet = true;
			RegCloseKey(hKey);
		}
		return bRet;
	}
	static	bool	CreateKey(IN HKEY hRootKey, IN PCWSTR pKey, IN PCWSTR pSubKey, OUT HKEY * phKey)
	{
		bool	bRet = false;
		HKEY	hKey = NULL;
		DWORD	dwDisposition = 0;
		CString	strPath;
		if (RegCreateKeyEx(hRootKey, pKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_WOW64_64KEY,
			NULL, &hKey, &dwDisposition) == ERROR_SUCCESS) {
			HKEY	hSubKey = NULL;
			if (pSubKey)
			{
				if (RegCreateKeyEx(hKey, pSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_WOW64_64KEY,
					NULL, &hSubKey, &dwDisposition) == ERROR_SUCCESS) {
					bRet = true;
					if (phKey)	*phKey = hSubKey;
					else 		RegCloseKey(hSubKey);
				}
			}
			else
			{
				bRet = true;
			}
			RegCloseKey(hKey);
			//	dwDisposition	REG_CREATED_NEW_KEY or REG_OPENED_EXISTING_KEY
		}
		return bRet;
	}
	static	bool	DeleteKey(IN HKEY hRootKey, IN PCWSTR pKey, IN PCWSTR pSubKey = NULL)
	{
		//	RegDeleteEx 는 윈도 비스타 또는 윈도XP(x64)에서만 제공. x86 XP에선 지원되지 않아요.
		TCHAR	szKey[AGENT_PATH_SIZE] = L"";
		if (pSubKey)
		{
			StringCbPrintf(szKey, sizeof(szKey), L"%s\\%s", pKey, pSubKey);
			pKey = szKey;
		}
		if (ERROR_SUCCESS == RegDeleteKeyEx(hRootKey, pKey, KEY_WOW64_64KEY, 0))
		{
			return true;
		}
		else
		{
			//Log("RegDeleteKeyEx() failed. code=%d", GetLastError());
		}
		return false;
	}
	static	bool	GetValue(IN HKEY hRootKey, IN PCWSTR pSubKey, IN PCWSTR pName, IN DWORD dwType, LPVOID pValue, IN DWORD dwValueSize)
	{
		bool	bRet = false;
		HKEY	hKey = NULL;
		if (ERROR_SUCCESS == RegOpenKeyEx(hRootKey, pSubKey, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKey))
		{
			if (ERROR_SUCCESS == RegQueryValueEx(hKey, pName, NULL, &dwType, (LPBYTE)pValue, &dwValueSize))
				bRet = true;
			RegCloseKey(hKey);
		}
		return bRet;
	}
	static	bool	SetValue(IN HKEY hRootKey, IN PCWSTR pSubKey, 
					IN PCWSTR pName, IN const LPVOID pValue, IN size_t nDataSize, IN DWORD dwType)
	{
		bool	bRet = false;
		HKEY	hKey = NULL;
		__try
		{
			if (ERROR_SUCCESS != RegOpenKeyEx(hRootKey, pSubKey, 0, KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_WOW64_64KEY, &hKey))
			{
				if (RegCreateKeyEx(hRootKey, pSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_WOW64_64KEY,
					NULL, &hKey, NULL) != ERROR_SUCCESS)
				{
					printf("%s RegCreateKeyEx() failed. code=%d\n", __FUNCTION__, GetLastError());
					__leave;
				}
			}
			if (ERROR_SUCCESS != RegSetValueEx(hKey, pName, 0, dwType, (const BYTE *)pValue, (DWORD)nDataSize))
				__leave;
			bRet = true;
		}
		__finally
		{
			if (hKey)		RegFlushKey(hKey), RegCloseKey(hKey), hKey = NULL;
		}
		return bRet;
	}
	static	bool	DeleteValue
	(
		HKEY			hKey,
		const	WCHAR*	pszRegKeyPath,
		const WCHAR*	pszRegValueName,
		BOOL			bWow64Redirect = FALSE,
		IN BOOL			bFlushImmediately = TRUE
	)
	{
		WCHAR	szRegPath[MAX_PATH] = { 0, };
		HKEY	hSubkey = NULL;
		DWORD	dwError = ERROR_SUCCESS;

		__try
		{
			if (hKey == NULL || pszRegKeyPath == NULL) {
				dwError = ERROR_INVALID_PARAMETER; __leave;
			}
			StringCchCopy(szRegPath, _countof(szRegPath), pszRegKeyPath);
			if (RegOpenKeyEx(hKey, szRegPath, 0, MAXIMUM_ALLOWED | (bWow64Redirect ? 0 : KEY_WOW64_64KEY), &hSubkey) != ERROR_SUCCESS) {
				dwError = GetLastError(); __leave;
			}
			if (RegDeleteValue(hSubkey, pszRegValueName) != ERROR_SUCCESS) {
				dwError = GetLastError(); __leave;
			}
			if (bFlushImmediately)	RegFlushKey(hSubkey);
		}
		__finally
		{
			if (hSubkey)	RegCloseKey(hSubkey);
			SetLastError(dwError);
		}
		return TRUE;
	}

private:

};
