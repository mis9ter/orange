#include "Framework.h"

void	CAgent::AddDbList(int nResourceID, PCWSTR pRootPath, PCWSTR pName, DbMap & table) {
	Log("----------------------------------------------------------");
	Log("%-32s %ws", "pName", pName);
	Log("----------------------------------------------------------");
	Log("%-32s %d", "nResourceID", nResourceID);
	Log("%-32s %ws", "pRootPath", pRootPath);

	try {
		DbConfigPtr	ptr		=	std::shared_ptr<DB_CONFIG>(new DB_CONFIG);

		ptr->nResourceID	= nResourceID;
		ptr->strCDB			= (std::wstring)pRootPath + L"\\" + pName + L".cdb";
		ptr->strODB			= (std::wstring)pRootPath + L"\\" + pName + L".odb";

		Log("%-32s %ws", "ODB", ptr->strODB.c_str());
		Log("%-32s %ws", "CDB", ptr->strCDB.c_str());

		table[pName]	= ptr;
	}
	catch( std::exception & e) {
		UNREFERENCED_PARAMETER(e);	
		Log("%-32s %s", __func__, e.what());
	}
	Log("----------------------------------------------------------");
}
CAgent::CAgent()
	:
	CFilterCtrl(NULL, NULL)
{
	Log("%-32s begin", __func__);
	ZeroMemory(&m_config, sizeof(m_config));
	GetModuleFileName(NULL, m_config.path.szApp, sizeof(m_config.path.szApp));

	PWSTR	pStr;
	StringCbCopy(m_config.path.szDriver, sizeof(m_config.path.szDriver), m_config.path.szApp);
	pStr	= _tcsrchr(m_config.path.szDriver, L'\\');
	if( pStr )	*pStr	= NULL;
	StringCbPrintf(m_config.path.szDriver + lstrlen(m_config.path.szDriver), 
		sizeof(m_config.path.szDriver) - lstrlen(m_config.path.szDriver) * sizeof(WCHAR), 
		L"\\%s", DRIVER_FILE_NAME);


	YAgent::GetDataFolder(AGENT_DATA_FOLDERNAME, m_config.path.szData, sizeof(m_config.path.szData));
	YAgent::GetDataFolder(AGENT_DATA_FOLDERNAME, m_config.path.szDump, sizeof(m_config.path.szDump));
	YAgent::MakeDirectory(m_config.path.szData);
	AddDbList(IDR_CONFIG_ODB, m_config.path.szData, L"config",		m_db);
	AddDbList(IDR_EVENT_ODB, m_config.path.szData, L"event",		m_db);
	AddDbList(IDR_SUMMARY_ODB, m_config.path.szData, L"summary",	m_db);
	AddDbList(IDR_STRING_ODB, m_config.path.szData, L"string",		m_db);
	AddDbList(IDR_PROCESS_ODB, m_config.path.szData, L"process", m_db);

	m_config.hShutdown	= CreateEvent(NULL, TRUE, FALSE, NULL);
	Log("%-32s end", __func__);
}
CAgent::~CAgent()
{
	Log(__FUNCTION__);
	CloseHandle(m_config.hShutdown);
}
bool	CAgent::Install()
{
	Log("%-32s %ws", __FUNCTION__, m_config.path.szDriver);
	CFilterCtrl::Install(DRIVER_SERVICE_NAME, m_config.path.szDriver, false);
	return true;
}
void	CAgent::Uninstall()
{
	CFilterCtrl::Uninstall();
}
DWORD	Patch(PCWSTR pODB, PCWSTR pCDB, HANDLE hBreakEvent) {
	CDB		cdb;
	return cdb.Patch(pODB, pCDB, hBreakEvent);
}
bool	CAgent::Initialize()
{
	Log(__FUNCTION__);

	do {
		ResetEvent(m_config.hShutdown);

		Log("%-32s APPPATH:%ws", __func__, m_config.path.szApp);
		Log("%-32s DATAATH:%ws", __func__, m_config.path.szData);
		Log("%-32s DRIVER :%ws", __func__, m_config.path.szDriver);

		if( !PathFileExists(m_config.path.szDriver)) {		
			if( SetResourceToFile(IDR_ORANGE_DRIVER, m_config.path.szDriver) ) {
				Log("%s %ws saved.", __func__, m_config.path.szDriver);
			}
			else {
				Log("%s SetReqourceToFile(%ws) failed.", __func__, m_config.path.szDriver);
			}
		}
		for( auto t : m_db ) {
			if( false == SetResourceToFile(t.second->nResourceID, t.second->strODB.c_str()) ) {
				Log("%s SetReqourceToFile(%ws) failed.", __func__, t.second->strCDB.c_str());
				break;
			}
			if( PathFileExists(t.second->strCDB.c_str()) ) {			
			
			}
			else {
				if( CopyFile(t.second->strODB.c_str(), t.second->strCDB.c_str(), TRUE) ) {				
				
				}
				else {
					CErrorMessage	err(GetLastError());
					Log("%-32s %s(%d)", __func__, (PCSTR)err, (DWORD)err);				
				}			
			}
			Patch(t.second->strODB.c_str(), t.second->strCDB.c_str(), NULL);			
			if ( t.second->cdb.Open(t.second->strCDB.c_str(), __func__) ) {
				Log("%-32s %ws %p", __func__, t.second->strCDB.c_str(), t.second->cdb.Handle());
			}
			else {
				Log("%s can not open db %ws", __func__, t.second->strCDB.c_str());
				break;
			}
		}
	}
	while( false );

	CMemoryPtr	stmt	= GetResourceData(NULL, IDR_STMT_JSON);
	CStmt::Create(stmt->Data(), stmt->Size());

	if( false == CEventCallback::CreateCallback() ) {
		Log("%-32s CEventCallback::Initialize() failed.", __func__);
		return false;
	}

	//	권한 설정
	//
	return (m_config.bInitialize = true);
}
void	CAgent::Destroy(PCSTR pCause)
{
	Log("%-32s @%s", __func__, pCause);
	if (m_config.bInitialize) {
		Log("CEventCallback::DestroyCallback()");
		CEventCallback::DestroyCallback();
		m_config.bInitialize = false;
	}

	Log("CStmt::Destroy");
	CStmt::Destroy();

	Log("db close");
	for( auto t : m_db ) {
		if ( t.second->cdb.IsOpened() ) {
			Log("%-32s %ws %p", __func__, t.second->strCDB.c_str(), t.second->cdb.Handle());
			t.second->cdb.Close(__func__);
		}
		else {
			Log("%s can not open db %ws", __func__, t.second->strCDB.c_str());
			break;
		}
	}
	Log("done");
}
bool	GetProcessModules(HANDLE hProcess, PVOID pContext, ModuleListCallback pCallback);
int		GetSystemModules(PVOID pContext, ModuleListCallback pCallback);

