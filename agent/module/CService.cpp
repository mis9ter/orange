// Service.cpp : Defines the entry point for the DLL application.
//
#include "CService.h"
#include "CException.h"

static	HMODULE	g_hModule;
/*
static	BOOL APIENTRY DllMain( HANDLE hModule, 
	DWORD  ul_reason_for_call, 
	LPVOID lpReserved
)
{
	if( DLL_PROCESS_ATTACH == ul_reason_for_call )
	{
		g_hModule	= (HMODULE)hModule;
	}
	return TRUE;
}
*/
static	LONG __stdcall __ExceptionHandler(struct _EXCEPTION_POINTERS * pExceptionInfo)
{
	WCHAR	szPath[MBUFSIZE]	= L"";
	::GetModuleFileName((HINSTANCE)g_hModule, szPath, sizeof(szPath));
	return EXCEPTION_EXECUTE_HANDLER;
}
void *	__stdcall CreateInstance()
{
	CService	*pClass	= CService::GetInstance();
	return dynamic_cast<IService *>(pClass);
}
bool	__stdcall DeleteInstance(LPVOID p)
{
	if( p )
	{
		IService	*pClass	= (IService *)p;
		pClass->Release();
	}
	return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
//	SCM 
///////////////////////////////////////////////////////////////////////////////////////////////////
bool	ServiceRemove(IN LPCTSTR pName)
{
	bool			bRet = false;
	SC_HANDLE		hService;
	SC_HANDLE		hManager;

	hManager	= ::OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if( hManager )
	{
		hService	= ::OpenService(hManager, pName, DELETE);
		if( hService )
		{
			if( ::DeleteService(hService) )
			{
				bRet = true;
			}
			else
			{
				//_log(_T("error: DeleteService() failed. code=%d."), GetLastError());
			}
			::CloseServiceHandle(hService);
		}
		else
		{
			//_log(_T("error: OpenService() failed. code=%d."), GetLastError());
		}
		::CloseServiceHandle(hManager);
	}
	else
	{
		//_log(_T("error: OpenSCManager() failed. code=%d."), GetLastError());
	}
	return bRet;
}
bool	ServiceIsInstalled(LPCTSTR pName)
{
	bool		bRet	= false;

	SC_HANDLE	hManager;
	SC_HANDLE	hService;

	hManager	= ::OpenSCManager(NULL, NULL, GENERIC_READ);
	if( hManager )
	{
		hService	= ::OpenService(hManager, pName, GENERIC_READ);
		if( hService )
		{
			::CloseServiceHandle(hService);
			bRet	= true;
		}
		else
		{
			//nAlert(TEXT("OpenService() failed. code=%d."), GetLastError());
		}
		::CloseServiceHandle(hManager);
	}			

	return bRet;
}
bool	ServiceInstall(
	IN LPCTSTR pPath, 
	IN LPCTSTR pName,
	IN LPCTSTR pDisplayName,
	IN LPCTSTR pDescription
)
{
	bool		bRet	= false;
	SC_HANDLE	hManager;
	SC_HANDLE	hService;

	hManager	= ::OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if( hManager )
	{
		hService	= ::CreateService(hManager, 
			pName,
			pDisplayName,
			SERVICE_ALL_ACCESS, 
			// 아래와 같은 경우 콘솔 프로그램을 수행하면 도스창이 뜬다.
			//SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS,
			SERVICE_WIN32_OWN_PROCESS,
			//SERVICE_WIN32_OWN_PROCESS,
			SERVICE_AUTO_START,
			SERVICE_ERROR_IGNORE,
			pPath,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL);
		if( hService && pDescription )
		{
			SERVICE_DESCRIPTION	sd;

			sd.lpDescription	= (LPTSTR)pDescription;
			::ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &sd);
		}
		else
		{
			//_log(_T("error: CreateService() failed. code=%d."), GetLastError());
		}
		if( hService )
		{
			bRet	= true;
			::CloseServiceHandle(hService);
		}
		::CloseServiceHandle(hManager);
	}
	else
	{
		//_log(_T("error: OpenSCManager() failed. code=%d."), GetLastError());
	}
	return bRet;
}
bool	ServiceStart(IN LPCTSTR pName)
{
	SC_HANDLE	hManager	= NULL;
	SC_HANDLE	hService	= NULL;
	LPCTSTR		av[]		= {_T("/s"),};
	bool		bFlag		= false;

	__try
	{
		hManager	= OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
		if( NULL == hManager )
		{
			//_log(_T("error: OpenSCManager() failed. name=[%s], code=%d"), pName, ::GetLastError());
			__leave;
		}

		hService	= OpenService(hManager, pName, SERVICE_ALL_ACCESS);
		if( NULL == hService )
		{
			//_log(_T("error: OpenService() failed. name=[%s], code=%d"), pName, ::GetLastError());
			__leave;
		}
		SERVICE_STATUS	s;

		if( ::StartService(hService, 1, av) )
		{
			ControlService(hService, SERVICE_START, &s);
			bFlag	= true;
		}
		else
		{
			bFlag	= false;
		}
	}
	__finally
	{
		if( hService )
		{
			CloseServiceHandle(hService);
		}
		if( hManager )
		{
			CloseServiceHandle(hManager);
		}
	}
	return bFlag;
}
bool	SetServiceShutdown(IN LPCTSTR pName)
{
	SC_HANDLE	hManager	= NULL;
	SC_HANDLE	hService	= NULL;
	bool		bFlag		= false;

	// TO DO: console mode로 동작중일때의 셧다운 처리 방법을 추가해야 한다.
	// 전역 이벤트등으로 처리해야 할 듯.

	__try
	{
		hManager	= ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if( NULL == hManager )
		{
			//_log(_T("error: request to service failure.\nOpenSCManager() failed. code=%d."), GetLastError());
			__leave;
		}

		hService	= ::OpenService(hManager, pName, SERVICE_ALL_ACCESS);
		if( hService )
		{
			SERVICE_STATUS	s;

			if( FALSE == ControlService(hService, SERVICE_CONTROL_STOP, &s) )
			{
				//_log(_T("error: control service failure.\nControlService() failed. code=%d"), GetLastError());
				__leave;
			}
		}
		bFlag	= true;
	}
	__finally
	{
		if( hService )
		{
			::CloseServiceHandle(hService);
		}
		if( hManager )
		{
			::CloseServiceHandle(hManager);
		}
	}
	return bFlag;	
}
bool	ServiceShutdown(IN LPCTSTR pName)
{
#define CHECK_PERIOD		1000
#define CHECK_MAXIMUM_TIME	30000

	SC_HANDLE	hManager	= NULL;
	SC_HANDLE	hService	= NULL;
	bool		bFlag		= false;
	DWORD		dwTime;

	// TO DO: console mode로 동작중일때의 셧다운 처리 방법을 추가해야 한다.
	// 전역 이벤트등으로 처리해야 할 듯.

	__try
	{
		hManager	= ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if( NULL == hManager )
		{
			//_log(_T("error: request to service failure.\nOpenSCManager() failed. code=%d."), GetLastError());
			__leave;
		}

		hService	= ::OpenService(hManager, pName, SERVICE_ALL_ACCESS);
		if( hService )
		{
			SERVICE_STATUS	s;

			if( QueryServiceStatus(hService, &s) )
			{
				if( SERVICE_STOPPED != s.dwCurrentState )
				{
					for( dwTime = 0 ; dwTime < CHECK_MAXIMUM_TIME && 
						SERVICE_STOP_PENDING == s.dwCurrentState ;  
						dwTime += CHECK_PERIOD )
					{
						//Sleep(s.dwWaitHint); -> hint가 쓸데없이 긴 경우.
						if( !QueryServiceStatus(hService, &s) )
						{
							break;
						}
						Sleep(CHECK_PERIOD);
					}
					if( SERVICE_STOPPED != s.dwCurrentState )
					{
						if( ControlService(hService, SERVICE_CONTROL_STOP, &s) )
						{
							for( dwTime = 0 ; dwTime < CHECK_MAXIMUM_TIME &&
								SERVICE_STOPPED != s.dwCurrentState ;
								dwTime += CHECK_PERIOD )
							{
								if( !QueryServiceStatus(hService, &s) )
								{
									break;
								}
								Sleep(CHECK_PERIOD);
							}
						}
					}
				}
			}
		}
		bFlag	= true;
	}
	__finally
	{
		if( hService )
		{
			::CloseServiceHandle(hService);
		}
		if( hManager )
		{
			::CloseServiceHandle(hManager);
		}
	}
	return bFlag;	
}
bool	IsServiceRunning(IN LPCTSTR pName)
{
	SC_HANDLE	hManager;
	SC_HANDLE	hService;
	bool		bRet	= false;

	hManager	= ::OpenSCManager(NULL, NULL, GENERIC_READ);
	if( hManager )
	{
		hService	= ::OpenService(hManager, pName, GENERIC_READ);
		if( hService )
		{
			SERVICE_STATUS	status;
			if( TRUE == QueryServiceStatus(hService, &status) )
			{
				if( SERVICE_RUNNING == status.dwCurrentState )
				{
					bRet	= true;
				}
			}
			else
			{
				//_log(_T("error: failed to query service status. error is %d"), GetLastError());
			}
			::CloseServiceHandle(hService);
		}
		else
		{
			//nAlert(TEXT("OpenService() failed. code=%d."), GetLastError());
		}
		::CloseServiceHandle(hManager);
	}			

	return bRet;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
//	CService Class
///////////////////////////////////////////////////////////////////////////////////////////////////
CService *		CService::m_pInstance	= NULL;
CService *		CService::GetInstance()
{
	if( NULL == m_pInstance )
	{
		m_pInstance	= new CService();
	}
	return m_pInstance;
}
CService::CService()
	:
	CAppLog(L"service.log")
{
	Log(__FUNCTION__);
	m_status.dwServiceType				= 
		SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS;
	m_status.dwCurrentState				= SERVICE_START_PENDING;
	m_status.dwControlsAccepted			= 
		/*
		2018/03/02	TigerJ
		SERVICE_ACCEPT_STOP을 뺀 것은 SCM에서 사용자가 서비스 종료를 하지 못하게 하기 위함이다.

		*/
		SERVICE_ACCEPT_SHUTDOWN|SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_HARDWAREPROFILECHANGE|
		SERVICE_ACCEPT_POWEREVENT|SERVICE_ACCEPT_SESSIONCHANGE|SERVICE_ACCEPT_PRESHUTDOWN;
	m_status.dwWin32ExitCode			= NO_ERROR;
	m_status.dwServiceSpecificExitCode	= NO_ERROR;
	m_status.dwCheckPoint				= 0;
	m_status.dwWaitHint					= 30000;
	m_dwMode							= SERVICE_MODE;
	m_dwPeriod							= DEFAULT_PERIOD;

	m_pServiceHandler					= NULL;
	m_pServiceHandlerContext			= NULL;
	m_pServiceFunction					= NULL;
	m_pServiceFunctionContext			= NULL;

	m_szName[0]			= NULL;
	m_szDescription[0]	= NULL;
	m_szDisplayName[0]	= NULL;

	TCHAR	szPath[MBUFSIZE];
	::GetModuleFileName(NULL, szPath, sizeof(szPath));
	//::StringCbCat(szPath, sizeof(szPath), L" /service");
	SetServicePath(szPath);
}
CService::~CService()
{
	Log(__FUNCTION__);
}
bool			CService::Start(IN LPCSTR pCause)
{
	//_nlogA(__tag,	"시스템에 의해 시작되었습니다.");
	SERVICE_TABLE_ENTRY		serviceTable[] = 
	{
		{const_cast<LPTSTR>(GetServiceName()), ServiceMain},
		{NULL, NULL},
	};
	if( ::StartServiceCtrlDispatcher(serviceTable) )
	{
		return true;
	}
	return false;
}
bool			CService::StartByUser(IN LPCSTR pCause)
{
	//_nlogA(__tag,	"사용자에 의해 시작되었습니다.");
	return ServiceStart(GetServiceName());
}
bool			CService::IsInstalled(IN LPCSTR pCause)
{
	//_nlogA(__tag,	"%s	설치 여부?", pCause);
	bool	bRet	= ServiceIsInstalled(GetServiceName());
	//if( bRet )
	//	_nlogA(__tag,	"설치되어 있습니다.");
	//else 
	//	_nlogA(__tag,	"설치되어 있지 않습니다.");
	return bRet;
}
bool			CService::IsRunning(IN LPCSTR pCause)
{
	//_nlogA(__tag,	"%s	실행 여부?", pCause);
	bool	bRet	= IsServiceRunning(GetServiceName());
	//if( bRet )
	//	_nlogA(__tag,	"현재 실행중입니다.");
	//else 
	//	_nlogA(__tag,	"현재 실행 중이지 않습니다.");
	return bRet;
}
void	SetPrivilege()
{
	HANDLE				hToken	= INVALID_HANDLE_VALUE;
	TOKEN_PRIVILEGES	tp;
	LUID				luid;

	if( !::OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken) )
	{
		
	}
	if( !::LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &luid) )
	{
		
	}
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if( !::AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL) )
	{
		
	}
}
static	bool	SetServiceRecoveryMode
(	
	IN	LPCTSTR pName, 											//	서비스 명
	IN	bool	bEnable
)
{
	bool		bRet = false;
	SC_HANDLE	hManager;
	SC_HANDLE	hService;

	hManager = ::OpenSCManager(NULL, NULL, GENERIC_WRITE);
	if (hManager)
	{
		hService	= ::OpenService(hManager, pName, SERVICE_ALL_ACCESS);
		if (hService )
		{
			//	2020/12/10	beom
			//	복구모드 설정
			//	SERVICE_CONFIG_FAILURE_ACTIONS	설정
			SERVICE_FAILURE_ACTIONS_FLAG	flag	= {TRUE,};
			flag.fFailureActionsOnNonCrashFailures	= bEnable? TRUE : FALSE;
			if( ::ChangeServiceConfig2(hService, SERVICE_CONFIG_FAILURE_ACTIONS_FLAG, &flag) ) ;
			else {
				CErrorMessage	err(GetLastError());
				CService::GetInstance()->Log("%-32s ChangeServiceConfig2(SERVICE_CONFIG_FAILURE_ACTIONS_FLAG) failed. error %d:%s", 
					__FUNCTION__, (DWORD)err, (PCSTR)err);
			}
			SERVICE_FAILURE_ACTIONS			action;
			wchar_t							szRebootMsg[]	= L"";
			wchar_t							szCommand[]		= L"";
			SC_ACTION						actions[]		= {
				{SC_ACTION_RESTART,		1000},
				{SC_ACTION_RESTART,		10 * 1000},
				{SC_ACTION_RESTART,		30 * 1000},		//	3번째. 여기까지만 지정.
				{SC_ACTION_RUN_COMMAND, 60 * 1000}		//	"/recovery" 를 인자로 실행
														//	이 코드에선 그저 재시작으로 처리됨.
			};			
			//	dwResetPeriod
			//	The time after which to reset the failure count to zero if there are no failures, 
			//	in seconds. Specify INFINITE to indicate that this value should never be reset.
			//	1분내로 연속해서 3번 죽으면 끝인거지.
			action.dwResetPeriod		= 60;	//	단위 초
			action.lpRebootMsg			= szRebootMsg;
			action.lpCommand			= szCommand;
			action.cActions				= bEnable? _countof(actions) : 0;
			action.lpsaActions			= actions;

			if( ::ChangeServiceConfig2(hService, SERVICE_CONFIG_FAILURE_ACTIONS, &action) )
				bRet	= true;
			else {
				CErrorMessage	err(GetLastError());
				CService::GetInstance()->Log("%-32s ChangeServiceConfig2(SERVICE_CONFIG_FAILURE_ACTIONS) failed. error %d:%s", 
					__FUNCTION__, (DWORD)err, (PCSTR)err);
			}
			::CloseServiceHandle(hService);
		}
		else
		{
			CErrorMessage	err(GetLastError());
			CService::GetInstance()->Log("%-32s OpenService() failed. %s(%d)", 
				__FUNCTION__, (PCSTR)err, (DWORD)err);
		}
		::CloseServiceHandle(hManager);
	}
	else
	{
		CErrorMessage	err(GetLastError());
		CService::GetInstance()->Log("%-32s OpenSCManager() failed. %s(%d)", 
			__FUNCTION__, (PCSTR)err, (DWORD)err);
	}
	return bRet;
}
static	bool	InstallService
(	
	IN	LPCTSTR pPath,											//	서비스 파일 경로
	IN	LPCTSTR pName,											//	서비스 명
	IN	LPCTSTR pDisplayName,									//	서비스 표시명
	IN	LPCTSTR pDescription,									//	서비스 설명
	IN	DWORD	dwServiceType	= SERVICE_WIN32_OWN_PROCESS,	//	서비스 유형
	IN	DWORD	dwStartType		= SERVICE_AUTO_START
)
{
	bool		bRet = false;
	SC_HANDLE	hManager;
	SC_HANDLE	hService;

	/*
	#define SERVICE_ALL_ACCESS             (STANDARD_RIGHTS_REQUIRED     | \
	SERVICE_QUERY_CONFIG         | \
	SERVICE_CHANGE_CONFIG        | \
	SERVICE_QUERY_STATUS         | \
	SERVICE_ENUMERATE_DEPENDENTS | \
	SERVICE_START                | \
	SERVICE_STOP                 | \
	SERVICE_PAUSE_CONTINUE       | \
	SERVICE_INTERROGATE          | \
	SERVICE_USER_DEFINED_CONTROL)
	*/

	if (NULL == pPath || NULL == *pPath)	{
		puts("pPath is empty.");
		return false;
	}
	WCHAR	szPath[4096]	= L"";
	//	Unquoted Service Path
	//	CreateProcess 함수인 lpApplicationName 을 이용한 실행 우회 취약점
	//	아래 보시면 Genian Insights E Agent for Windows 서비스에 “”(quoted)로 path가 지정되어 있지 않아 C : \에 Program.exe 파일이 있으면 해당 파일이 시스템 권한을 가지고 실행
	//	이를 방지하기 위해 ""으로 싸줍니다.
	if (L'"' != *pPath)
		::StringCbPrintf(szPath, sizeof(szPath), L"\"%s\"", pPath);
	else
		::StringCbPrintf(szPath, sizeof(szPath), pPath);

	hManager = ::OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (hManager)
	{
		hService = ::CreateService(hManager,
			pName,
			pDisplayName,
			SERVICE_ALL_ACCESS,
			// 아래와 같은 경우 콘솔 프로그램을 수행하면 도스창이 뜬다.
			//SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS,
			dwServiceType,
			//SERVICE_WIN32_OWN_PROCESS,
			dwStartType,
			SERVICE_ERROR_IGNORE,
			szPath,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL);
		if (hService )
		{
			if( pDescription ) {
				SERVICE_DESCRIPTION	sd;
				sd.lpDescription = (LPTSTR)pDescription;
				::ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &sd);
			}
		}
		else
		{
			printf("error: CreateService() failed. code=%d\n", GetLastError());
		}
		if (hService)
		{
			bRet = true;
			::CloseServiceHandle(hService);
		}
		::CloseServiceHandle(hManager);
	}
	else
	{
		printf("error: OpenSCManager() failed. code=%d\n", GetLastError());
	}
	return bRet;
}
bool			CService::Install(IN DWORD dwStartType, IN LPCSTR pCause)
{
	//_tprintf(_T("%s\n"), GetServicePath());
	//_tprintf(_T("%s\n"), GetServiceName());
	//_tprintf(_T("%s\n"), GetServiceDisplayName());
	//_tprintf(_T("%s\n"), GetServiceDescription());
	//_nlog(__tag,	_T("%S	설치	%s	시작유형 %d"), pCause, GetServiceName(), dwStartType);
	if( InstallService(GetServicePath(), GetServiceName(), 
		GetServiceDisplayName(), GetServiceDescription(), SERVICE_WIN32_OWN_PROCESS, dwStartType) ) {
		SetServiceRecoveryMode(true);
	}
	return false;
}
bool			CService::SetServiceRecoveryMode(IN bool bEnable)
{
	return ::SetServiceRecoveryMode(GetServiceName(), bEnable);
}
bool			CService::Remove(IN LPCSTR pCause)
{
	//_nlog(__tag,	_T("%S	삭제	%s"), pCause, GetServiceName());
	return ServiceRemove(GetServiceName());
}
void			CService::SetServiceFunction(
	IN	_ServiceFunction	pFunction,
	IN	PVOID				pContext
)
{
	m_pServiceFunction			= pFunction;
	m_pServiceFunctionContext	= pContext;
}
void			CService::SetServiceHandler(
	IN	_ServiceHandler		pHandler,
	IN	PVOID				pContext
)
{
	m_pServiceHandler			= pHandler;
	m_pServiceHandlerContext	= pContext;
}
void			CService::SetServicePath(IN LPCTSTR pValue)
{
	::StringCbCopy(m_szPath, sizeof(m_szPath), pValue);
}
LPCTSTR			CService::GetServicePath()
{
	return m_szPath;
}
bool	CService::GetServiceConfig(IN LPCTSTR pName, OUT SERVICE_CONFIG * p)
{
	bool					bRet = false;
	SC_HANDLE				schSCManager = NULL;
	SC_HANDLE				schService = NULL;
	LPQUERY_SERVICE_CONFIG	lpsc = NULL;
	LPSERVICE_DESCRIPTION	lpsd = NULL;
	DWORD					dwBytesNeeded, cbBufSize, dwError;

	// Get a handle to the SCM database. 

	if( NULL == pName )
		pName	= GetServiceName();

	if( NULL == pName )	return false;

	__try
	{
		schSCManager = ::OpenSCManager(
			NULL,                    // local computer
			NULL,                    // ServicesActive database 
			SERVICE_QUERY_CONFIG);  // full access rights 

		if (NULL == schSCManager)
		{
			//_nlog(LOGNAME, _T("OpenSCManager failed (%d)"), GetLastError());
			__leave;
		}

		// Get a handle to the service.

		schService = ::OpenService(
			schSCManager,          // SCM database 
			pName,             // name of service 
			SERVICE_QUERY_CONFIG); // need query config access 

		if (schService == NULL)
		{
			//_nlog(LOGNAME, _T("OpenService failed (%d)"), GetLastError());
			::CloseServiceHandle(schSCManager);
			__leave;
		}

		// Get the configuration information.

		if (!::QueryServiceConfig(
			schService,
			NULL,
			0,
			&dwBytesNeeded))
		{
			dwError = GetLastError();
			if (ERROR_INSUFFICIENT_BUFFER == dwError)
			{
				cbBufSize = dwBytesNeeded;
				lpsc = (LPQUERY_SERVICE_CONFIG)::LocalAlloc(LMEM_FIXED, cbBufSize);
			}
			else
			{
				//_nlogA(LOGNAME, "QueryServiceConfig failed (%d)", dwError);
				__leave;
			}
		}
		if (!::QueryServiceConfig(
			schService,
			lpsc,
			cbBufSize,
			&dwBytesNeeded))
		{
			//_nlogA(LOGNAME, "QueryServiceConfig failed (%d)", GetLastError());
			__leave;
		}

		::StringCbCopy(p->szCommand, sizeof(p->szCommand), lpsc->lpBinaryPathName);
		::StringCbCopy(p->szAccount, sizeof(p->szAccount), lpsc->lpServiceStartName);
		p->szPath[0] = NULL;

		p->dwServiceType = lpsc->dwServiceType;
		p->dwStartType = lpsc->dwStartType;
		p->dwErrorControl = lpsc->dwErrorControl;
		p->dwTagId = lpsc->dwTagId;


		LPTSTR	pStr = wcsrchr(p->szCommand, L'\\');
		if (pStr)
		{
			for (; *pStr && *pStr != _T(' '); pStr++);
			::StringCchCopyN(p->szFilePath, sizeof(p->szFilePath) / sizeof(TCHAR),
				p->szCommand, pStr - p->szCommand);
		}
		else
		{
			::StringCbCopy(p->szFilePath, sizeof(p->szFilePath), p->szCommand);
		}

		::StringCbCopy(p->szPath, sizeof(p->szPath), p->szFilePath);
		pStr = wcsrchr(p->szPath, '\\');
		if (pStr)	*pStr = NULL;

		bRet = true;

		if (!QueryServiceConfig2(
			schService,
			SERVICE_CONFIG_DESCRIPTION,
			NULL,
			0,
			&dwBytesNeeded))
		{
			dwError = GetLastError();
			if (ERROR_INSUFFICIENT_BUFFER == dwError)
			{
				cbBufSize = dwBytesNeeded;
				lpsd = (LPSERVICE_DESCRIPTION)LocalAlloc(LMEM_FIXED, cbBufSize);
			}
			else
			{
				//_nlogA(LOGNAME, "QueryServiceConfig2 failed (%d)", dwError);
				__leave;
			}
		}
		if (!QueryServiceConfig2(
			schService,
			SERVICE_CONFIG_DESCRIPTION,
			(LPBYTE)lpsd,
			cbBufSize,
			&dwBytesNeeded))
		{
			//_nlogA(LOGNAME, "QueryServiceConfig2 failed (%d)", GetLastError());
			__leave;
		}
	}
	__finally
	{
		if (lpsc)	LocalFree(lpsc);
		if (lpsd)	LocalFree(lpsd);

		if (schService)	CloseServiceHandle(schService);
		if (schSCManager)	CloseServiceHandle(schSCManager);
	}
	return bRet;
}
bool			CService::GetSCMServicePath(OUT LPTSTR pValue, IN DWORD dwSize)
{
	SERVICE_CONFIG	service;

	::ZeroMemory(&service, sizeof(service));
	if( GetServiceName() && GetServiceConfig(GetServiceName(), &service) )
	{
		::StringCbCopy(pValue, dwSize, service.szFilePath);
		return true;
	}
	return false;
}
void			CService::SetServiceName(IN LPCTSTR lpValue)
{
	//_nlogA(__tag,	"서비스명이 %s로 설정되었습니다.", __ansi(lpValue));
	::StringCbCopy(m_szName, sizeof(m_szName), lpValue);
}
LPCTSTR			CService::GetServiceName()
{
	return m_szName[0]? m_szName : NULL;
}
void			CService::SetServiceDisplayName(IN LPCTSTR lpValue)
{
	::StringCbCopy(m_szDisplayName, sizeof(m_szDisplayName), lpValue);
}
LPCTSTR			CService::GetServiceDisplayName()
{
	return m_szDisplayName;
}
void			CService::SetServiceDescription(IN LPCTSTR lpValue)
{
	::StringCbCopy(m_szDescription, sizeof(m_szDescription), lpValue);
}
LPCTSTR			CService::GetServiceDescription()
{
	return m_szDescription;
}

