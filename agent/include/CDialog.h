#pragma once
#include <map>
#include <thread>
typedef void	(*PMessageCallback)(IN HWND hWnd, IN UINT uMessage, WPARAM wParam, LPARAM lParam, IN PVOID ptr);
typedef void	(*PMessageThread)(IN HANDLE hShutdown, IN HWND hWnd, IN PVOID ptr);

typedef std::map<UINT, PMessageCallback>	MessageMap;
class CDialog
{
public:
	CDialog()
		:
		m_hWnd(NULL),
		m_pCallbackPtr(NULL),
		m_pKey(NULL),
		m_hThread(NULL)
	{

	}
	~CDialog()
	{

	}

	void		SetMessageCallbackPtr(IN PVOID pCallbackPtr)
	{
		m_pCallbackPtr	= pCallbackPtr;
	}
	void		AddMessageCallback(IN UINT nMsgId, IN PMessageCallback pCallback)
	{
		auto	t	= m_message.find(nMsgId);
		if( m_message.end() != t )
		m_message.erase(t);
		m_message[nMsgId]	= pCallback;
	}
	bool		Create(UINT nDialogId, PCWSTR pName)
	{
		m_hWnd = ::CreateDialogParam(NULL, MAKEINTRESOURCE(nDialogId), 0, DialogProc, (LPARAM)this);
		if (NULL == m_hWnd)
		{
			return false;
		}
		if( pName )
			SetWindowText(m_hWnd, pName);
		::ShowWindow(m_hWnd, SW_SHOW);
		return true;
	}
	void		MessagePump(IN PMessageThread pThread)
	{
		MSG		msg;
		HANDLE	hShutdown	= CreateEvent(NULL, FALSE, FALSE, NULL);

		std::thread	periodic(pThread, hShutdown, m_hWnd, m_pCallbackPtr);
		while (::GetMessage(&msg, 0, 0, 0))
		{
			if (!::IsDialogMessage(m_hWnd, &msg))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
		}
		SetEvent(hShutdown);
		periodic.join();
		CloseHandle(hShutdown);
	}
	void		Destroy()
	{
		if( m_hWnd )
		{
			DestroyWindow(m_hWnd);
			m_hWnd	= NULL;
		}
	}
	void		SetKey(IN PVOID pKey)
	{
		m_pKey	= pKey;
	}
	PVOID		GetKey()
	{
		return m_pKey;
	}
	void		ClearMessageCallback()
	{
		m_message.clear();
	}
private:
	HWND		m_hWnd;
	PVOID		m_pCallbackPtr;
	MessageMap	m_message;
	PVOID		m_pKey;
	HANDLE		m_hThread;

	static	INT_PTR CALLBACK DialogProc(
		_In_ HWND   hWnd,
		_In_ UINT   uMsg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam
	)
	{
		static	CDialog* pClass;

		switch (uMsg)
		{
		case WM_INITDIALOG:
			pClass = (CDialog*)lParam;
			{
				auto	t = pClass->m_message.find(WM_INITDIALOG);
				if (pClass->m_message.end() != t)
				{
					t->second(hWnd, uMsg, wParam, lParam, pClass->m_pCallbackPtr);
				}
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:

				default:
				{
					auto	t	= pClass->m_message.find(LOWORD(wParam));
					if (pClass->m_message.end() != t)
					{
						t->second(hWnd, uMsg, wParam, lParam, pClass->m_pCallbackPtr);
					}
				}
				break;
			}
			break;
		case WM_CLOSE:
			::DestroyWindow(hWnd);
			return TRUE;

		case WM_DESTROY:
			::PostQuitMessage(0);
			return TRUE;
		}
		return FALSE;
	}
};
