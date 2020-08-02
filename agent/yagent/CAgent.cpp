#include "CAgent.h"

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
bool	CAgent::Start(void* pCallbackPtr, PFUNC_AGENT_RUNLOOP pCallback)
{
	Log(__FUNCTION__);
	wchar_t		szPath[AGENT_PATH_SIZE];
	GetModuleFileName(NULL, szPath, sizeof(szPath));
	wchar_t* p = wcsrchr(szPath, L'\\');
	if (p)		*(p + 1) = NULL;

	StringCbCat(p, sizeof(szPath) - wcslen(szPath), DRIVER_FILE_NAME);
	do
	{
		m_config.pRunLoopPtr = pCallbackPtr;
		m_config.pRunLoopFunc = pCallback;

		if (CFilterCtrl::IsInstalled())
		{

		}
		else
		{
			if (CFilterCtrl::Install(DRIVER_SERVICE_NAME, szPath, false))
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