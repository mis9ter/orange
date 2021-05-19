#pragma once

#include <functional>
#include <wrl.h>
#include <wil/com.h>
using namespace Microsoft::WRL;

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>

#include "CheckFailure.h"
#include "WebView2.h"

#include "CAppRegistry.h"
#include "CAppLog.h"
#include "yagent.common.h"
#include "Chttp.h"
#include "CIpc.h"

#define WEBVIEW_LOG_NAME				L"webview.log"
#define	WEBVIEW_CLASS_NAME2				L"OrangeClass"
#define WEBVIEW_APP_TITLE2				L"B"
#define WEBVIEW2_x64_INSTALL_ROOTKEY	L"SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients"
#define WEBVIEW2_x86_INSTALL_ROOTKEY	L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"
#define WEBVIEW2_INSTALL_KEY			L"{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"

typedef std::function<bool (IN PVOID pContext, IN Json::Value & req, OUT Json::Value & res)>	
	WebMessageCallback;


class CWebview
	:
	virtual	public	CAppLog
{
public:
	CWebview() 
	:
		CAppLog(WEBVIEW_LOG_NAME),
		m_szWindowClass(WEBVIEW_CLASS_NAME2),
		m_szAppTitle(WEBVIEW_APP_TITLE2),
		m_hInstance(NULL),
		m_webviewController(nullptr),
		m_webviewWindow(nullptr)
	{
		ZeroMemory(&m_callback, sizeof(m_callback));
	}
	~CWebview() {
	
	}
	static	bool	Json2Utf16(const Json::Value & doc, std::wstring & wstr) {
		Json::StreamWriterBuilder	builder;
		std::string		str		= Json::writeString(builder, doc);
		wstr	= __utf16(str);
		return true;
	}
	static	bool	IsWebview2Installed(PWSTR pVer = NULL, DWORD dwVerSize = 0) {
		//	x64	
		//	HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}
		//	x86
		//	HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}

		bool	bRet	= CAppRegistry::IsExistingKey(HKEY_LOCAL_MACHINE, WEBVIEW2_x64_INSTALL_ROOTKEY, WEBVIEW2_INSTALL_KEY);

		PWSTR	pVersion	= NULL;
		HRESULT	hr			= GetAvailableCoreWebView2BrowserVersionString(NULL, &pVersion);

		if( SUCCEEDED(hr) && pVersion && pVer && dwVerSize ) {
			StringCbCopy(pVer, dwVerSize, pVersion);
		}
		if( bRet && pVersion && SUCCEEDED(hr))	return true;
		return false;
	}

	void			SetWebMessageCallback(IN PVOID pContext, IN WebMessageCallback pCallback) {
		m_callback.webMessage.pContext		= pContext;
		m_callback.webMessage.pCallback		= pCallback;	
	}
	bool			Run(IN HINSTANCE hInstance, IN HWND hWnd, IN PCWSTR pStartPath, IN PCWSTR pStartPage) {
		// <-- WebView2 sample code starts here -->

		// Step 3 - Create a single WebView within the parent window
		// Locate the browser and set up the environment for WebView
		m_hInstance	= hInstance;

		HRESULT	hr;
		hr	= CreateCoreWebView2EnvironmentWithOptions(nullptr, pStartPath, nullptr,
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
					m_webviewController = controller;
					m_webviewController->get_CoreWebView2(&m_webviewWindow);
				}

				// Add a few settings for the webview
				// The demo step is redundant since the values are the default settings
				ICoreWebView2Settings* Settings;
				m_webviewWindow->get_Settings(&Settings);
				Settings->put_IsScriptEnabled(TRUE);
				Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
				Settings->put_IsWebMessageEnabled(TRUE);
				Settings->put_AreDevToolsEnabled(TRUE);
				// Resize WebView to fit the bounds of the parent window
				RECT bounds;
				GetClientRect(hWnd, &bounds);
				m_webviewController->put_Bounds(bounds);

				EventRegistrationToken	documentTitleChangedToken;

				CHECK_FAILURE(
					m_webviewWindow->add_DocumentTitleChanged
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
					m_webviewWindow->add_ContainsFullScreenElementChanged
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
							//EnterFullScreen(hWnd);
						}
						else
						{
							//ExitFullScreen(hWnd);
						}
					}
					return S_OK;
				}).Get(),nullptr)
				);


				// Step 4 - Navigation events

				EventRegistrationToken token;
				m_webviewWindow->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
					[&](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs * args) -> HRESULT {
					PWSTR uri;
					args->get_Uri(&uri);
					std::wstring source(uri);
					Log("%ws", uri);
					if (source.substr(0, 5) != L"https") {
						//args->put_Cancel(true);
					}
					CoTaskMemFree(uri);
					return S_OK;
				}).Get(), &token);
				m_webviewWindow->add_ContentLoading(Callback<ICoreWebView2ContentLoadingEventHandler>(
					[&](ICoreWebView2* webview, ICoreWebView2ContentLoadingEventArgs * args) -> HRESULT {
					ULONG64	nID	= 0;
					args->get_NavigationId(&nID);
					Log("%I64d", nID);
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
				m_webviewWindow->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
					[&](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs * args) -> HRESULT {
					MessageHandler(webview, args);
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
				if( SUCCEEDED(m_webviewWindow->Navigate(pStartPage)) ) {

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
	
		// Main message loop:
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return true;
	}
	void	OnSize(IN HWND hWnd) {
		if (m_webviewController != nullptr) {
			RECT bounds;
			GetClientRect(hWnd, &bounds);
			m_webviewController->put_Bounds(bounds);
		};
	}
private:
	// The main window class name.
	WCHAR		m_szWindowClass[100];
	// The string that appears in the application's title bar.
	WCHAR		m_szAppTitle[100];
	// Pointer to WebViewController
	wil::com_ptr<ICoreWebView2Controller>	m_webviewController;

	// Pointer to WebView window
	wil::com_ptr<ICoreWebView2>				m_webviewWindow;

	HINSTANCE	m_hInstance;
	// Forward declarations of functions included in this code module:
	
	struct {
		struct {
			WebMessageCallback	pCallback;
			PVOID				pContext;
		} webMessage;
	
	}	m_callback;
	void	MessageHandler
	(
		ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs * args) {

		CoPWSTR		message;
		CoPWSTR		source;
		HRESULT		hr;

		if( SUCCEEDED(args->get_Source(source))) {
			Log("%ws", source);
		}
		if(SUCCEEDED(args->TryGetWebMessageAsString(message)) ) {

			Log(__func__);
			//g_log.Log("%ws", message);



			Json::Value		req, res;

			Json::CharReaderBuilder	builder;
			const std::unique_ptr<Json::CharReader>		reader(builder.newCharReader());

			std::string		str		= __utf8(message);
			std::string		errors;
			std::wstring	wstr;

			//Log("%ws", (PWSTR)message);

			try {
				if( reader->parse(str.c_str(), str.c_str() + str.length(), &req, &errors) ) {

					if( m_callback.webMessage.pCallback ) {
						if( m_callback.webMessage.pCallback(m_callback.webMessage.pContext, req, res) ) {
							if( Json2Utf16(res, wstr)) {
								Log("%ws", wstr.c_str());
								if( SUCCEEDED(hr = webview->PostWebMessageAsString(wstr.c_str()))) {

								}
								else {
									Log("PostWebMessageAsString() failed.");
								}
							}						
						}					
					}
					else {
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

							for( auto i = 0 ; i < 1000 ; i++ ) {
								row.clear();
								row["key"]	= i;

								row["a"]	= std::to_string(i);
								row["b"]	= std::to_string(i);
								row["c"]	= std::to_string(i);
								res["data"].append(row);
							}
						}
						if( Json2Utf16(res, wstr)) {
							Log("%ws", wstr.c_str());
							if( SUCCEEDED(hr = webview->PostWebMessageAsString(wstr.c_str()))) {

							}
							else {
								Log("PostWebMessageAsString() failed.");
							}
						}
					}
				}
				else {
					Log("%-32s json parsing failed.", __func__);
				}
			}
			catch( std::exception & e ) {
				Log("%-32s %s", __func__, e.what());
			}
		}
	}
};
