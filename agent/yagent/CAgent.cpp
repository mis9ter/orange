#include "Framework.h"

CAgent::CAgent()
	:
	CFilterCtrl(NULL, NULL)
{
	Log(__FUNCTION__);
	ZeroMemory(&m_config, sizeof(m_config));
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
	GetModuleFileName(NULL, m_config.szPath, sizeof(m_config.szPath));
	wchar_t* p = wcsrchr(m_config.szPath, L'\\');
	if (p)		*p = NULL;

	__try {
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
		if (false == CDB::Open(m_config.szEventDBPath)) {
			Log("%s can not open db %ws", __FUNCTION__, m_config.szEventDBPath);
			__leave;
		}
		if( false == CEventCallback::Initialize() ) {
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
	if (CDB::IsOpened()) {
		CDB::Close();
	}
	if (m_config.bInitialize) {
		CEventCallback::Destroy();
	}
}

bool	CAgent::Start(void* pCallbackPtr, PFUNC_AGENT_RUNLOOP pCallback)
{
	Log(__FUNCTION__);
	do
	{
		m_config.pRunLoopPtr	= pCallbackPtr;
		m_config.pRunLoopFunc	= pCallback;
		if (CFilterCtrl::IsInstalled())
		{

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
		m_config.bRun = true;
	} while( false );
	return m_config.bRun;
}
void	CAgent::Shutdown()
{
	if( WAIT_TIMEOUT == WaitForSingleObject(m_config.hShutdown, 0))
	{
		Log(__FUNCTION__);
		SetEvent(m_config.hShutdown);

		if (CFilterCtrl::IsRunning())
		{
			CFilterCtrl::Shutdown();
		}
	}
}
void	CAgent::RunLoop()
{
	if( m_config.bRun )
	{
		if( m_config.pRunLoopFunc )
			m_config.pRunLoopFunc(m_config.hShutdown, m_config.pRunLoopPtr);

	}
	Shutdown();
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