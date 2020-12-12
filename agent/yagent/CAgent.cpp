#include "Framework.h"

CAgent::CAgent()
	:
	CFilterCtrl(NULL, NULL)
{
	Log(__FUNCTION__);
	ZeroMemory(&m_config, sizeof(m_config));
	GetModuleFileName(NULL, m_config.szAppPath, sizeof(m_config.szAppPath));
	GetModuleFileName(NULL, m_config.szPath, sizeof(m_config.szPath));
	wchar_t* p = wcsrchr(m_config.szPath, L'\\');
	if (p)		*p = NULL;
	m_config.hShutdown	= CreateEvent(NULL, TRUE, FALSE, NULL);
}
CAgent::~CAgent()
{
	Log(__FUNCTION__);
	CloseHandle(m_config.hShutdown);
}
bool	CAgent::Install()
{
	wchar_t		szPath[AGENT_PATH_SIZE];
	YAgent::GetModulePath(szPath, sizeof(szPath));
	StringCbCat(szPath, sizeof(szPath), L"\\");
	StringCbCat(szPath, sizeof(szPath), DRIVER_FILE_NAME);
	CFilterCtrl::Install(DRIVER_SERVICE_NAME, szPath, false);
	return true;
}
void	CAgent::Uninstall()
{
	CFilterCtrl::Uninstall();
}
bool	CAgent::Initialize()
{
	Log(__FUNCTION__);
	__try {
		ResetEvent(m_config.hShutdown);
		StringCbPrintf(m_config.szDriverPath, sizeof(m_config.szDriverPath),
			L"%s\\%s", m_config.szPath, DRIVER_FILE_NAME);
		StringCbPrintf(m_config.szEventDBPath, sizeof(m_config.szEventDBPath),
			L"%s\\%s", m_config.szPath, DB_EVENT_DB);
		if( false == SetResourceToFile(IDR_KERNEL_DRIVER, m_config.szDriverPath) )
			__leave;
		if (!PathFileExists(m_config.szEventDBPath)) {
			if( false == SetResourceToFile(IDR_EVENT_ODB, m_config.szEventDBPath) ) {
				Log("%s SetReqourceToFile(%ws) failed.", __FUNCTION__, m_config.szEventDBPath);
				__leave;
			}
		}
		if (false == CDB::Open(m_config.szEventDBPath, __FUNCTION__)) {
			Log("%s can not open db %ws", __FUNCTION__, m_config.szEventDBPath);
			__leave;
		}
		if( false == CEventCallback::CreateCallback() ) {
			Log("%s CEventCallback::Initialize() failed.", __FUNCTION__);
			__leave;
		}
		m_config.bInitialize = true;
	}
	__finally {

	}
	return m_config.bInitialize;
}
void	CAgent::Destroy()
{
	if (m_config.bInitialize) {
		CEventCallback::DestroyCallback();
		m_config.bInitialize = false;
	}
	if (CDB::IsOpened()) {
		CDB::Close(__FUNCTION__);
	}
}

bool	CAgent::Start()
{
	Log(__FUNCTION__);
	do
	{
		if( false == Initialize() )	{
			Log("%s Initialize() failed.", __FUNCTION__);
			return false;
		}
		//	Initialize 단계가 끝나면 이벤트 청취 등록은 더 이상 되지 않는다.
		EnableRegistration(false);
		if (CFilterCtrl::IsInstalled())
		{
			if (CFilterCtrl::IsConnected() ) {
				return true;
			}
		}
		else
		{
			if (CFilterCtrl::Install(DRIVER_SERVICE_NAME, m_config.szDriverPath, false))
			{

			}
			else
			{
				Log("%s install failure.", __FUNCTION__);
				break;
			}
		}
		if (false == CFilterCtrl::Start(
				NULL, NULL, 
				dynamic_cast<CEventCallback *>(this),
				EventCallbackProc
			)
		)
		{
			Log("%s start failure.", __FUNCTION__);
			break;
		}
		if (CFilterCtrl::Connect())
		{

		}
		else
		{
			Log("%s connect failure.", __FUNCTION__);
			break;
		}

		CIPc::SetServiceCallback(IPCCallback, this);

		if( CIPc::Start(AGENT_PIPE_NAME, true) ) {
			Log("%s ipc pipe created.", __FUNCTION__);
		}
		else {
			Log("%s ipc pipe not created. code=%d", __FUNCTION__, GetLastError());
		}
		//	이제부터 비정상 종료시 SCM에 의해 되 살아납니다.
		Service()->SetServiceRecoveryMode(false);
		m_config.bRun = true;
	} while( false );
	return m_config.bRun;
}
void	CAgent::Shutdown(IN DWORD dwControl)
{
	switch( dwControl ) {
	case SERVICE_CONTROL_PRESHUTDOWN:

	break;

	default:
		if( WAIT_TIMEOUT == WaitForSingleObject(m_config.hShutdown, 0))
		{
			Log(__FUNCTION__);
			//	종료 과정에서 문제가 생겨 비정상 종료된 후 되살아나면 낭패.
			//	어차피 종료가 돼야 하니까.
			Service()->SetServiceRecoveryMode(true);

			SetEvent(m_config.hShutdown);
			CIPc::Shutdown();
			if (CFilterCtrl::IsRunning())
			{
				CFilterCtrl::Shutdown();
			}
			ResetEvent(m_config.hShutdown);
			Destroy();
		}
	}
}
void	CAgent::RunLoop(IN DWORD dwMilliSeconds)
{
	static	DWORD64		dwCount;
	Log("%s begin", __FUNCTION__);
	while( true ) {
		if( WAIT_OBJECT_0 == WaitForSingleObject(m_config.hShutdown, dwMilliSeconds) ) {
			break;
		}
		Notify(NOTIFY_TYPE_AGENT, NOTIFY_EVENT_PERIODIC, &dwCount, sizeof(dwCount));
		if( m_config.bRun )
		{
			if( m_config.pRunLoopFunc )
				m_config.pRunLoopFunc(m_config.hShutdown, m_config.pRunLoopPtr);
		}
		dwCount++;
	}
	Log("%s end", __FUNCTION__);
}
void	CAgent::MainThread(void* ptr, HANDLE hShutdown)
{
	CAgent	*pClass	= (CAgent *)ptr;

	printf("%-30s begin\n", __FUNCTION__);
	while (hShutdown)
	{
		if (WAIT_OBJECT_0 == WaitForSingleObject(hShutdown, 0))
		{
			printf("%-30s shutdown event\n", __FUNCTION__);
			break;
		}

		printf("command(q):");
		char	ch	= getchar();
		if ('q' == ch)
		{

		}
	}
	printf("%-30s end\n", __FUNCTION__);
}