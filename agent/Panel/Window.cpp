#include "framework.h"

#define MAIN_WINDOW_CLASSNAME   L"Orange.User"
#define MAIN_WINDOW_NAME        L"Orange.User"
HINSTANCE               g_hInstance;
HWND                    m_hWnd;
HWND                    m_hTab;
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
void OnSize(const HWND hwnd,int cx,int cy,UINT flags)
{
    // Get the header control handle which has been previously stored in the user
    // data associated with the parent window.
    HWND hTabCntrl=reinterpret_cast<HWND>(static_cast<LONG_PTR>
        (GetWindowLongPtr(hwnd,GWLP_USERDATA)));

    MoveWindow(hTabCntrl, 2, 2, cx - 4, cy - 4, TRUE);
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
        OnSize(hWnd, LOWORD(lParam), HIWORD(lParam), static_cast<UINT>(wParam));
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

    ShowWindow(m_hWnd, SW_SHOW);
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

HWND	m_hWndTab;
enum {
    IDC_TAB=200,
};

HWND CreateTab(const HWND hParent,const HINSTANCE hInst,DWORD dwStyle,
    const RECT& rc,const int id)
{
    dwStyle |= WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_MULTILINE;

    return CreateWindowEx(0,    //  extended styles
        WC_TABCONTROL,          //  control 'class' name
        0,                      //  control caption
        dwStyle,                //  wnd style
        rc.left,                //  position: left
        rc.top,                 //  position: top
        rc.right,               //  width
        rc.bottom,              //  height
        hParent,                //  parent window handle
                                //  control's ID
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        hInst,              //instance
        0);                 //user defined info
}

int InsertItem(HWND hTc,const std::wstring & txt,int item_index,int image_index,
    UINT mask = TCIF_TEXT|TCIF_IMAGE)
{
    std::vector<TCHAR> tmp(txt.begin(),txt.end());
    tmp.push_back(_T('\0'));

    TCITEM tabPage={0};

    tabPage.mask=mask;
    tabPage.pszText=&tmp[0];
    tabPage.cchTextMax=static_cast<int>(txt.length());
    tabPage.iImage=image_index;
    return static_cast<int>(SendMessage(hTc,TCM_INSERTITEM,item_index,
        reinterpret_cast<LPARAM>(&tabPage)));
}
int AddTabControl(HWND hWnd, int nIndex, PCWSTR pStr)
{
    TCITEM item;

    item.mask       = TCIF_TEXT;
    item.pszText    = (PWSTR)pStr;

    return TabCtrl_InsertItem(hWnd, nIndex, &item);
}

void StartCommonControls(DWORD flags)
{
    INITCOMMONCONTROLSEX iccx;
    iccx.dwSize=sizeof(INITCOMMONCONTROLSEX);
    iccx.dwICC=flags;
    InitCommonControlsEx(&iccx);
}
void	InitializeTabControl(IN HINSTANCE hInstance, IN HWND hWnd)
{

    RECT rc={0,0,0,0};
    StartCommonControls(ICC_TAB_CLASSES);

    HWND hTabCntrl=CreateTab(hWnd, hInstance,TCS_FIXEDWIDTH,rc, IDC_TAB);

    // Store the tab control handle as the user data associated with the
    // parent window so that it can be retrieved for later use
    SetWindowLongPtr(hWnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(hTabCntrl));

    AddTabControl(hTabCntrl, 0, L"프로세스");
    AddTabControl(hTabCntrl, 1, L"Page 2");
    AddTabControl(hTabCntrl, 2, L"Page 3");

    //set the font of the tabs to a more typical system GUI font
    SendMessage(hTabCntrl,WM_SETFONT,
        reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),0);                   
}

void    DestroyTabControl(const HWND hwnd)
{
    HWND hTabCntrl=reinterpret_cast<HWND>(static_cast<LONG_PTR>
        (GetWindowLongPtr(hwnd,GWLP_USERDATA)));

    HIMAGELIST hImages=reinterpret_cast<HIMAGELIST>(SendMessage(hTabCntrl,
        TCM_GETIMAGELIST,0,0)); 

    ImageList_Destroy(hImages);
    PostQuitMessage(0);
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

    m_hWnd  = CreateWindow(MAKEINTATOM(windowAtom), MAIN_WINDOW_NAME, WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN, 
        windowRectangle.Left,
        windowRectangle.Top,
        windowRectangle.Width,
        windowRectangle.Height,
        NULL, NULL, NULL, NULL);

   // CreateMainWindowMenu(m_hWnd);

    SetApplicationWindowIcon(m_hWnd);
    // Choose a more appropriate rectangle for the window.
    PhAdjustRectangleToWorkingArea(m_hWnd, &windowRectangle);
    MoveWindow(
        m_hWnd, 
        windowRectangle.Left, windowRectangle.Top,
        windowRectangle.Width, windowRectangle.Height,
        FALSE
    );
    UpdateWindow(m_hWnd);

    //InitializeTabControl(g_hInstance, m_hWnd);
   // PhInitializeWindowTheme(m_hWnd, PhEnableThemeSupport); // HACK

    LONG    nRet    = PhMainMessageLoop();
    PhExitApplication(nRet);
    return 0;
}