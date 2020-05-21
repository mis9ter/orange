#pragma once
#include <stdlib.h>

#define _SECOND						((ULONGLONG) 10000000)
#define _MINUTE						(60 * _SECOND)
#define _HOUR						(60 * _MINUTE)
#define _DAY						(24 * _HOUR)
#define LAST_SHUTDOWN_SAFETIME		(3 * _MINUTE)

#define	_abs(x)						((x) >= 0)? (x) : (-(x))

#define SHUTDOWN_REGISTRY_KEY			L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reliability\\shutdown"
#define REASONCODE_REGISTRY_VALUE		L"ReasonCode"
#define DIRTY_SHUTDOWN_REGISTRY_KEY		L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reliability"
#define DIRTY_SHUTDOWN_REGISTRY_VALUE	L"DirtyShutdown"

#define STARTTIME_REG_VALUENAME			L"LastStartTime"	//	드라이버가 로드된 수. 정상적으로 언로드될 때 감소되므로 
#define LOADEDCOUNT_REG_VALUENAME		L"LoadedCount"		//	이 값이 일정 값 이상인 경우 비정상 종료가 많이 발생되었다는 것.
#define EMERGENT_STATE_VALUENAME		L"EmergentState"
#define INSTALLED_REG_VALUENAME			L"InstalledTime"
#define MAX_LOADEDCOUNT					3