// Virtual Function
void	CService::SetWaitHint(DWORD dwHint)
{
	m_status.dwWaitHint	= dwHint;
}
void	CService::SetExitCode(DWORD dwCode)
{
	//_log(_T("windows service exit code is changed to %d."), dwCode);

	m_status.dwWin32ExitCode	= dwCode;
}

bool	CService::SetStatus(DWORD dwState)
{
	BOOL	bRet	= TRUE;
	
	PCSTR	pNames[]	= {
		"",
		"SERVICE_STOPPED",
		"SERVICE_START_PENDING",
		"SERVICE_STOP_PENDING",
		"SERVICE_RUNNING",
		"SERVICE_CONTINUE_PENDING",
		"SERVICE_PAUSE_PENDING",
		"SERVICE_PAUSED",
	};

	if( dwState > 0 && dwState <= SERVICE_PAUSED )
		Log("%s %s", __FUNCTION__, pNames[dwState]);
	else 
		Log("%s UNKNOWN(%d)", __FUNCTION__, dwState);
	if( dwState )
	{
		m_status.dwCurrentState		= dwState;
	}

	if( SERVICE_RUNNING == dwState || SERVICE_STOPPED == dwState )
	{
		m_status.dwCheckPoint	= 0;
	}
	else
	{
		m_status.dwCheckPoint++;
	}

	bRet	= ::SetServiceStatus(m_hStatus, &m_status);
	if( FALSE == bRet )
	{
		//_tprintf(_T("SetServiceStatus() failed. code=%d.\n"), GetLastError());
	}
	return ( bRet )? true : false;
}

