#pragma once
#include "CAppRegistry.h"
#include <versionhelpers.h>
/*
	Driver Management	- Install/IsInstall/Uninstall/Start/Stop
*/
typedef struct _CONFIG
{
	_CONFIG
	(
		IN PCWSTR	pDriverPath		= DRIVER_FILE_NAME,
		IN PCWSTR	pServiceName	= AGENT_SERVICE_NAME,
		IN PCWSTR	pDisplayName	= AGENT_DISPLAY_NAME
	)
	{
		InitializeCriticalSection(&lock);
		hDriver = INVALID_HANDLE_VALUE;
		bGood = IsGood();
		StringCbCopyW(szServiceName, sizeof(szServiceName), pServiceName);
		StringCbCopyW(szDisplayName, sizeof(szDisplayName), pDisplayName);

		CString		driverPath;
		WCHAR		szModulePath[MAX_PATH] = { 0 };
		GetModuleFileNameW((HMODULE)&__ImageBase, szModulePath, MAX_PATH);
		*wcsrchr(szModulePath, L'\\') = L'\0';
		driverPath.Format(L"%s\\%s", szModulePath, pDriverPath);

		StringCbCopyW(szDriverPath, sizeof(szDriverPath), driverPath);
		StringCbCopyW(szDriverInstallPath, sizeof(szDriverInstallPath), driverPath);
		CString deviceName;
		deviceName.Format(L"\\\\.\\%s", szServiceName);
		StringCbCopyW(szDeviceName, sizeof(szDeviceName), deviceName);
	}
	~_CONFIG()
	{
		DeleteCriticalSection(&lock);
	}
	bool				bGood;									//	동작할 수 있는 환경이다. 
	HANDLE				hDriver;								//	드라이버와의 연결 핸들 
																//	[TODO] 나중에 미니필터의 메세지로 변경할까요?
	TCHAR				szDeviceName[AGENT_PATH_SIZE];			//	드라이버 심볼릭 링크
	TCHAR				szServiceName[AGENT_NAME_SIZE];		//	드라이버 서비스 이름
	TCHAR				szDisplayName[AGENT_NAME_SIZE];		//	드라이버 서비스 표시
	TCHAR				szDriverPath[AGENT_PATH_SIZE];			//	드라이버 파일 경로
	TCHAR				szDriverInstallPath[AGENT_PATH_SIZE];	//	드라이버 설치 경로

	CRITICAL_SECTION	lock;

	void				Lock()
	{
		EnterCriticalSection(&lock);
	}
	void				Unlock()
	{
		LeaveCriticalSection(&lock);
	}

	bool			IsGood()
	{
		bool	bRet = IsWindows7OrGreater();
		return bRet;
	}
} CONFIG;
class CDriverService
	:
	public CAppRegistry,
	virtual public CAppLog

