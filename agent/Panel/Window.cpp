#include "framework.h"

#define MAIN_WINDOW_CLASSNAME   L"Orange.User"
#define MAIN_WINDOW_NAME        L"Orange.User"
HINSTANCE               g_hInstance;
HWND                    PhMainWndHandle;
HWND                    TabControlHandle;
BOOLEAN                 PhPluginsEnabled = FALSE;
PPH_STRING              PhSettingsFileName = NULL;
PH_STARTUP_PARAMETERS   PhStartupParameters;

PH_PROVIDER_THREAD      PhPrimaryProviderThread;
PH_PROVIDER_THREAD      PhSecondaryProviderThread;

static PPH_LIST         DialogList = NULL;
static PPH_LIST         FilterList = NULL;
static PH_AUTO_POOL     BaseAutoPool;

#ifndef DEBUG
#include <symprv.h>
#include <minidumpapiset.h>

VOID PhpCreateUnhandledExceptionCrashDump(
    _In_ PEXCEPTION_POINTERS ExceptionInfo
)
{
    static PH_STRINGREF dumpFilePath = PH_STRINGREF_INIT((PWSTR)L"%USERPROFILE%\\Desktop\\");
    HANDLE fileHandle;
    PPH_STRING dumpDirectory;
    PPH_STRING dumpFileName;
    WCHAR alphastring[16] = L"";

    dumpDirectory = PhExpandEnvironmentStrings(&dumpFilePath);
    PhGenerateRandomAlphaString(alphastring, RTL_NUMBER_OF(alphastring));

    dumpFileName = PhConcatStrings(
        4,
        PhGetString(dumpDirectory),
        L"\\ProcessHacker_",
        alphastring,
        L"_DumpFile.dmp"
    );

    if (NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        dumpFileName->Buffer,
        FILE_GENERIC_WRITE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
    )))
    {
        MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;

        exceptionInfo.ThreadId = HandleToUlong(NtCurrentThreadId());
        exceptionInfo.ExceptionPointers = ExceptionInfo;
        exceptionInfo.ClientPointers = FALSE;

        PhWriteMiniDumpProcess(
            NtCurrentProcess(),
            NtCurrentProcessId(),
            fileHandle,
            MiniDumpNormal,
            &exceptionInfo,
            NULL,
            NULL
        );

        NtClose(fileHandle);
    }

    PhDereferenceObject(dumpFileName);
    PhDereferenceObject(dumpDirectory);
}

ULONG CALLBACK PhpUnhandledExceptionCallback(
    _In_ PEXCEPTION_POINTERS ExceptionInfo
)
{
    PPH_STRING errorMessage;
    INT result;
    PPH_STRING message;

    if (NT_NTWIN32(ExceptionInfo->ExceptionRecord->ExceptionCode))
        errorMessage = PhGetStatusMessage(0, WIN32_FROM_NTSTATUS(ExceptionInfo->ExceptionRecord->ExceptionCode));
    else
        errorMessage = PhGetStatusMessage(ExceptionInfo->ExceptionRecord->ExceptionCode, 0);

    message = PhFormatString(
        (PWSTR)L"0x%08X (%s)",
        ExceptionInfo->ExceptionRecord->ExceptionCode,
        PhGetStringOrEmpty(errorMessage)
    );

    if (TaskDialogIndirect)
    {
        TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
        TASKDIALOG_BUTTON buttons[2];

        buttons[0].nButtonID = IDYES;
        buttons[0].pszButtonText = L"Minidump";
        buttons[1].nButtonID = IDRETRY;
        buttons[1].pszButtonText = L"Restart";

        config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
        config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
        config.pszWindowTitle = PhApplicationName;
        config.pszMainIcon = TD_ERROR_ICON;
        config.pszMainInstruction = L"Process Hacker has crashed :(";
        config.pszContent = PhGetStringOrEmpty(message);
        config.cButtons = RTL_NUMBER_OF(buttons);
        config.pButtons = buttons;
        config.nDefaultButton = IDCLOSE;

        if (SUCCEEDED(TaskDialogIndirect(&config, &result, NULL, NULL)))
        {
            switch (result)
            {
            case IDRETRY:
            {
                /*
                PhShellProcessHacker(
                NULL,
                NULL,
                SW_SHOW,
                0,
                PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                0,
                NULL
                );
                */
            }
            break;
            case IDYES:
                PhpCreateUnhandledExceptionCrashDump(ExceptionInfo); 
                break;
            }
        }
    }
    else
    {
        if (PhShowMessage(
            NULL,
            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2,
            (PWSTR)L"Process Hacker has crashed :(\r\n\r\n%s",
            (PWSTR)L"Do you want to create a minidump on the Desktop?"
        ) == IDYES)
        {
            PhpCreateUnhandledExceptionCrashDump(ExceptionInfo);
        }

        /*
        PhShellProcessHacker(
        NULL,
        NULL,
        SW_SHOW,
        0,
        PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
        0,
        NULL
        );
        */
    }

    PhExitApplication(ExceptionInfo->ExceptionRecord->ExceptionCode);

    PhDereferenceObject(message);
    PhDereferenceObject(errorMessage);

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

void    OnCommand(HWND hWnd, ULONG nId) {

    switch( nId ) {
        case IDC_CLOSE:
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            break;
    }
}
void    OnShowWindow(HWND hWnd, BOOLEAN bShow, ULONG nState) {

}
BOOLEAN OnSysCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Type,
    _In_ LONG CursorScreenX,
    _In_ LONG CursorScreenY
)
{
    switch (Type)
    {
    case SC_CLOSE:
    {
        ShowWindow(WindowHandle, SW_HIDE);
        return TRUE;
    }
    break;
    case SC_MINIMIZE:
    {
        // Save the current window state because we may not have a chance to later.
        ShowWindow(WindowHandle, SW_HIDE);
        return TRUE;
    }
    break;
    }

    return FALSE;
}
VOID OnMenuCommand(
    _In_ HWND WindowHandle,
    _In_ ULONG Index,
    _In_ HMENU Menu
)
{
    YAgent::Alert(L"%S", __FUNCTION__);
    MENUITEMINFO menuItemInfo;

    memset(&menuItemInfo, 0, sizeof(MENUITEMINFO));
    menuItemInfo.cbSize = sizeof(MENUITEMINFO);
    menuItemInfo.fMask = MIIM_ID | MIIM_DATA;

    if (GetMenuItemInfo(Menu, Index, TRUE, &menuItemInfo))
    {
        /*
        PhMwpDispatchMenuCommand(
            WindowHandle,
            Menu,
            Index,
            menuItemInfo.wID,
            menuItemInfo.dwItemData
        );
        */
    }
}
VOID OnSize(
    _In_ HWND hWnd
)
{
    if (!IsMinimized(hWnd))
    {

    }
}

