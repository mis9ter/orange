#pragma once
#include "CWebview.h"
#include "CIPCCommandCallback.h"

class CWebApp
	:
	public	CWebview,
	public	CIPC,
	public	CIPCCommand,
	public	CIPCCommandCallback,
	virtual	public	CAppLog

{
public:
	CWebApp() 
		:
		CAppLog(WEBVIEW_LOG_NAME),
		m_hInstance(NULL),
		m_hWnd(NULL)
	{
	
	}
	~CWebApp() {
	
	}

	bool	Initialize(HINSTANCE hInstance, int nCmdShow) {
		m_hInstance	= hInstance;

		if( false == CheckWebview() )
		return false;

		WCHAR		szWindowClass[] = _T("DesktopApp");
		WCHAR		szTitle[]		= _T("B");

		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = m_hInstance;
		wcex.hIcon = LoadIcon(m_hInstance, IDI_APPLICATION);
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

		m_hWnd = CreateWindow(
			szWindowClass,
			szTitle,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			800, 600,
			NULL,
			NULL,
			hInstance,
			this
		);

		if (NULL == m_hWnd)
		{
			MessageBox(NULL,
				_T("Call to CreateWindow failed!"),
				_T("Windows Desktop Guided Tour"),
				NULL);
			return false;
		}

		ShowWindow(m_hWnd, nCmdShow);
		UpdateWindow(m_hWnd);

		HICON hicon = (HICON)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_SMALL), 
			IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);
		SendMessageW(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hicon);
		return true;
	}
	bool	Run() {
		WCHAR	szStartPath[AGENT_PATH_SIZE]	= L"";
		YAgent::GetDataFolder(AGENT_DATA_FOLDERNAME L"\\User", szStartPath, sizeof(szStartPath));
		YAgent::MakeDirectory(szStartPath);

		WCHAR	szStartPage[AGENT_PATH_SIZE]	= L"";
		StringCbPrintf(szStartPage, sizeof(szStartPage), L"%s\\index.html", szStartPath);

		SetWebMessageCallback(this, WebMessageHandler);
		CWebview::Run(m_hInstance, m_hWnd, szStartPath, szStartPage);
		CIPC::SetServiceCallback(IPCRecvCallback, this);
		if( CIPC::Start(AGENT_WEBAPP_PIPE_NAME, true) ) {
			Log("%s ipc pipe created.", __FUNCTION__);
		}
		else {
			Log("%s ipc pipe not created. code=%d", __FUNCTION__, GetLastError());
		}
		CWebview::MessagePump();
		return true;
	}
	void	Destroy() {
		CIPC::Shutdown();	
	}
	void		SetResult(
		IN	PCSTR	pFile, PCSTR pFunction, int nLine,
		IN	Json::Value & res, IN bool bRet, IN int nCode, IN PCSTR pMsg /*utf8*/
	) {
		Json::Value	&_result	= res["result"];

		_result["ret"]	= bRet;
		_result["code"]	= nCode;
		_result["msg"]	= pMsg;
		_result["file"]	= pFile;
		_result["line"]	= nLine;
		_result["function"]	= pFunction;

		if( false == bRet ) {
			Log("%-32s %s(%d) %s", pFunction, pFile, nLine, pMsg);		
		}
	}

private:
	HINSTANCE		m_hInstance;
	HWND			m_hWnd;

	bool	CheckWebview() {
		if( false == CWebview::IsWebview2Installed() ) 
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
				//ERROR_NOT_FOUND;
				return false;
			}
			if( false == CWebview::IsWebview2Installed() ) {
				YAgent::Alert(L"에버그린이 설치되지 않은 관계로 전 이만..");	
				ERROR_NOT_FOUND;
				return false;
			}
		}
		return true;	
	}
	static	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		static	CWebApp		*pClass	= NULL;
		switch (message)
		{
		case WM_CREATE:
		{
			CREATESTRUCT	*p	= (CREATESTRUCT *)lParam;
			if( p ) {
				pClass	= (CWebApp *)p->lpCreateParams;
			}
			else {

			}
		}
		break;
		case WM_SIZE:
			if( pClass )
				pClass->OnSize(hWnd);
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
	bool	ServiceMessageHandler(Json::Value & req, Json::Value & res) 
	{
		CIPC	client;
		HANDLE	hClient;

		Log("%-32s", __func__);
		if( INVALID_HANDLE_VALUE != (hClient = client.Connect(AGENT_SERVICE_PIPE_NAME, __func__)) ) {

			Log("%-32s connected", __FUNCTION__);
			IPCHeader   header  = {IPCJson, };

			std::string					str;
			Json::StreamWriterBuilder	builder;
			builder["indentation"]	= "";
			str	= Json::writeString(builder, req);
			header.dwSize   = (DWORD)(str.length() + 1);

			DWORD   dwBytes = 0;

			if( client.Request(__FUNCTION__, hClient, (PVOID)str.c_str(), (DWORD)str.length()+1,
				[&](PVOID pResponseData, DWORD dwResponseSize) {
				Log("%-32s response %d bytes", __FUNCTION__, dwResponseSize);
				Log("%s", pResponseData);

				Json::CharReaderBuilder	rbuilder;
				const std::unique_ptr<Json::CharReader>		reader(rbuilder.newCharReader());

				std::string		errors;
				try {
					if( reader->parse((const char *)pResponseData, (const char *)pResponseData + dwResponseSize, 
						&res, &errors) ) {

					}
				}
				catch( std::exception & e) {
					res["exception"]["function"]	= __func__;
					res["exception"]["file"]		= __FUNCTION__;
					res["exception"]["line"]		= __LINE__;
					res["exception"]["msg"]			= e.what();
				}

			}) ) {



			}
			client.Disconnect(hClient, __FUNCTION__);
		}
		else {
			MessageBox(NULL, L"IPC not connected", L"", 0);
			return false;
		}
		return true;
	}
	static	bool	WebMessageHandler(IN PVOID pContext, IN Json::Value & req, IN Json::Value & res) {
		CWebApp		*pClass	= (CWebApp *)pContext;

		pClass->Log("%-32s", __func__);
		pClass->ServiceMessageHandler(req, res);
		return true;
	}
	static	bool	IPCRecvCallback(
		IN	HANDLE		hClient,
		IN	IIPCClient	*pClient,
		IN	void		*pContext,
		IN	HANDLE		hShutdown
	);
};
