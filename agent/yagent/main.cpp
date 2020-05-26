// yagent.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "stdio.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void    Console(HANDLE hShutdown, void* ptr)
{

}
void    AgentRunLoop(HANDLE hShutdown, void* pCallbackPtr)
{

}
#ifdef _CONSOLE
int     main()
{
   CAgent       agent;
   char         szCmd[100]  = "";

   CDialog      dialog;
   dialog.SetMessageCallbackPtr(&agent);
   if (dialog.Create(IDD_CONTROL_DIALOG, AGENT_SERVICE_NAME))
   {
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
           CAgent * pAgent  = (CAgent *)ptr;
           pAgent->Start(pAgent, AgentRunLoop);
       });
       dialog.AddMessageCallback(IDC_BUTTON_SHUTDOWN, [](
           IN	HWND	hWnd,
           IN	UINT	nMsgId,
           IN	WPARAM	wParam,
           IN	LPARAM	lParam,
           IN	PVOID	ptr
           ) {
           CAgent* pAgent = (CAgent*)ptr;
           pAgent->Shutdown();
           PostQuitMessage(0);
       });
       dialog.MessagePump([](IN HANDLE hShutdown, IN HWND hWnd, IN PVOID ptr) {
           while (WAIT_TIMEOUT == WaitForSingleObject(hShutdown, 100))
           {
               
           }
       });
       dialog.Destroy();
   }
   /*
   if (agent.Start(NULL, [&](HANDLE hShutdown, LPVOID pCallbackPtr) {
        bool    bRun    = true;    
        while( bRun )
        {
            if( WAIT_OBJECT_0 == WaitForSingleObject(hShutdown, 0) )
                break;
            printf("command(q,i,u,s,t):");
            gets_s(szCmd);
            if (1 == lstrlenA(szCmd))
            {
                switch (szCmd[0])
                {
                    case 's':
                        puts("  -- start");
                        agent.Start(NULL, AgentRunLoop);
                        break;
                    case 't':
                        puts("  -- shutdown");
                        agent.Shutdown();
                        break;
					case 'i':
						puts("  -- install");
						agent.Install();
						break;
					case 'u':
						puts("  -- uninstall");
						agent.Uninstall();
						break;
					case 'q':
						puts("  -- quit");
						bRun    = false;
						break;
					case '\r':
					case '\n':
						break;
					default:
						puts("  -- unknown command");
						break;
                }
            }
            //printf("%-30s pCallbackPtr=%p\n", __FUNCTION__, pCallbackPtr);
        }
   }))
   {
       agent.RunLoop();
   }
   */
   return 0;
}
#else
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);


    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_YAGENT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_YAGENT));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}
#endif
//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_YAGENT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_YAGENT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
