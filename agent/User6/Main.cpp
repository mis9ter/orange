#include "pch.h"
#include "CIpc.h"
#include "CAppLog.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int		W1(HINSTANCE hInstance, int nCmdShow);
int		W2(HINSTANCE hInstance, int nCmdShow);


CAppLog     g_log(L"user6.log");

bool	ServiceMessageHandler(Json::Value & req, Json::Value & res) 
{
	CIPc	client;
	HANDLE	hClient;

	g_log.Log("%-32s", __func__);

	if( hClient = client.Connect(AGENT_PIPE_NAME, __func__) ) {

		g_log.Log("%-32s connected", __FUNCTION__);
		IPCHeader   header  = {IPCJson, };

		std::string					str;
		Json::StreamWriterBuilder	builder;
		builder["indentation"]	= "";
		str	= Json::writeString(builder, req);
		header.dwSize   = (DWORD)(str.length() + 1);

		DWORD   dwBytes = 0;

		if( client.Request(__FUNCTION__, hClient, (PVOID)str.c_str(), (DWORD)str.length()+1,
			[&](PVOID pResponseData, DWORD dwResponseSize) {
			g_log.Log("%-32s response %d bytes", __FUNCTION__, dwResponseSize);
			g_log.Log("%s", pResponseData);

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
	}



}

bool	WebMessageHandler(IN PVOID pContext, IN Json::Value & req, IN Json::Value & res) {

	g_log.Log("%-32s", __func__);
	ServiceMessageHandler(req, res);
	/*
	res.clear();

	auto	&header	= req["header"];
	res["header"]	= req["header"];
	if( !header["command"].asString().compare("pproduct.header")) {
		Json::Value	column;

		column["text"]	= __utf8(L"제품명");
		column["value"]	= "a";
		res["data"].append(column);
		column["text"]	= __utf8(L"설치일");
		column["value"]	= "b";
		res["data"].append(column);
		column["text"]	= __utf8(L"제조사");
		column["value"]	= "c";
		res["data"].append(column);
	}
	else if( !header["command"].asString().compare("pproduct.data"))
	{
		Json::Value	row;

		for( auto i = 0 ; i < 100 ; i++ ) {
			row.clear();
			row["key"]	= i;

			row["a"]	= std::to_string(i);
			row["b"]	= std::to_string(i);
			row["c"]	= std::to_string(i);
			res["data"].append(row);
		}
	}
	*/
	return true;

}
int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
) {
	return W2(hInstance, nCmdShow);
}
int		W2(HINSTANCE hInstance, int nCmdShow)
{
	WCHAR		szWindowClass[] = _T("DesktopApp");
	WCHAR		szTitle[]		= _T("B");
	CWebview	webview(hInstance);

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
			return ERROR_NOT_FOUND;
		}
	}
	if( false == CWebview::IsWebview2Installed() ) {
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

	HWND hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		800, 600,
		NULL,
		NULL,
		hInstance,
		&webview
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
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	HICON hicon = (HICON)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_SMALL), 
		IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);
	SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hicon);


	WCHAR	szStartPath[AGENT_PATH_SIZE]	= L"";
	YAgent::GetDataFolder(AGENT_DATA_FOLDERNAME L"\\User", szStartPath, sizeof(szStartPath));
	YAgent::MakeDirectory(szStartPath);

	WCHAR	szStartPage[AGENT_PATH_SIZE]	= L"";
	StringCbPrintf(szStartPage, sizeof(szStartPage), L"%s\\index.html", szStartPath);

	webview.SetWebMessageCallback(NULL, WebMessageHandler);
	webview.Run(hWnd, szStartPath, szStartPage);

	return 0;
}
static	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static	CWebview	*pClass	= NULL;
	switch (message)
	{
		case WM_CREATE:
			{
				CREATESTRUCT	*p	= (CREATESTRUCT *)lParam;
				if( p ) {
					pClass	= (CWebview *)p->lpCreateParams;
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
