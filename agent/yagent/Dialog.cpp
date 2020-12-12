#include "framework.h"
#include "stdio.h"

void    AgentRunLoop(HANDLE hShutdown, void* pCallbackPtr)
{


}

void    ShowCounter(CAgent* pAgent, HWND hWnd) {
    static  DWORD       dwProcess   = (DWORD)-1;
    static  DWORD       dwThread    = (DWORD)-1;
    static  DWORD       dwModule    = (DWORD)-1;

    if (dwProcess != pAgent->m_counter.dwProcess)
        SetDlgItemInt(hWnd, IDC_EDIT_PROCESS, pAgent->m_counter.dwProcess, false);
    if (dwProcess != pAgent->m_counter.dwThread)
        SetDlgItemInt(hWnd, IDC_EDIT_THREAD, pAgent->m_counter.dwThread, false);
    if (dwProcess != pAgent->m_counter.dwModule)
        SetDlgItemInt(hWnd, IDC_EDIT_MODULE, pAgent->m_counter.dwModule, false);

    dwProcess   = pAgent->m_counter.dwProcess;
    dwThread    = pAgent->m_counter.dwThread;
    dwModule    = pAgent->m_counter.dwModule;
}
void    CALLBACK    Timer(
    HWND        hWnd,
    UINT        uMsg,
    UINT_PTR    pCallbackPtr,
    DWORD       dwTicks
) {
    CAgent      *pAgent = (CAgent *)pCallbackPtr;

    static  FILETIME    ftPrevTime[4];
    FILETIME            ftTime[4];

    GetProcessTimes(GetCurrentProcess(), &ftTime[0], &ftTime[1], &ftTime[2], &ftTime[3]);  
    if (
        ftPrevTime[2].dwLowDateTime != ftTime[2].dwLowDateTime ||
        ftPrevTime[3].dwLowDateTime != ftTime[2].dwLowDateTime)
    {
        WCHAR   szTime[2][40]   = {L"", L""};
        WCHAR   szTimes[100]    = L"";
        CTime::FileTimeToSystemTimeString(&ftTime[2], szTime[0], sizeof(szTime[0]), true);
        CTime::FileTimeToSystemTimeString(&ftTime[3], szTime[1], sizeof(szTime[1]), true);
        StringCbPrintf(szTimes, sizeof(szTimes), L"K:%s U:%s", szTime[0], szTime[1]);
        SetDlgItemText(hWnd, IDC_STATIC_TIMES, szTimes);

        CopyMemory(ftPrevTime, ftTime, sizeof(ftPrevTime));
    }
    ShowCounter(pAgent, hWnd);
}
void    RefreshButton(CAgent* pAgent, HWND hWnd) {
    EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_START), pAgent->IsRunning() ? FALSE : TRUE);
    EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_SHUTDOWN), pAgent->IsRunning() ? TRUE : FALSE);
};
void    RunInDialog(CAgent * pAgent)
{
    CDialog      dialog;

    dialog.SetMessageCallbackPtr(pAgent);
    dialog.AddMessageCallback(WM_INITDIALOG, [](
        IN	HWND	hWnd,
        IN	UINT	nMsgId,
        IN	WPARAM	wParam,
        IN	LPARAM	lParam,
        IN	PVOID	ptr
        ) {
        CAgent* pAgent = (CAgent*)ptr;
        ShowCounter(pAgent, hWnd);
        RefreshButton(pAgent, hWnd);
        WCHAR    szTime[40];
        SetDlgItemText(hWnd, IDC_STATIC_STARTTIME, CTime::GetLocalTimeString(szTime, sizeof(szTime)));
    });
    dialog.AddMessageCallback(IDC_BUTTON_INSTALL, [](
        IN	HWND	hWnd,
        IN	UINT	nMsgId,
        IN	WPARAM	wParam,
        IN	LPARAM	lParam,
        IN	PVOID	ptr
        ) {
        CAgent* pAgent = (CAgent*)ptr;
        pAgent->Install();
    });
    dialog.AddMessageCallback(IDC_BUTTON_UNINSTALL, [](
        IN	HWND	hWnd,
        IN	UINT	nMsgId,
        IN	WPARAM	wParam,
        IN	LPARAM	lParam,
        IN	PVOID	ptr
        ) {
        CAgent* pAgent = (CAgent*)ptr;
        pAgent->Uninstall();
    });
    dialog.AddMessageCallback(IDC_BUTTON_START, [](
        IN	HWND	hWnd,
        IN	UINT	nMsgId,
        IN	WPARAM	wParam,
        IN	LPARAM	lParam,
        IN	PVOID	ptr
        ) {
        CAgent* pAgent = (CAgent*)ptr;

        if (pAgent->IsInitialized()) {

        }
        else {
            pAgent->SetRunLoop(pAgent, AgentRunLoop);
            pAgent->Start();
            RefreshButton(pAgent, hWnd);
        }
    });
    dialog.AddMessageCallback(IDC_BUTTON_SHUTDOWN, [](
        IN	HWND	hWnd,
        IN	UINT	nMsgId,
        IN	WPARAM	wParam,
        IN	LPARAM	lParam,
        IN	PVOID	ptr
        ) {
        CAgent* pAgent = (CAgent*)ptr;
        pAgent->Shutdown(SERVICE_CONTROL_STOP);
        RefreshButton(pAgent, hWnd);
    });

    if (dialog.Create(IDD_CONTROL_DIALOG, AGENT_SERVICE_NAME))
    {
        dialog.SetTimer((ULONG_PTR)pAgent, Timer);
        dialog.MessagePump([](IN HANDLE hShutdown, IN HWND hWnd, IN PVOID ptr) {
            while (WAIT_TIMEOUT == WaitForSingleObject(hShutdown, 1000))
            {

            }
        });
        dialog.Destroy();
    }
}
