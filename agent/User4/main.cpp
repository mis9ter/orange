// User4.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"

#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
                                                // 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

     // TODO: 여기에 코드를 입력합니다.
    if (!SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
        return -1;


    //  윈도 버전을 알아둔다.
#ifdef ORANGE
    if (WindowsVersion >= WINDOWS_10)
    {
        PROCESS_MITIGATION_POLICY_INFORMATION policyInfo;

        // Note: The PhInitializeMitigationPolicy function enables the other mitigation policies.
        // However, we can only enable the ProcessSignaturePolicy after loading plugins.
        policyInfo.Policy = ProcessSignaturePolicy;
        policyInfo.SignaturePolicy.Flags = 0;
        policyInfo.SignaturePolicy.MicrosoftSignedOnly = TRUE;

        NtSetInformationProcess(NtCurrentProcess(), ProcessMitigationPolicy, &policyInfo, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));
    }

    if (PhIsExecutingInWow64())
    {
        PhShowWarning(
            NULL,
            L"You are attempting to run the 32-bit version of Process Hacker on 64-bit Windows. "
            L"Most features will not work correctly.\n\n"
            L"Please run the 64-bit version of Process Hacker instead."
        );
        RtlExitUserProcess(STATUS_IMAGE_SUBSYSTEM_NOT_PRESENT);
    }
    // Set the default priority.
    {
        PROCESS_PRIORITY_CLASS priorityClass;

        priorityClass.Foreground = FALSE;
        priorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_HIGH;

        if (PhStartupParameters.PriorityClass != 0)
            priorityClass.PriorityClass = (UCHAR)PhStartupParameters.PriorityClass;

        PhSetProcessPriority(NtCurrentProcess(), priorityClass);
    }

#endif

    WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
    WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_USER4, szWindowClass, MAX_LOADSTRING);

    CMainDialog     main;

    main.InitCommonControls();

    //SetDefaultSettings(&main);
    main.Create(hInstance, nCmdShow, szTitle, szWindowClass);
    main.MessageLoop();


    CoUninitialize();

    return 0;
}