#ifndef SERVICE_CONTROL_PRESHUTDOWN
#define SERVICE_CONTROL_PRESHUTDOWN          0x0000000F
#endif

DWORD	CService::ServiceHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	CService			*pService	= (CService *)lpContext;
	pService->Log("%s %-32s %s", __FUNCTION__, GetControlName(dwControl), GetEventType(dwEventType));
	if( pService->m_pServiceHandler )
	{
		pService->m_pServiceHandler(dwControl, dwEventType, lpEventData, pService->m_pServiceHandlerContext);
	}
	switch( dwControl )
	{
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_USER_SHUTDOWN:
		case SERVICE_CONTROL_SHUTDOWN:
			pService->SetWaitHint(30000);
			pService->SetStatus(SERVICE_STOP_PENDING);

		case SERVICE_CONTROL_PRESHUTDOWN:
			pService->ServiceFunction(SERVICE_FUNCTION_SHUTDOWN, dwControl);
			break;
	}
	return NO_ERROR;
}
bool	CService::Console()
{


	return true;
}
void	CService::ServiceMain(DWORD ac, PTSTR *av)
{
	CService *	pService	= CService::GetInstance();
	DWORD		dwLine		= __LINE__;

	__try
	{
		pService->Log(__FUNCTION__);
		pService->m_hStatus	= ::RegisterServiceCtrlHandlerEx(pService->GetServiceName(), 
			pService->ServiceHandlerEx, pService);
		if( NULL == pService->m_hStatus ) {
			pService->Log("%s RegisterServiceCtrlHandlerEx() failed. code=%d", __FUNCTION__, GetLastError());
		}
		dwLine	= __LINE__;
		pService->SetStatus(SERVICE_START_PENDING);
		dwLine	= __LINE__;
		if( NULL == pService->m_hStatus )
		{
			dwLine	= __LINE__;
			pService->Log("오류	pService->m_hStatus의 값이 없습니다.\n");
			dwLine	= __LINE__;
			pService->SetStatus(SERVICE_STOPPED);
			dwLine	= __LINE__;
			return;
		}
		pService->SetWaitHint(1000);
		if( pService->ServiceFunction(SERVICE_FUNCTION_INITIALIZE, 0) )
		{
			dwLine	= __LINE__;
			pService->SetStatus(SERVICE_RUNNING);
			dwLine	= __LINE__;
			if( pService->ServiceFunction(SERVICE_FUNCTION_RUN, 0) )
			{
				dwLine	= __LINE__;
			}
			dwLine	= __LINE__;
		}
		dwLine	= __LINE__;
		pService->SetWaitHint(30000);
		dwLine	= __LINE__;
		pService->SetStatus(SERVICE_STOP_PENDING);
		dwLine	= __LINE__;
		pService->ServiceFunction(SERVICE_FUNCTION_SHUTDOWN, 0);			
		dwLine	= __LINE__;
	}
	__except( CException::Handler(GetExceptionInformation()) )
	{
		pService->Log("익셉션	@서비스 메인 함수 래퍼. 함수=%s, 라인=%d\n", __FUNCTION__, dwLine);
	}
	pService->SetStatus(SERVICE_STOPPED);
}