VOID OnSizing(
    _In_ ULONG Edge,
    _In_ PRECT DragRectangle
)
{
    PhResizingMinimumSize(DragRectangle, Edge, 400, 340);
}

VOID PhMwpOnSetFocus(
    VOID
)
{

}

VOID OnInitMenuPopup(
    _In_ HWND WindowHandle,
    _In_ HMENU Menu,
    _In_ ULONG Index,
    _In_ BOOLEAN IsWindowMenu
);

LRESULT CALLBACK PhMwpWndProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    switch (uMsg)
    {
    case WM_DESTROY:
    {
        //  PhMwpOnDestroy(hWnd);
        PostQuitMessage(0);
    }
    break;
    case WM_ENDSESSION:
    {
        //PhMwpOnEndSession(hWnd);
    }
    break;
    case WM_SETTINGCHANGE:
    {
        //PhMwpOnSettingChange();
    }
    break;
    case WM_COMMAND:
    {
        OnCommand(hWnd, GET_WM_COMMAND_ID(wParam, lParam));
        //PhMwpOnCommand(hWnd, GET_WM_COMMAND_ID(wParam, lParam));
    }
    break;
    case WM_SHOWWINDOW:
    {
        OnShowWindow(hWnd, !!wParam, (ULONG)lParam);
        //PhMwpOnShowWindow(hWnd, !!wParam, (ULONG)lParam);
    }
    break;
    case WM_SYSCOMMAND:
    {
        OnSysCommand(hWnd, (ULONG)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        //if (PhMwpOnSysCommand(hWnd, (ULONG)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
        //   return 0;
    }
    break;
    case WM_MENUCOMMAND:
    {
        OnMenuCommand(hWnd, (ULONG)wParam, (HMENU)lParam);
        //PhMwpOnMenuCommand(hWnd, (ULONG)wParam, (HMENU)lParam);
    }
    break;
    case WM_INITMENUPOPUP:
    {
        //OnInitMenuPopup(hWnd, (HMENU)wParam, LOWORD(lParam), !!HIWORD(lParam));
        //PhMwpOnInitMenuPopup(hWnd, (HMENU)wParam, LOWORD(lParam), !!HIWORD(lParam));
    }
    break;
    case WM_SIZE:
    {
        OnSize(hWnd);
    }
    break;
    case WM_SIZING:
    {
        OnSizing((ULONG)wParam, (PRECT)lParam);
    }
    break;
    case WM_SETFOCUS:
    {
        //PhMwpOnSetFocus();
    }
    break;
    case WM_NOTIFY:
    {
        //LRESULT result;

        //if (PhMwpOnNotify((NMHDR *)lParam, &result))
        //    return result;
    }
    break;
    case WM_DEVICECHANGE:
    {
        MSG message;

        memset(&message, 0, sizeof(MSG));
        message.hwnd = hWnd;
        message.message = uMsg;
        message.wParam = wParam;
        message.lParam = lParam;

        //PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackWindowNotifyEvent), &message);
    }
    break;
    }

    //if (uMsg >= WM_PH_FIRST && uMsg <= WM_PH_LAST)
    //{
    //   return PhMwpOnUserMessage(hWnd, uMsg, wParam, lParam);
    // }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
ATOM PhMwpInitializeWindowClass()
{
    WNDCLASSEX wcex;

    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = PhMwpWndProc;
    wcex.hInstance = g_hInstance;
    wcex.lpszClassName = MAIN_WINDOW_CLASSNAME;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDR_MENU1);

    return RegisterClassEx(&wcex);
} 


