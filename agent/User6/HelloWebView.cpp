// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c

#include "pch.h"

// Global variables

// The main window class name.
static TCHAR szWindowClass[] = _T("DesktopApp");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("B");

HINSTANCE hInst;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Pointer to WebViewController
static wil::com_ptr<ICoreWebView2Controller> webviewController;

// Pointer to WebView window
static wil::com_ptr<ICoreWebView2> webviewWindow;

#define WEBVIEW2_x64_INSTALL_ROOTKEY	L"SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients"
#define WEBVIEW2_x86_INSTALL_ROOTKEY	L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"
#define WEBVIEW2_INSTALL_KEY		L"{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"

CAppLog	g_log(L"user6.log");

bool	IsWebview2Installed(PWSTR pVer = NULL, DWORD dwVerSize = 0) {
	//	x64	
	//	HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}
	//	x86
	//	HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}

	bool	bRet	= CAppRegistry::IsExistingKey(HKEY_LOCAL_MACHINE, WEBVIEW2_x64_INSTALL_ROOTKEY, WEBVIEW2_INSTALL_KEY);

	PWSTR	pVersion	= NULL;
	HRESULT	hr			= GetAvailableCoreWebView2BrowserVersionString(NULL, &pVersion);
	g_log.Log("%-32s %d %ws", __func__, bRet, pVersion);

	if( SUCCEEDED(hr) && pVersion && pVer && dwVerSize ) {
		StringCbCopy(pVer, dwVerSize, pVersion);
	}
	if( bRet && pVersion && SUCCEEDED(hr))	return true;
	return false;
}

void	MessageHandlerByWebView(
	ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs * args) {

	PWSTR message;
	if(SUCCEEDED(args->TryGetWebMessageAsString(&message)) ) {

		g_log.Log(__func__);
		g_log.Log("%ws", message);


		webview->PostWebMessageAsString(message);
		CoTaskMemFree(message);
	}
}

//	https://github.com/MicrosoftEdge/WebView2Samples/blob/def7237dd5b70cc54ee480710df2e0f2ce09690e/SampleApps/WebView2APISample/AppWindow.cpp#L802
HMENU	g_hMenu;
RECT	g_previousWindowRect;

