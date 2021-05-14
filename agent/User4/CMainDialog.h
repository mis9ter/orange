#pragma once

#include <combaseapi.h>
#include <strsafe.h>

#include "yagent.h"
#include "CAppLog.h"
#include "CSettings.h"
#include "CControl.h"

class CMainDialog
	:
	virtual	public	CAppLog,
	virtual	public	CSettings,
	virtual public	CControl

{
public:
	CMainDialog() 
		:
		CAppLog(L"user.log"),
		m_szClassName(L""),
		m_szTitle(L""),
		m_hInstance(NULL),
		m_hWnd(NULL),
		m_hFont(NULL)
	
	{
		Log(__FUNCTION__);
		
	}
	~CMainDialog() {
		Log(__FUNCTION__);
	}


	bool		Create(
		HINSTANCE	hInstance, 
		int			nCmdShow,
		PCWSTR		pTitle,
		PCWSTR		pClassName
	) 
	{
		InitCommonControls();
		/*
		https://docs.microsoft.com/en-us/windows/win32/dataxchg/about-atom-tables
		으흠. 아톰 이란건 또 처음 알았다. 이건 UI할 때 쓰는 것인가?
		내용을 보면 UI가 아니라도 충분히 사용할 수 있겠는데?

		About Atom Tables
		An atom table is a system-defined table that stores strings and corresponding identifiers.
		An application places a string in an atom table and receives a 16-bit integer, 
		called an atom, that can be used to access the string. 

		A string that has been placed in an atom table is called an atom name.

		The system provides a number of atom tables. 
		Each atom table serves a different purpose. 
		For example, Dynamic Data Exchange (DDE) applications use the global atom table 
		to share item-name and topic-name strings with other applications. 
		Rather than passing actual strings, a DDE application passes global atoms 
		to its partner application. 
		The partner uses the atoms to obtain the strings from the atom table.

		Applications can use local atom tables to store their own item-name associations.

		The system uses atom tables that are not directly accessible to applications. 
		However, the application uses these atoms when calling a variety of functions. 
		For example, registered clipboard formats are stored in an internal atom table 
		used by the system. An application adds atoms to this atom table using 
		the RegisterClipboardFormat function. 

		Also, registered classes are stored in an internal atom table used by the system. 
		An application adds atoms to this atom table using 
		the RegisterClass or RegisterClassEx function.
		*/

		Setting::Rect	rect;

		rect.position	= GetIntegerPair(L"MainWindowPosition");
		rect.size		= GetScalableIntegerPair(L"MainWindowSize").pair;
		USHORT  nWindowAtom	= RegisterWindowClass(hInstance, pClassName);
		m_hWnd	= CreateWindow(pClassName, pTitle, WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
			rect.left, rect.top, rect.width, rect.height, 
			nullptr, nullptr, hInstance, this);
		if ( NULL == m_hWnd)	{
			
			return false;
		}

		m_hInstance		= hInstance;	
		m_nCmdShow		= nCmdShow;
		StringCbCopy(m_szTitle, sizeof(m_szTitle), pTitle);
		StringCbCopy(m_szClassName, sizeof(m_szClassName), pClassName);

		AdjustRectToWorkingArea(m_hWnd, &rect);
		MoveWindow(m_hWnd, rect.left, rect.top, rect.width, rect.height, FALSE);

		ShowWindow(m_hWnd, nCmdShow);
		UpdateWindow(m_hWnd);

		InitControls(hInstance, m_hWnd);

		//PhInitializeWindowTheme(PhMainWndHandle, PhEnableThemeSupport); // HACK

		return true;
	}
	void		Destroy() {
		PostQuitMessage(0);	
	}
	LONG		MessageLoop()
	{
		if( NULL == m_hWnd )	{
			Log("%-32s m_hWnd == NULL", __FUNCTION__);
			return 0;
		}
		HACCEL hAccelTable = LoadAccelerators(m_hInstance, MAKEINTRESOURCE(IDC_USER4));

		MSG     msg;

		// 기본 메시지 루프입니다:
		while (GetMessage(&msg, nullptr, 0, 0))
		{
			bool    bProcessed = false;
#ifdef ORANGE
			if (FilterList)
			{
				for (i = 0; i < FilterList->Count; i++)
				{
					PPH_MESSAGE_LOOP_FILTER_ENTRY entry = FilterList->Items[i];

					if (entry->Filter(&message, entry->Context))
					{
						processed = TRUE;
						break;
					}
				}
			}

			if (!processed)
			{
				if (
					message.hwnd == PhMainWndHandle ||
					IsChild(PhMainWndHandle, message.hwnd)
					)
				{
					if (TranslateAccelerator(PhMainWndHandle, acceleratorTable, &message))
						processed = TRUE;
				}

				if (DialogList)
				{
					for (i = 0; i < DialogList->Count; i++)
					{
						if (IsDialogMessage((HWND)DialogList->Items[i], &message))
						{
							processed = TRUE;
							break;
						}
					}
				}
			}
#endif
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		return (int) msg.wParam;
	}

private:
	HWND			m_hWnd;
	HINSTANCE		m_hInstance;
	int				m_nCmdShow;
	WCHAR			m_szTitle[100];
	WCHAR			m_szClassName[100];
	HFONT			m_hFont;

	ATOM RegisterWindowClass(HINSTANCE hInstance, PCWSTR pClassName)
	{
		WNDCLASSEXW wcex;
		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style          = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc    = WndProc;
		wcex.cbClsExtra     = 0;
		wcex.cbWndExtra     = 0;
		wcex.hInstance      = hInstance;
		wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_USER4));
		wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
		wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_USER4);
		wcex.lpszClassName  = pClassName;
		wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

		return RegisterClassExW(&wcex);
	}
	static	LRESULT CALLBACK WndProc(
		_In_ HWND hWnd,
		_In_ UINT uMsg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam
	)
	{
		static	CMainDialog		*pClass;
		switch (uMsg)
		{
			case WM_CREATE:
				pClass	= (CMainDialog *)((CREATESTRUCT *)lParam)->lpCreateParams;
				break;
			case WM_COMMAND:
				{
					int		wmID	= LOWORD(wParam);
					switch( wmID ) {
						case IDM_ABOUT:

							break;
						case IDM_EXIT:
							if( pClass )	pClass->Destroy();
							else			DestroyWindow(hWnd);
							break;

						default:
							return DefWindowProc(hWnd, uMsg, wParam, lParam);
					}
				}
				break;

			case WM_DESTROY:
				if( pClass )	pClass->Destroy();
				else			DestroyWindow(hWnd);
				break;

			default:
				return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
		return 0;
	}
	void    OnDestroy(HWND hWnd)
	{
		DestroyControls();
		PostQuitMessage(0);
	}
	void	DestroyControls()
	{
	
	}
	HFONT	CreateFont(
			_In_ PWSTR Name,
			_In_ ULONG Size,
			_In_ ULONG Weight
	)
	{
		ULONG	PhGlobalDpi	= 96;
		return ::CreateFont(
			-(LONG)MultiplyDivide(Size, PhGlobalDpi, 72),
			0,
			0,
			0,
			Weight,
			FALSE,
			FALSE,
			FALSE,
			ANSI_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH,
			Name
		);
	}
	void	InitializeFont()
	{
	
		NONCLIENTMETRICS metrics = { sizeof(metrics) };

		if (m_hFont)
			DeleteObject(m_hFont);

		if (
			!(m_hFont = CreateFont((PWSTR)L"Microsoft Sans Serif", 8, FW_NORMAL)) &&
			!(m_hFont = CreateFont((PWSTR)L"Tahoma", 8, FW_NORMAL))
			)
		{
			if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0))
				m_hFont = CreateFontIndirect(&metrics.lfMessageFont);
			else
				m_hFont = NULL;
		}
	}
};