{
public:
	CDriverService
	(
		IN PCWSTR	pDriverPath		= DRIVER_FILE_NAME,
		IN PCWSTR	pServiceName	= DRIVER_SERVICE_NAME,
		IN PCWSTR	pDisplayName	= DRIVER_DISPLAY_NAME
	)
		:
		m_config(pDriverPath, pServiceName, pDisplayName)
	{

	}
	~CDriverService()
	{
		if (IsConnected())	Disconnect();
	}
	static	PCWSTR	GetDriverRegistryPath(IN PCWSTR pName, OUT PWSTR pValue, IN DWORD dwSize)
	{
		if (NULL == pValue || 0 == dwSize)	return NULL;
		StringCbPrintf(pValue, dwSize, L"SYSTEM\\CurrentControlSet\\Services\\%s", pName);
		return pValue;
	}
	void			SetDriver(IN PCWSTR pName, IN PCWSTR pPath)
	{
		WCHAR	szCurPath[AGENT_PATH_SIZE];
		GetModuleFileName(NULL, szCurPath, sizeof(szCurPath));
		PathRemoveFileSpec(szCurPath);

		if (pPath && PathIsRelative(pPath))
			StringCbPrintf(m_config.szDriverPath, sizeof(m_config.szDriverPath), L"%s\\%s", szCurPath, pPath);
		else if (NULL == pPath)
			StringCbPrintf(m_config.szDriverPath, sizeof(m_config.szDriverPath), L"%s\\%s", szCurPath, pName);
		else
			StringCbCopy(m_config.szDriverPath, sizeof(m_config.szDriverPath), pPath);
		Log("DRIVER_PATH:%ws", m_config.szDriverPath);
	}
	void			SetDriverInstallPath(IN PCWSTR pPath)
	{
		StringCbCopy(m_config.szDriverInstallPath, sizeof(m_config.szDriverInstallPath), pPath);
	}
protected:
	bool			DriverInstall(
		PCWSTR		pName,
		DWORD		dwAltitude,		
		bool		bAutoStart,
		bool		bRunInSafeMode
	)
	{
		if (false == IsGood())	return false;
		return CreateDriverService(m_config.szDriverInstallPath, pName, dwAltitude,
			bAutoStart, bRunInSafeMode);
	}
	bool			DriverIsInstalled(void) const
	{
		bool		bRet = false;
		SC_HANDLE	hScm = NULL;
		SC_HANDLE	hService = NULL;
		__try
		{
			hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
			if (NULL == hScm)	__leave;
			hService = OpenServiceW(hScm, GetDriverServiceName(), SERVICE_START);
			if (NULL == hService)	__leave;
			bRet = true;
		}
		__finally
		{
			if (hService)	CloseServiceHandle(hService);
			if (hScm)		CloseServiceHandle(hScm);
		}
		return bRet;
	}
	bool			DriverUninstall()
	{
		if (!DriverIsInstalled())	return true;

		Disconnect();
		for (int i = 0; i < 3; i++)
		{
			if (StopDriverService()) break;
			::Sleep(1000);
		}
		if (DeleteDriverService())
		{
			if (DeleteFile(m_config.szDriverInstallPath))
			{
				Log("[%ws] is deleted.", m_config.szDriverInstallPath);
				return true;
			}
		}
		return false;
	}
	bool			DriverStart()
	{
		bool	bRet = false;
		__try
		{
			if (false == StartDriverService())	{
				__leave;
			}
			bRet = true;
		}
		__finally
		{
			if (false == bRet)
			{
				DriverStop();
			}
		}
		return bRet;
	}
	bool			DriverIsRunning()
	{
		bool		bRet = false;
		SC_HANDLE	hScm = NULL;
		SC_HANDLE	hService = NULL;
		__try
		{
			hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
			if (NULL == hScm) {
				Log("OpenSCManagerW() failed. code=%d", ::GetLastError());
				__leave;
			}
			hService = OpenServiceW(hScm, GetDriverServiceName(), SERVICE_QUERY_STATUS);
			if (NULL == hService) {
				Log("OpenServiceW() failed. code=%d", ::GetLastError());
				__leave;
			}
			SERVICE_STATUS_PROCESS	ssp;
			DWORD dwBytesNeeded = 0;

			if (QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
			{
				bRet = (ssp.dwCurrentState == SERVICE_RUNNING);
			}
			else
			{
				Log("QueryServiceStatusEx() failed. code=%d", ::GetLastError());
			}
		}
		__finally
		{
			if (hService)	CloseServiceHandle(hService);
			if (hScm)		CloseServiceHandle(hScm);
		}
		return bRet;
	}
	bool			DriverStop()
	{
		if (DriverIsRunning())
		{
			CString	subkey;

			subkey.Format(L"SYSTEM\\CurrentControlSet\\Services\\%s", GetDriverServiceName());
			if (IsConnected())	Disconnect();
			return StopDriverService();
		}
		return true;
	}
	bool			DriverControl(
		IN	DWORD dwControlCode,
		IN	LPVOID pRequest,
		IN DWORD dwRequestSize,
		OUT LPVOID pResponse,
		OUT DWORD dwResponseSize,
		OUT DWORD *pBytesReturned
	)
	{
		bool	bRet = false;
		__try
		{
			if (false == IsConnected()) {
				if (false == Connect())
				{
					Log("can not connect to driver. code=%d", ::GetLastError());
					__leave;
				}
			}
			if (DeviceIoControl(m_config.hDriver, dwControlCode, pRequest, dwRequestSize, pResponse, dwResponseSize, pBytesReturned, NULL))
			{

				bRet = true;
			}
			else
			{
				Log("DeviceIoControl() failed. code=%d", GetLastError());
			}
		}
		__finally
		{

		}
		return bRet;
	}
	PCWSTR			GetDriverServiceName()	const { return m_config.szServiceName; }
	PCWSTR			GetDriverDisplayName()	const { return m_config.szDisplayName; }
	PCWSTR			GetDriverDeviceName()		const { return m_config.szDeviceName; }
	bool			IsGood() const { return m_config.bGood; }
	CONFIG *		Config()
	{
		return &m_config;
	}
	bool			Connect()
	{
		__function_lock(&m_config.lock);
		if (INVALID_HANDLE_VALUE != m_config.hDriver)
		{
			//Log("%s	already connected.", __FUNCTION__);
			return true;
		}
		m_config.hDriver = CreateFile(GetDriverDeviceName(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (m_config.hDriver == INVALID_HANDLE_VALUE)
		{
			Log("%s	CreateFile() failed. code=%d", __FUNCTION__, GetLastError());
			return false;
		}
		return true;
	}
	void			Disconnect()
	{
		__function_lock(&m_config.lock);
		if (INVALID_HANDLE_VALUE != m_config.hDriver)
		{
			CloseHandle(m_config.hDriver);
			m_config.hDriver = INVALID_HANDLE_VALUE;
		}
	}
	bool			IsConnected()
	{
		__function_lock(&m_config.lock);
		return (m_config.hDriver != INVALID_HANDLE_VALUE);
	}
private:
	CONFIG			m_config;
	bool			SetDriverInstance(PCWSTR pInstanceName, DWORD dwAltitude, DWORD dwFlags)
	{
		bool	bRet = false;
		HKEY	hKey = NULL,
			hSubkey = NULL;
		WCHAR	szSubKeyPath[MAX_PATH] = { 0 };
		WCHAR	szInstances[MAX_PATH] = { 0 };

		StringCbPrintf(szSubKeyPath, sizeof(szSubKeyPath), L"SYSTEM\\CurrentControlSet\\Services\\%s\\Instances", GetDriverServiceName());
		__try
		{
			if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szSubKeyPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
				__leave;

			RtlZeroMemory(szInstances, sizeof(szInstances));
			StringCbCopy(szInstances, sizeof(szInstances), pInstanceName);
			if (RegSetValueEx(hKey, L"DefaultInstance", 0, REG_SZ, (const BYTE*)szInstances, ((DWORD)wcslen(szInstances) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS)
				__leave;
			if (RegCreateKeyEx(hKey, pInstanceName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hSubkey, NULL) != ERROR_SUCCESS)
				__leave;
			WCHAR	szAltitude[32] = { 0 };
			StringCbPrintf(szAltitude, sizeof(szAltitude), L"%u.%u", dwAltitude, GetTickCount());
			if (RegSetValueEx(hSubkey, L"Altitude", 0, REG_SZ, (const BYTE*)szAltitude, ((DWORD)wcslen(szAltitude) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS)
				__leave;
			if (RegSetValueEx(hSubkey, L"Flags", 0, REG_DWORD, (const BYTE*)&dwFlags, sizeof(dwFlags)) != ERROR_SUCCESS)	__leave;
			bRet = true;
		}
		__finally
		{
			if (hSubkey)	RegFlushKey(hSubkey), RegCloseKey(hSubkey), hSubkey = NULL;
			if (hKey)		RegFlushKey(hKey), RegCloseKey(hKey), hKey = NULL;
		}
		return bRet;
	}
	bool			SetForSafeMode(IN bool bSet)
	{
		/*
			안전모드에서 드라이버를 로딩시키기 위한 처리.

			1. SERVICE_BOOT_START 로 등록하기 위해서는 .sys 파일이 c:\windows\system32\drivers 에 있어야 한다.
			2. 레지스트리 설정
				HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\SafeBoot\Minimal
				HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\SafeBoot\Network
				에 드라이버 이름으로 키를 만드고 (확장자 불필요), 기본값에 REG_SZ 로 "Driver" 라고 적어주면 된다.

			출처: https://wendys.tistory.com/6 [웬디의 기묘한 이야기]
			안전모드에서 동작하려면 이 레지스트리들도 보호해줘야 겠다..
		*/
		bool	bRet = false;
		if (bSet)
		{
			__try
			{
				if (!CAppRegistry::SetValue(HKEY_LOCAL_MACHINE, SAFEBOOT_REG_MINIMAL,
					NULL, (const LPVOID)GetDriverServiceName(), (wcslen(GetDriverServiceName()) + 1) * sizeof(TCHAR), REG_SZ))	__leave;
				if (!CAppRegistry::SetValue(HKEY_LOCAL_MACHINE, SAFEBOOT_REG_NETWORK,
					NULL, (const LPVOID)GetDriverServiceName(), (wcslen(GetDriverServiceName()) + 1) * sizeof(TCHAR), REG_SZ))	__leave;
				bRet = true;
			}
			__finally
			{
				if (false == bRet)
					SetForSafeMode(false);
			}
		}
		else
		{
			CAppRegistry::DeleteKey(HKEY_LOCAL_MACHINE, SAFEBOOT_REG_MINIMAL, GetDriverServiceName());
			CAppRegistry::DeleteKey(HKEY_LOCAL_MACHINE, SAFEBOOT_REG_NETWORK, GetDriverServiceName());
		}
		return bRet;
	}
	bool			CreateDriverService
	(
		PCWSTR	pModulePath, PCWSTR pInstanceName, DWORD dwAltitude, bool bAutoStart,
		bool	bRunAtSafeMode

	)
	{
		bool		bRet = false;
		SC_HANDLE	hScm = NULL;
		SC_HANDLE	hService = NULL;

		SERVICE_FILE_SYSTEM_DRIVER;

		__try
		{
			hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
			if (NULL == hScm) {
				Log("OpenSCManagerW() failed. code=%d", GetLastError());
				__leave;
			}
			hService = CreateServiceW(hScm, GetDriverServiceName(), GetDriverDisplayName(),
				SERVICE_ALL_ACCESS, SERVICE_FILE_SYSTEM_DRIVER,
				bAutoStart ? (bRunAtSafeMode ? SERVICE_BOOT_START : SERVICE_AUTO_START) : SERVICE_DEMAND_START,
				SERVICE_ERROR_NORMAL,
				pModulePath, NULL, NULL, NULL, NULL, NULL);
			if (NULL == hService)
			{
				DWORD dwError = GetLastError();
				if (ERROR_SERVICE_EXISTS == dwError) {
					//	이미 존재한다는건 성공으로 간주.
				}
				else
				{
					Log("CreateServiceW() failed. code=%d", dwError);
					__leave;
				}
			}
			if (false == SetDriverInstance(pInstanceName, dwAltitude, 0))		__leave;
			SetForSafeMode(bRunAtSafeMode);
			bRet = true;
		}
		__finally
		{
			if (hService)	CloseServiceHandle(hService);
			if (hScm)	CloseServiceHandle(hScm);
		}
		return bRet;
	}
	bool			DeleteDriverService(void)
	{
		StopDriverService();
		bool		bRet = false;
		SC_HANDLE	hScm = NULL;
		SC_HANDLE	hService = NULL;

		__try
		{
			hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
			if (NULL == hScm)
			{
				Log("OpenSCManager() failed. code=%d", GetLastError());
				__leave;
			}
			hService = OpenServiceW(hScm, GetDriverServiceName(), SERVICE_ALL_ACCESS);
			if (NULL == hService)
			{
				Log("OpenService() failed. code=%d", GetLastError());
				__leave;
			}
			bRet = DeleteService(hService) ? true : false;
			if (false == bRet)
			{
				Log("DeleteService() failed. code=%d", GetLastError());
			}
		}
		__finally
		{
			if (hService)	CloseServiceHandle(hService);
			if (hScm)		CloseServiceHandle(hScm);
			SetForSafeMode(false);
		}
		return bRet;
	}
	bool			StartDriverService(void)
	{
		bool		bRet = false;
		SC_HANDLE	hScm = NULL;
		SC_HANDLE	hService = NULL;
		Log(__FUNCTION__);
		__try
		{
			hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
			if (NULL == hScm)
			{
				CErrorMessage	err(GetLastError());
				Log("%-32s OpenSCManager() failed. %s(%d)",
					__func__, (PCSTR)err, (DWORD)err);
				__leave;
			}
			hService = OpenServiceW(hScm, GetDriverServiceName(), SERVICE_START);
			if (NULL == hService)
			{
				CErrorMessage	err(GetLastError());
				Log("%-32s OpenService() failed. %s(%d)",
					__func__, (PCSTR)err, (DWORD)err);
				__leave;
			}
			if (StartServiceW(hService, 0, NULL)) {

			}
			else {
				CErrorMessage	err(GetLastError());
				if (ERROR_SERVICE_ALREADY_RUNNING == (DWORD)err)
				{

				}
				else
				{
					Log("%-32s StartService() failed.\n%s(%d)",
						__func__, (PCSTR)err, (DWORD)err);
					__leave;
				}
			}
			bRet = true;
		}
		__finally
		{
			if (hService)	CloseServiceHandle(hService);
			if (hScm)		CloseServiceHandle(hScm);
		}
		return bRet;
	}
	bool			StopDriverService(DWORD dwTimeout = 3000)
	{
		bool		bRet = true;
		SC_HANDLE	hScm = NULL;
		SC_HANDLE	hService = NULL;
		__try
		{
			hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
			if (NULL == hScm)
			{
				Log("OpenSCManager() failed. code=%d", GetLastError());
				__leave;
			}
			hService = OpenServiceW(hScm, GetDriverServiceName(), SERVICE_STOP | SERVICE_QUERY_STATUS);
			if (NULL == hService)
			{
				Log("OpenService() failed. code=%d", GetLastError());
				__leave;
			}

			SERVICE_STATUS_PROCESS	ssp;
			DWORD dwBytesNeeded = 0;

			if (QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
			{
				DWORD dwStartTime = GetTickCount();
				if (ssp.dwCurrentState == SERVICE_RUNNING)
				{
					bRet = false;
					if (ControlService(hService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp))
					{
						Sleep(ssp.dwWaitHint);
						while (SERVICE_STOP_PENDING == ssp.dwCurrentState)
						{
							if (GetTickCount() - dwStartTime >= dwTimeout)	break;
							Sleep(ssp.dwWaitHint);
							QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded);
						}
						while (SERVICE_STOPPED != ssp.dwCurrentState)
						{
							if (GetTickCount() - dwStartTime >= dwTimeout)	break;
							Sleep(ssp.dwWaitHint);
							QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded);
						}
					}
					else
					{
						Log("ControlService() failed. code=%d", GetLastError());
					}
				}
				if (SERVICE_STOPPED == ssp.dwCurrentState)
				{
					bRet = true;
				}
			}
		}
		__finally
		{
			if (hService)	CloseServiceHandle(hService);
			if (hScm)		CloseServiceHandle(hScm);
		}
		return bRet;
	}
};