void	EnterFullScreen(HWND hWnd)
{
	DWORD style = GetWindowLong(hWnd, GWL_STYLE);
	MONITORINFO monitor_info = {sizeof(monitor_info)};
	g_hMenu = ::GetMenu(hWnd);
	::SetMenu(hWnd, nullptr);
	if (GetWindowRect(hWnd, &g_previousWindowRect) &&
		GetMonitorInfo(
			MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), &monitor_info))
	{
		SetWindowLong(hWnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
		SetWindowPos(
			hWnd, HWND_TOP, monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
			monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
			monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}

void	ExitFullScreen(HWND hWnd)
{
	DWORD style = GetWindowLong(hWnd, GWL_STYLE);
	::SetMenu(hWnd, g_hMenu);
	SetWindowLong(hWnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
	SetWindowPos(
		hWnd, NULL, g_previousWindowRect.left, g_previousWindowRect.top,
		g_previousWindowRect.right - g_previousWindowRect.left,
		g_previousWindowRect.bottom - g_previousWindowRect.top,
		SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{
	if( false == IsWebview2Installed() ) 
	{
		WCHAR	szPackagePath[1024]	= L"";
		DWORD	dwPackageSize		= 0;
		//	https://go.microsoft.com/fwlink/p/?LinkId=2124703

		//YAgent::Alert(L"에버그린이란 것을 설치 좀 하겠습니다.");

		YAgent::GetModulePath(szPackagePath, sizeof(szPackagePath));
		StringCbCat(szPackagePath, sizeof(szPackagePath), L"\\webview2.exe");
		if( CHttp::RequestFile(L"https://go.microsoft.com/fwlink/p/?LinkId=2124703", szPackagePath, &dwPackageSize, NULL, NULL) ) {
			HANDLE	h	= YAgent::Run(szPackagePath, NULL);
			if( h ) {
				WaitForSingleObject(h, INFINITE);
				CloseHandle(h);			
				DeleteFile(szPackagePath);
			}		
		}
		else {
			YAgent::Alert(L"에버그린 부트스트래퍼 다운로드 실패!");
			return ERROR_NOT_FOUND;
		}
	}
	if( false == IsWebview2Installed() ) {
		YAgent::Alert(L"에버그린이 설치되지 않은 관계로 전 이만..");	
		return ERROR_NOT_FOUND;
	}

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL,
			_T("Call to RegisterClassEx failed!"),
			_T("Windows Desktop Guided Tour"),
			NULL);

		return 1;
	}

	// Store instance handle in our global variable
	hInst = hInstance;

	// The parameters to CreateWindow explained:
	// szWindowClass: the name of the application
	// szTitle: the text that appears in the title bar
	// WS_OVERLAPPEDWINDOW: the type of window to create
	// CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
	// 500, 100: initial size (width, length)
	// NULL: the parent of this window
	// NULL: this application does not have a menu bar
	// hInstance: the first parameter from WinMain
	// NULL: not used in this application
/*
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED     | \
                             WS_CAPTION        | \
                             WS_SYSMENU        | \
                             WS_THICKFRAME     | \
                             WS_MINIMIZEBOX    | \
                             WS_MAXIMIZEBOX)
*/
	HWND hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		600, 600,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!hWnd)
	{
		MessageBox(NULL,
			_T("Call to CreateWindow failed!"),
			_T("Windows Desktop Guided Tour"),
			NULL);

		return 1;
	}

	// The parameters to ShowWindow explained:
	// hWnd: the value returned from CreateWindow
	// nCmdShow: the fourth parameter from WinMain
	ShowWindow(hWnd,
		nCmdShow);
	UpdateWindow(hWnd);

	HICON hicon = (HICON)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_SMALL), 
		IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);
	SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hicon);


	WCHAR	szDataPath[AGENT_PATH_SIZE]	= L"";
	YAgent::GetDataFolder(AGENT_DATA_FOLDERNAME L"\\User", szDataPath, sizeof(szDataPath));
	YAgent::MakeDirectory(szDataPath);

	WCHAR	szStartPage[AGENT_PATH_SIZE]	= L"";
	StringCbPrintf(szStartPage, sizeof(szStartPage), L"%s\\index.html", szDataPath);

	// <-- WebView2 sample code starts here -->

	// Step 3 - Create a single WebView within the parent window
	// Locate the browser and set up the environment for WebView
	HRESULT	hr;

	hr	= CreateCoreWebView2EnvironmentWithOptions(nullptr, szDataPath, nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[&](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

		// Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
		HRESULT	hr;
		hr	= env->CreateCoreWebView2Controller(hWnd, 
			Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>
			(
				[&](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT 
		{
			if (controller != nullptr) {
				webviewController = controller;
				webviewController->get_CoreWebView2(&webviewWindow);
			}

			// Add a few settings for the webview
			// The demo step is redundant since the values are the default settings
			ICoreWebView2Settings* Settings;
			webviewWindow->get_Settings(&Settings);
			Settings->put_IsScriptEnabled(TRUE);
			Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
			Settings->put_IsWebMessageEnabled(TRUE);
			Settings->put_AreDevToolsEnabled(TRUE);
			// Resize WebView to fit the bounds of the parent window
			RECT bounds;
			GetClientRect(hWnd, &bounds);
			webviewController->put_Bounds(bounds);

			EventRegistrationToken	documentTitleChangedToken;

			CHECK_FAILURE(
				webviewWindow->add_DocumentTitleChanged
				(
					Callback<ICoreWebView2DocumentTitleChangedEventHandler>
					(
						[hWnd](ICoreWebView2* sender, IUnknown* args) -> HRESULT {
				wil::unique_cotaskmem_string title;
				CHECK_FAILURE(sender->get_DocumentTitle(&title));
				SetWindowText(hWnd, title.get());
				return S_OK;
			}
						).Get(), &documentTitleChangedToken)
			);

			// Register a handler for the ContainsFullScreenChanged event.
			bool	bFullScreen	= true;
			BOOL	bContainsFullscreenElement	= FALSE;
			CHECK_FAILURE(
				webviewWindow->add_ContainsFullScreenElementChanged
				(
					Callback<ICoreWebView2ContainsFullScreenElementChangedEventHandler>
					(
						[&](ICoreWebView2* sender, IUnknown* args) -> HRESULT {
						if (bFullScreen)
						{
							CHECK_FAILURE(
								sender->get_ContainsFullScreenElement(&bContainsFullscreenElement)
							);
							if (bContainsFullscreenElement)
							{
								EnterFullScreen(hWnd);
							}
							else
							{
								ExitFullScreen(hWnd);
							}
						}
						return S_OK;
				}).Get(),nullptr)
			);


			// Step 4 - Navigation events

			EventRegistrationToken token;
			webviewWindow->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
				[](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs * args) -> HRESULT {
				PWSTR uri;
				args->get_Uri(&uri);
				std::wstring source(uri);
				g_log.Log("%ws", uri);
				if (source.substr(0, 5) != L"https") {
					//args->put_Cancel(true);
				}
				CoTaskMemFree(uri);
				return S_OK;
			}).Get(), &token);
			webviewWindow->add_ContentLoading(Callback<ICoreWebView2ContentLoadingEventHandler>(
				[](ICoreWebView2* webview, ICoreWebView2ContentLoadingEventArgs * args) -> HRESULT {
				ULONG64	nID	= 0;
				args->get_NavigationId(&nID);
				g_log.Log("%I64d", nID);
				return S_OK;
			}).Get(), &token);




			// Step 5 - Scripting

#ifdef TEST
			// Schedule an async task to add initialization script that freezes the Object object
			webviewWindow->AddScriptToExecuteOnDocumentCreated(L"Object.freeze(Object);", nullptr);
			// Schedule an async task to get the document URL
			webviewWindow->ExecuteScript(L"window.document.URL;", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
				[](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT {
				LPCWSTR URL = resultObjectAsJson;
				//doSomethingWithURL(URL);
				return S_OK;
			}).Get());
#endif

			// Step 6 - Communication between host and web content
			// Set an event handler for the host to return received message back to the web content
			webviewWindow->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
				[](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs * args) -> HRESULT {
				MessageHandlerByWebView(webview, args);
				return S_OK;
			}).Get(), &token);



			// Schedule an async task to add initialization script that
			// 1) Add an listener to print message from the host
			// 2) Post document URL to the host
			//webviewWindow->AddScriptToExecuteOnDocumentCreated(
			//	L"window.chrome.webview.addEventListener(\'message\', event => alert(event.data));" \
								//	L"window.chrome.webview.postMessage(window.document.URL);",
//	nullptr);

// Schedule an async task to navigate to Bing
			if( SUCCEEDED(webviewWindow->Navigate(szStartPage)) ) {

			}
			//webviewWindow->Navigate(L"https://172.29.59.18:8443/mc/#/login");
			return S_OK;
		}
				).Get()
			);
		if( FAILED(hr) ) {
			MessageBox(NULL,
				_T("CreateCoreWebView2Controller() failed!"),
				_T("Windows Desktop Guided Tour"),
				NULL);
		}
		return S_OK;
	}).Get());
	if( FAILED(hr) ) {
		MessageBox(NULL,
			_T("CreateCoreWebView2EnvironmentWithOptions() failed!"),
			_T("Windows Desktop Guided Tour"),
			NULL);
	}

	// <-- WebView2 sample code ends here -->

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_DESTROY  - post a quit message and return
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR greeting[] = _T("Hello, Windows desktop!");

	switch (message)
	{
	case WM_SIZE:
		if (webviewController != nullptr) {
			RECT bounds;
			GetClientRect(hWnd, &bounds);
			webviewController->put_Bounds(bounds);
		};
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}