bool	CAgent::Start()
{
	Log(__FUNCTION__);
	Log("YFILTER_HEADER  %d", sizeof(YFILTER_HEADER));
	Log("Y_HEADER        %d", sizeof(Y_HEADER));
	Log("YFILTER_DATA    %d", sizeof(YFILTER_DATA));

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
			Log("%-32s driver is installed.", __func__);
			if (CFilterCtrl::IsConnected() ) {
				Log("%-32s driver is connected.", __func__);
				//break;
			}
		}
		else
		{
			Log("%-32s driver is not installed.", __func__);
			if (CFilterCtrl::Install(DRIVER_SERVICE_NAME, m_config.path.szDriver, false))
			{
				Log("%-32s driver is installed.", __func__);
			}
			else
			{
				Log("%-32s driver install failure.", __FUNCTION__);
				break;
			}
		}
		CIPCCommand::AddCommand("sqlite3.query", "unknown", this, Sqllite3QueryUnknown);

		if (false == CFilterCtrl::Start2(
				NULL, NULL, 
				dynamic_cast<CEventCallback *>(this),
				EventCallbackProc2
			)
		)
		{
			Log("%s start failure.", __FUNCTION__);
		}
		if (CFilterCtrl::Connect())
		{

		}
		else
		{
			Log("%s connect failure.", __FUNCTION__);
		}

		COMMAND_ENABLE	command = { 0, };
		Y_REPLY		reply = { 0, };
		command.dwCommand = Y_COMMAND_ENABLE_MODULE;
		command.bEnable = true;
		command.nSize	= sizeof(COMMAND_ENABLE);
		if (CFilterCtrl::SendCommand2((PY_COMMAND)&command)) {
			Log("%-32s %-20s %d", __func__, "Y_COMMAND_ENABLE_MODULE", command.bRet);
		}
		command.dwCommand = Y_COMMAND_ENABLE_THREAD;
		command.bEnable = true;
		command.bRet	= false;
		if (CFilterCtrl::SendCommand2(&command)) {
			Log("%-32s %-20s %d", __func__, "Y_COMMAND_ENABLE_THREAD", command.bRet);
		}

		CIPC::SetServiceCallback(IPCRecvCallback, this);
		if( CIPC::Start(AGENT_SERVICE_PIPE_NAME, true) ) {
			Log("%s ipc pipe created.", __FUNCTION__);
		}
		else {
			Log("%s ipc pipe not created. code=%d", __FUNCTION__, GetLastError());
		}
		//	이제부터 비정상 종료시 SCM에 의해 되 살아납니다.
		Service()->SetServiceRecoveryMode(false);
		CFilterCtrl::SendCommand(Y_COMMAND_GET_PROCESS_LIST);


		Log("---------------------------------------------------------------------------");
		HANDLE	PID			= (HANDLE)5096;
		HANDLE	hProcess	= NULL;

		if ((HANDLE)4 == PID) {
			GetSystemModules(this, [&](PVOID pContext, PMODULE p)->bool {

				WCHAR	szPath[AGENT_PATH_SIZE] = L"";
				if (false == CAppPath::GetFilePath(p->FullName, szPath, sizeof(szPath)))
					StringCbCopy(szPath, sizeof(szPath), p->FullName);
				Log("%-32s %p %ws", "System", p->ImageBase, szPath);
				return true;
			});
		}
		else {
			if (KOpenProcess(PID, PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, &hProcess)) {
				Log("%-32s KOpenProcess() succeeded. hProcess=%p", __func__, hProcess);

				GetProcessModules(hProcess, this,
					[&](PVOID pContext, PMODULE p)->bool {
					Log("%-32s %p %ws", "Process", p->ImageBase, p->BaseName);
					return true;
				});
				Log("%-32s CloseHandle()=%d", __func__, CloseHandle(hProcess));
			}
		}
		m_config.bRun = true;
	} while( false );
	return m_config.bRun;
}
void	CAgent::Shutdown(IN DWORD dwControl)
{
	Log("%-32s (1)", __func__);
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
			CIPC::Shutdown();
			if (CFilterCtrl::IsRunning())
			{
				CFilterCtrl::Shutdown();
			}
			ResetEvent(m_config.hShutdown);
			Destroy(__func__);
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
		Notify(NOTIFY_TYPE_AGENT, NOTIFY_EVENT_PERIODIC, &dwCount, sizeof(dwCount), 
			[&](DWORD64 dwValue, bool *bCallback) {

				if( dwCount && 0 == dwCount % dwValue ) 
					*bCallback	= true;
				else 
					*bCallback	= false;
			
			});
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