class CLastShutdownChecker
{
public:
	CLastShutdownChecker(IN PUNICODE_STRING pKeyPath)
		:
		m_pKeyPath(pKeyPath)
	{
		//__log("%s %wZ", __FUNCTION__, pKeyPath);
	}
	~CLastShutdownChecker()
	{
		//__log("%s", __FUNCTION__);
	}
	bool			IsDirtyShutdown()
	{
		/*
			이전 윈도 사용시 BSOD가 발생했는지 여부 검사.
			참고URL:  http://xenkimv.blogspot.com/2013/04/dirty-shutdown-eventlog-6008.html

			Windows은 Shutdown을 두 가지 종류로 구분합니다
			1. Clean Shutdown(=Expected Shutdown): 정상적인 Shutdown
			2. Dirty Shutdown(=Unexpected Shutdown): 비정상적인 Shutdown
			사용자가 시스템을 정상으로 종료한 것이 아니고 비정상적으로 종료 된 경우가 Dirty Shutdown에 해당 되며 바로 6008 오류가 로깅 됩니다.

			Registry를 통한 시스템 비정상 종료 여부 판단
			HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Reliability
			DirtyShutdown
			값이 0인 경우 정상 종료, 1인 경우 비정상 종료
		* */
		NTSTATUS		status = STATUS_SUCCESS;
		PKEY_VALUE_PARTIAL_INFORMATION	pValue = NULL;
		DWORD			dwValue = 0;
		CUnicodeString	strKey;

		strKey.SetString(DIRTY_SHUTDOWN_REGISTRY_KEY, PagedPool);
		status = CDriverRegistry::QueryValue((PUNICODE_STRING)strKey, DIRTY_SHUTDOWN_REGISTRY_VALUE,
					&pValue);
		if( NT_SUCCESS(status))
		{
			if (REG_DWORD == pValue->Type)
			{
				dwValue = *((DWORD *)pValue->Data);
				if( dwValue )	__log("%-20s DirtyShutdown=0x%0x", __FUNCTION__, dwValue);
			}
		}
		if (pValue)	CDriverRegistry::FreeValue(pValue);
		return dwValue? true : false;
	}
	DWORD			GetShutdownReasonCode()
	{
		/*
			이전 윈도 사용시 BSOD가 발생했는지 여부 검사.

			참고URL:  http://xenkimv.blogspot.com/2013/04/dirty-shutdown-eventlog-6008.html

			Windows은 Shutdown을 두 가지 종류로 구분합니다
			1. Clean Shutdown(=Expected Shutdown): 정상적인 Shutdown
			2. Dirty Shutdown(=Unexpected Shutdown): 비정상적인 Shutdown
			사용자가 시스템을 정상으로 종료한 것이 아니고 비정상적으로 종료 된 경우가 Dirty Shutdown에 해당 되며 바로 6008 오류가 로깅 됩니다.

			Registry를 통한 시스템 비정상 종료 여부 판단(조사중, 아직 불확실)

			HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Reliability\shutdown

			ReasonCode(DWORD)
			값이 0인 경우 정상 종료, 아닌 경우 비정상 종료인가?
			https://docs.microsoft.com/en-us/windows/win32/shutdown/system-shutdown-reason-codes

			현재는 ReasonCode 의 값이 0인 경우 성공적인 종료, 0이 아니면 비정상 종료로 간주한다.

		* */
		NTSTATUS		status = STATUS_SUCCESS;
		PKEY_VALUE_PARTIAL_INFORMATION	pValue = NULL;
		DWORD			dwValue = 0;
		CUnicodeString	strKey;
		
		strKey.SetString(SHUTDOWN_REGISTRY_KEY, PagedPool);
		status = CDriverRegistry::QueryValue((PUNICODE_STRING)strKey, REASONCODE_REGISTRY_VALUE, 
					&pValue);
		if( NT_SUCCESS(status) )
		{
			if (REG_DWORD == pValue->Type)
			{
				dwValue = *((DWORD *)pValue->Data);
				if( dwValue )	__log("%-20s ReasonCode=0x%0x", __FUNCTION__, dwValue);
			}
		}
		if (pValue)	CDriverRegistry::FreeValue(pValue);
		return dwValue;
	}
	void			SetEmergentState(bool bFlag)
	{
		DWORD	dwValue	= bFlag;
		CDriverRegistry::SetValue((PUNICODE_STRING)m_pKeyPath, EMERGENT_STATE_VALUENAME,
			REG_DWORD, &dwValue, sizeof(dwValue));
	}
	bool			Check()
	{
		bool	bDirtyShutdown	= IsDirtyShutdown();
		DWORD	dwReasonCode	= GetShutdownReasonCode();
		UNREFERENCED_PARAMETER(bDirtyShutdown);
		return dwReasonCode? false : true;
	}
	bool			CheckOld()
	{
		bool			bRet	= true;
		NTSTATUS		status = STATUS_SUCCESS;
		PKEY_VALUE_PARTIAL_INFORMATION	pValue = NULL;
		LARGE_INTEGER	currentTime = { 0, };
		PLARGE_INTEGER	pLastStartTime = NULL;

		if( IsDirtyShutdown() )	return false;

		__try
		{
			KeQuerySystemTime(&currentTime);
			status = CDriverRegistry::QueryValue(m_pKeyPath, STARTTIME_REG_VALUENAME, &pValue);
			if (!NT_SUCCESS(status))
			{
				// 마지막 시작 시간이 없다는건 최초 설치 후 동작? 또는 삭제 후 다시 설치 등등 => 최초로 보자.
				status	= STATUS_SUCCESS;
				__leave;
			}
			if (pValue->Type != REG_QWORD)
			{
				//	내가 저장한 형태는 이것이 아닌데?
				__leave;
			}
			pLastStartTime = (PLARGE_INTEGER)pValue->Data;
			if ((currentTime.QuadPart - pLastStartTime->QuadPart) < LAST_SHUTDOWN_SAFETIME)
			{
				// 이전 호출된 후 5분이 안되서 다시 호출되었다는 것:
				//	1. 시스템 크래시로 인한 리부팅. 그런데 메모리 덤프 생성 과정의 시간이 꽤 길단 말이지. 
				//	이 과정에서 5분이 넘으면 다시 로드/크래시가 반복될 텐데. 
				__log("%s emergenct state. seconds=%d %d (%I64d-%I64d)/%d", 
				__FUNCTION__, 
					(DWORD)((currentTime.QuadPart - pLastStartTime->QuadPart)/_SECOND),
					(DWORD)(LAST_SHUTDOWN_SAFETIME/_SECOND),
					currentTime.QuadPart, pLastStartTime->QuadPart, _SECOND);
				bRet	= false;
				status = STATUS_INVALID_SERVER_STATE;
				__leave;
			}
		}
		__finally
		{
			if(pValue)	CDriverRegistry::FreeValue(pValue);
			//status	= CDriverRegistry::SetValue((PUNICODE_STRING)m_keyPath, STARTTIME_REG_VALUENAME, 
			//			(wcslen(STARTTIME_REG_VALUENAME)+1) * sizeof(WCHAR), REG_QWORD, &currentTime.QuadPart, sizeof(currentTime));
		}
		return bRet;
	}
	void			Clear()
	{
		//	정상적으로 종료되었으므로 이전 기록을 삭제, 초기화시킨다.
		CDriverRegistry::DeleteValue((PUNICODE_STRING)m_pKeyPath, STARTTIME_REG_VALUENAME, 
						(wcslen(STARTTIME_REG_VALUENAME) + 1) * sizeof(WCHAR));
		SetEmergentState(false);
	}
private:
	PUNICODE_STRING		m_pKeyPath;
};
