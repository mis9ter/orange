#include "framework.h"
#include "CService.h"

void	CAgent::ServiceHandler
(
	IN DWORD dwControl, IN DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext
)
{
	CAgent *	pAgent	= (CAgent *)lpContext;
	pAgent->Log("%-32s %s(%d)", CService::GetControlName(dwControl), CService::GetEventType(dwEventType), 
		dwEventType);
	switch (dwControl)
	{
		case SERVICE_CONTROL_POWEREVENT:
			switch (dwEventType)
			{
			case PBT_APMSUSPEND:			//	절전 모드 진입
				pAgent->Log("SUSPEND");
				//pAgent->Notify(NULL, ClientNotifyWindowsSuspend, NULL, 0);
				break;

			case PBT_APMRESUMEAUTOMATIC:	//	절전 모드 해제
				pAgent->Log("RESUME");
				//pAgent->Notify(NULL, ClientNotifyWindowsResume, NULL, 0);
				break;
			}
			if (PBT_APMQUERYSUSPEND == dwEventType)
			{

			}
			break;

		case SERVICE_CONTROL_PRESHUTDOWN:
			break;

		case SERVICE_CONTROL_SHUTDOWN:
			break;

		case SERVICE_CONTROL_STOP:
			break;

		case SERVICE_CONTROL_SESSIONCHANGE:
			switch (dwEventType)
			{
				case WTS_CONSOLE_CONNECT:
				case WTS_CONSOLE_DISCONNECT:
				case WTS_REMOTE_CONNECT:
				case WTS_REMOTE_DISCONNECT:
				case WTS_SESSION_LOGON:
				case WTS_SESSION_LOGOFF:
				case WTS_SESSION_LOCK:
				case WTS_SESSION_UNLOCK:
				case WTS_SESSION_REMOTE_CONTROL:
					pAgent->Notify(NOTIFY_TYPE_SESSION, NOTIFY_EVENT_SESSION, 
						(WTSSESSION_NOTIFICATION *)lpEventData, sizeof(WTSSESSION_NOTIFICATION), NULL);
					break;

				default:
					//_nlogA(_T("session"), "SERVICE_CONTROL_SESSIONCHANGE - UNKNOWN EVENT CODE	%d", dwControl);
					break;
			}
			break;
	}
}
bool	CAgent::ServiceFunction(IN DWORD dwFunction, IN DWORD dwControl, IN LPVOID ptr) {
	CAgent *pAgent = (CAgent *)ptr;
	switch( dwFunction ) {
	
		case SERVICE_FUNCTION_INITIALIZE:
			pAgent->Log("SERVICE_FUNCTION_INITIALIZE");
			break;

		case SERVICE_FUNCTION_RUN:
			pAgent->Log("SERVICE_FUNCTION_RUN");
			if( pAgent->Start() ) {
				pAgent->RunLoop(AGENT_RUNLOOP_PERIOD);			
			}
			else 
				return false;
			break;

		case SERVICE_FUNCTION_SHUTDOWN:
			pAgent->Log("SERVICE_FUNCTION_SHUTDOWN:%d", dwControl);
			switch( dwControl ) {
				case 0:
					break;
				case SERVICE_CONTROL_SHUTDOWN:
					pAgent->Notify(NOTIFY_TYPE_SYSTEM, NOTIFY_EVENT_SHUTDOWN, NULL, NULL,NULL);
				case SERVICE_CONTROL_STOP:
				case SERVICE_CONTROL_USER_SHUTDOWN:
					pAgent->Shutdown(dwControl);
					break;

				case SERVICE_CONTROL_PRESHUTDOWN:
					pAgent->Log("SERVICE_CONTROL_PRESHUTDOWN");
					pAgent->Notify(NOTIFY_TYPE_SYSTEM, NOTIFY_EVENT_PRESHUTDOWN, NULL, NULL, NULL);
					break;
			}			
			break;
			
	}	
	return true;
}
void    RunInService(CAgent * pAgent)
{
    DWORD   dwSessionId = YAgent::GetCurrentSessionId();
    
    pAgent->Log("%s dwSessionId=%d", __FUNCTION__, dwSessionId);

    IService    *pService   = CService::GetInstance();
    if( pService ) {
        pService->SetServiceName(AGENT_SERVICE_NAME);
        pService->SetServiceDisplayName(AGENT_SERVICE_NAME);
        pService->SetServiceDescription(AGENT_SERVICE_DESC);
        pService->SetServicePath(pAgent->AppPath());

		if( pService->IsInstalled(__FUNCTION__)	&& pService->IsRunning(__FUNCTION__) ) {
			pAgent->Log("SERVICE_ALREADY_RUNNING");
			return;
		}
		if( pService->IsInstalled(__FUNCTION__) ) {

		}
		else {
			if( pService->Install(SERVICE_AUTO_START, __FUNCTION__) ) {

			}
			else {
				pAgent->Log("%s pService->Install() failed.", __FUNCTION__);
			}
		}
		pAgent->Log("SERVICE_INSTALLED");
		pService->SetServiceFunction(pAgent->ServiceFunction, pAgent);
        pService->SetServiceHandler(pAgent->ServiceHandler, pAgent);

		if( pService->Start(__FUNCTION__) ) {
			pAgent->Log("START_BY_SERVICE");
		}
		else {
			//	SCM에 의해 수행된 것이 아니라면 직접 SCM을 통해 호출한다.
			if( pService->StartByUser(__FUNCTION__) ) {
				pAgent->Log("START_BY_USER");
			}
			else {
				pAgent->Log("START_FAILURE");
			}
		}
   
        pService->Release();
    }
    else {
        pAgent->Log("%s pService is null.", __FUNCTION__);
    }    
}