LONG PhMainMessageLoop(
    VOID
)
{
    BOOL result;
    MSG message;
    HACCEL acceleratorTable;

    acceleratorTable = LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDR_USER_ACCEL));

    ShowWindow(PhMainWndHandle, SW_SHOW);
    while (GetMessage(&message, NULL, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return (LONG)message.wParam;
}

BOOLEAN PhInitializeExceptionPolicy(
    VOID
)
{
#if (PHNT_VERSION >= PHNT_WIN7)
#ifndef DEBUG
    ULONG errorMode;

    if (NT_SUCCESS(PhGetProcessErrorMode(NtCurrentProcess(), &errorMode)))
    {
        errorMode &= ~(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
        PhSetProcessErrorMode(NtCurrentProcess(), errorMode);
    }

    RtlSetUnhandledExceptionFilter(PhpUnhandledExceptionCallback);
#endif
#endif

    return TRUE;
}

VOID PhInitializeCommonControls(
    VOID
)
{
    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC =
        ICC_LISTVIEW_CLASSES |
        ICC_TREEVIEW_CLASSES |
        ICC_BAR_CLASSES |
        ICC_TAB_CLASSES |
        ICC_PROGRESS_CLASS |
        ICC_COOL_CLASSES |
        ICC_STANDARD_CLASSES |
        ICC_LINK_CLASS
        ;

    InitCommonControlsEx(&icex);
}





HICON GetApplicationIcon(
    _In_ BOOLEAN SmallIcon
)
{
    static HICON hIcon  = NULL;

    if (!hIcon)
        hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ORANGE));
    return hIcon;
}

VOID SetApplicationWindowIcon(
    _In_ HWND WindowHandle
)
{
    HICON   hIcon;

    if (hIcon = GetApplicationIcon(TRUE))
    {
        SendMessage(WindowHandle, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }
}

/*
PPH_MAIN_TAB_PAGE PhMwpCreateInternalPage(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_ PPH_MAIN_TAB_PAGE_CALLBACK Callback
)
{
    PH_MAIN_TAB_PAGE page;

    memset(&page, 0, sizeof(PH_MAIN_TAB_PAGE));
    PhInitializeStringRef(&page.Name, Name);
    page.Flags = Flags;
    page.Callback = Callback;

    return PhMwpCreatePage(&page);
}
*/

void    InitializeMainWindowControls(HWND hWnd) {

    TabControlHandle = CreateWindow(
        WC_TABCONTROL,
        NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_MULTILINE,
        0,
        0,
        3,
        3,
        hWnd,
        NULL,
        g_hInstance,
        NULL
    );
}

void	CreateMainWindowMenu(IN HWND hWnd);
int	CreateMainWindow(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR    lpCmdLine,
    int       nCmdShow
)
{
    g_hInstance     = hInstance;

    
    PhInitializeCommonControls();
    if (!NT_SUCCESS(PhInitializePhLibEx((PWSTR)L"Process Hacker", ULONG_MAX, g_hInstance, 0, 0)))
        return 1;
    //if (!PhInitializeDirectoryPolicy())
    //    return 1;
    //if (!PhInitializeExceptionPolicy())
    //    return 1;
    //if (!PhInitializeNamespacePolicy())
    //    return 1;
    //if (!PhInitializeMitigationPolicy())
    //    return 1;

    PH_RECTANGLE windowRectangle    = {100,100,800,600};

    ATOM        windowAtom  = PhMwpInitializeWindowClass();


    if( INVALID_ATOM == windowAtom )    {
        MessageBoxA(NULL, "-1", "", 0);
        return -1;
    }
    RECT    rect    = {100,100,100,100};

    PhMainWndHandle  = CreateWindow(MAKEINTATOM(windowAtom), MAIN_WINDOW_NAME, WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN, 
        windowRectangle.Left,
        windowRectangle.Top,
        windowRectangle.Width,
        windowRectangle.Height,
        NULL, NULL, NULL, NULL);

   // CreateMainWindowMenu(PhMainWndHandle);

    SetApplicationWindowIcon(PhMainWndHandle);
    // Choose a more appropriate rectangle for the window.
    PhAdjustRectangleToWorkingArea(PhMainWndHandle, &windowRectangle);
    MoveWindow(
        PhMainWndHandle, 
        windowRectangle.Left, windowRectangle.Top,
        windowRectangle.Width, windowRectangle.Height,
        FALSE
    );
    UpdateWindow(PhMainWndHandle);

    InitializeMainWindowControls(PhMainWndHandle);
   // PhInitializeWindowTheme(PhMainWndHandle, PhEnableThemeSupport); // HACK

    LONG    nRet    = PhMainMessageLoop();
    PhExitApplication(nRet);
    return 0;
}