bool	CService::Shutdown(IN LPCSTR pCause)
{
	//_nlogA(__tag,	"%s	종료를 원합니다.", pCause);
	return	RequestByUser(SERVICE_FUNCTION_SHUTDOWN);
}

bool	CService::RequestByUser(DWORD dwControl)
{
	SC_HANDLE	hManager	= NULL;
	SC_HANDLE	hService	= NULL;
	bool		bFlag		= false;

	// TO DO: console mode로 동작중일때의 셧다운 처리 방법을 추가해야 한다.
	// 전역 이벤트등으로 처리해야 할 듯.

	//_log(_T("windows service request(%d) by user"), dwControl);

	__try
	{
		hManager	= ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
		if( NULL == hManager )
		{
			printf("오류	서비스 매니저 열기 실패. 오류코드=%d\n", GetLastError());
			__leave;
		}

		//hService	= ::OpenService(hManager, GetServiceName(), 
		//					SERVICE_USER_DEFINED_CONTROL);
		hService	= ::OpenService(hManager, GetServiceName(), 
			SERVICE_CONTROL_STOP);
		if( hService )
		{
			SERVICE_STATUS	s;
			//if( FALSE == ControlService(hService, SERVICE_CONTROL_USER_SHUTDOWN, &s) )
			if( FALSE == ControlService(hService, SERVICE_CONTROL_STOP, &s) )
			{
				Log("오류	서비스를 종료시킬수 없습니다. 오류코드=%d\n", GetLastError());
			}
		}
		bFlag	= true;
	}
	__finally
	{
		if( hManager )
		{
			::CloseServiceHandle(hManager);
		}
		if( hService )
		{
			::CloseServiceHandle(hService);
		}
	}
	return bFlag;
}