#pragma once

#include "yagent.define.h"
#include "IFilterCtrl.h"
#include "CDriverService.h"
#include <fltUser.h>
#include <atomic>

#pragma comment(lib, "FltLib.lib")

#pragma pack(push, 1)
typedef struct YFILTERCTRL_MESSAGE
{
	FILTER_MESSAGE_HEADER	_header;
	YFILTER_HEADER			header;
	YFILTER_DATA			data;
	OVERLAPPED				ov;
} YFILTERCTRL_MESSAGE, *PYFILTERCTRL_MESSAGE;
#define MESSAGE_GET_SIZE	(sizeof(FILTER_MESSAGE_HEADER) + sizeof(YFILTER_HEADER) + sizeof(YFILTER_DATA))
typedef struct YFILTERCTRL_REPLY
{
	FILTER_REPLY_HEADER		_header;
	YFILTER_REPLY			data;
} YFILTERCTRL_REPLY, * PYFILTERCTRL_REPLY;
#pragma pack(pop)

class CFilterCtrlImpl;
typedef struct {
	WCHAR				szName[100];
	HANDLE				hCompletion;
	HANDLE				hInit;
	HANDLE				hShutdown;
	HANDLE				hPort;
	DWORD				dwThreadCount;
	HANDLE *			hThreads;
	YFILTERCTRL_MESSAGE	message;
	CFilterCtrlImpl		*pClass;
} PORT_INFO, *PPORT_INFO;

class CFilterCtrlImpl	
	:	
	public	IFilterCtrl,
	public	CDriverService,
	virtual	public	CAppLog
{
public:
	CFilterCtrlImpl()
	{
		Log(__FUNCTION__);
		ZeroMemory(&m_driver, sizeof(m_driver));
		ZeroMemory(&m_callback, sizeof(m_callback));
	}
	~CFilterCtrlImpl()
	{
		Log(__FUNCTION__);
	}
	virtual	IFilterCtrl *	Create(IN LPVOID p1, IN LPVOID p2)
	{
		UNREFERENCED_PARAMETER(p1);
		UNREFERENCED_PARAMETER(p2);
		return dynamic_cast<IFilterCtrl *>(new CFilterCtrlImpl());
	}
	void	Destroy()
	{
		delete this;
	}
	bool	IsInstalled()
	{
		bool	bRet	= false;
		__try
		{

		}
		__finally
		{

		}
		return bRet;
	}
	bool	Install
	(
		STRSAFE_LPCWSTR		pDriverName,
		STRSAFE_LPCWSTR		pDriverPath,
		bool				bRunInSafeMode
	) 
	{
		UNREFERENCED_PARAMETER(pDriverPath);
		UNREFERENCED_PARAMETER(pDriverName);
		bool	bRet	= false;
		__try
		{
			SetDriver(pDriverName, pDriverPath);
			if (PathFileExists(Config()->szDriverInstallPath))
			{
				Log("DRIVER_INSTALL_PATH exists.");
			}
			else
			{
				if (CopyFile(Config()->szDriverPath, Config()->szDriverInstallPath, FALSE))
				{
					Log("%ws => %ws", Config()->szDriverPath, Config()->szDriverInstallPath);
				}
				else
				{
					Log("%ws => %ws", Config()->szDriverPath, Config()->szDriverInstallPath);
					Log("%s CopyFile() failed. code=%d", __FUNCTION__, GetLastError());
					__leave;
				}
			}
			if (DriverIsInstalled())
			{

			}
			else
			{
				if (DriverInstall(pDriverName, DRIVER_ALTITUDE, false, bRunInSafeMode))
				{

				}
				else
				{
					Log("error: can not install.");
					__leave;
				}
			}
			bRet	= true;
		}
		__finally
		{

		}
		return bRet;
	}
	void	Uninstall()
	{
		if( DriverIsRunning() )		DriverStop();
		if (DriverIsInstalled())	DriverUninstall();
	}
	bool	IsRunning()
	{
		bool	bRet = false;
		__try
		{
			bRet	= DriverIsRunning();
		}
		__finally
		{

		}
		return bRet;
	}
	bool	Start(
		IN PVOID pCommandCallbackPtr, IN CommandCallbackProc pCommandCallback,
		IN PVOID pEventCallbackPtr, IN EventCallbackProc pEventCallback
	)
	{
		Log("%s pEventCallback=%p, pEventCallbackPtr=%p", 
			__FUNCTION__, pEventCallbackPtr, pEventCallback);
		m_callback.command.pCallback		= pCommandCallback;
		m_callback.command.pCallbackPtr		= pCommandCallbackPtr;
		m_callback.event.pCallback			= pEventCallback;
		m_callback.event.pCallbackPtr		= pEventCallbackPtr;
		if (DriverIsRunning())	return true;
		return DriverStart();
	}
	void	Shutdown()
	{
		Log("%s begin", __FUNCTION__);
		if (IsConnected())
		{
			Disconnect();
		}
		if (DriverIsRunning())
		{
			DriverStop();
		}
		Log("%s end", __FUNCTION__);
	}

	bool	CreatePortInfo(PPORT_INFO p, PCWSTR pPortName, DWORD dwThreadCount,
		_beginthreadex_proc_type proc)
	{
		bool	bRet	= false;
		HRESULT	hResult	= S_FALSE;
		__try
		{
			if( NULL == p )	__leave;
			ZeroMemory(p, sizeof(PORT_INFO));
			StringCbCopy(p->szName, sizeof(p->szName), pPortName);
			hResult = FilterConnectCommunicationPort(
				pPortName, 0, this, sizeof(this), NULL, &p->hPort);
			if (FAILED(hResult) || INVALID_HANDLE_VALUE == p->hPort)
			{
				Log("%s error: FilterConnectCommunicationPort() failed. code=%d",
					__FUNCTION__, GetLastError());
				__leave;
			}
			p->pClass			= this;
			StringCbCopy(p->szName, sizeof(p->szName), pPortName);
			p->hInit			= CreateEvent(NULL, FALSE, FALSE, NULL);
			p->hShutdown		= CreateEvent(NULL, FALSE, FALSE, NULL);
			p->dwThreadCount	= dwThreadCount;
			p->hCompletion		= CreateIoCompletionPort(p->hPort, NULL, (ULONG_PTR)this, dwThreadCount);
			if (NULL == p->hCompletion) {
				Log("%s error: CreateIoCompletionPort() failed. code=%d", __FUNCTION__, GetLastError());
				__leave;
			}
			p->hThreads			= (HANDLE *)malloc(sizeof(HANDLE) * dwThreadCount);
			if (NULL == p->hThreads) {
				Log("%s error: alloca() failed. code=%d", __FUNCTION__, GetLastError());
				__leave;
			}
			ZeroMemory(p->hThreads, sizeof(HANDLE) * p->dwThreadCount);
			for (DWORD i = 0; i < p->dwThreadCount; i++)
			{
				p->hThreads[i]	= (HANDLE)_beginthreadex(NULL, 0, proc, p, 0, NULL);
				if (NULL == p->hThreads[i]) {
					Log("%s thread[%d] failed. code=%d", __FUNCTION__, i, GetLastError());
					break;
				}
			}
			SetEvent(p->hInit);
			bRet	= true;
		}
		__finally
		{

		}
		return bRet;
	}
	void	DestroyPortInfo(PPORT_INFO p)
	{
		if( NULL == p )	return;
		if (p->hThreads && p->dwThreadCount)
		{
			for (DWORD i = 0; i < p->dwThreadCount; i++)
			{
				if (FALSE == PostQueuedCompletionStatus(p->hCompletion, 0, NULL, NULL)) {
					Log("%s thread[%d] PostQueuedCompletionStatus() failed. code=%d", __FUNCTION__, i, GetLastError());
				}
				WaitForSingleObject(p->hThreads[i], 1000 * 3);
				WAIT_OBJECT_0;
				CloseHandle(p->hThreads[i]);
			}
			free(p->hThreads);
		}
		if (p->hPort && INVALID_HANDLE_VALUE != p->hPort)
		{
			CloseHandle(p->hPort);
		}
		SetEvent(p->hShutdown);
		if (p->hCompletion && INVALID_HANDLE_VALUE != p->hCompletion)
		{
			CloseHandle(p->hCompletion);
		}		
	
		if (p->hShutdown)	CloseHandle(p->hShutdown);
		if( p->hInit )		CloseHandle(p->hInit);
		ZeroMemory(p, sizeof(PORT_INFO));
	}
	bool	SendCommand(DWORD dwCommand)
	{
		Log("%s %08x", __FUNCTION__, dwCommand);
		YFILTER_COMMAND	command;
		YFILTER_REPLY		reply;
		DWORD			dwBytes;

		command.dwCommand = dwCommand;
		if (S_OK == FilterSendMessage(m_driver.command.hPort, &command, sizeof(command), 
					&reply, sizeof(reply), &dwBytes)) {
			Log("%s reply.bRet=%d, dwBytes=%d", __FUNCTION__, reply.bRet, dwBytes);
			return true;
		}
		else {
			Log("%s FilterSendMessage() failed.", __FUNCTION__);
		}
		return true;
	}
	bool	Connect()
	{
		Log(__FUNCTION__);
		__try
		{
			if (CreatePortInfo(&m_driver.command, DRIVER_COMMAND_PORT, 1, CommandThread))
			{

			}
			else
			{
				__leave;
			}
			if (CreatePortInfo(&m_driver.event, DRIVER_EVENT_PORT, 1, EventThread))
			{

			}
			else
			{
				__leave;
			}
			//	
			m_driver.bConnected = SendCommand(YFILTER_COMMAND_START);
		}
		__finally
		{
			if( false == m_driver.bConnected )
			{
				DestroyPortInfo(&m_driver.command);
				DestroyPortInfo(&m_driver.event);
			}
		}
		return m_driver.bConnected;
	}
	bool	IsConnected()
	{
		return m_driver.bConnected;
	}
	void	Disconnect()
	{
		bool	bConnected = m_driver.bConnected;
		Log("%s m_driver.bConnected=%d", __FUNCTION__, bConnected);
		if( m_driver.bConnected )
		{
			SendCommand(YFILTER_COMMAND_STOP);
			DestroyPortInfo(&m_driver.command);
			DestroyPortInfo(&m_driver.event);
			m_driver.bConnected	= false;
		}
	}
private:
	struct {
		PORT_INFO			event;
		PORT_INFO			command;
		std::atomic<bool>	bConnected;
	} m_driver;

	struct {
		struct {
			PVOID				pCallbackPtr;
			CommandCallbackProc	pCallback;
		} command;
		struct {
			PVOID				pCallbackPtr;
			EventCallbackProc	pCallback;
		} event;
	} m_callback;

	static	unsigned	__stdcall	CommandThread(LPVOID ptr)
	{
		if (NULL == ptr)	return (unsigned)-1;
		PPORT_INFO		p = (PPORT_INFO)ptr;

		p->pClass->Log("%s begin", __FUNCTION__);
		WaitForSingleObject(p->hInit, 30 * 1000);
		bool			bLoop = true;
		while (bLoop)
		{
			bool			bRet;
			DWORD			dwBytes;
			LPOVERLAPPED	pOverlapped	= NULL;
			ULONG_PTR		pCompletionKey;

			__try 
			{
				bRet = GetQueuedCompletionStatus(p->hCompletion, &dwBytes, &pCompletionKey, &pOverlapped, INFINITE);
				p->pClass->Log("%s bRet=%d dwBytes=%d, pCompletionKey=%p, pOverlapped=%p",
					__FUNCTION__, bRet, dwBytes, pCompletionKey, pOverlapped);
				if (false == bRet || NULL == pCompletionKey)	break;
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				p->pClass->Log("%s exception", __FUNCTION__);
				break;
			}
		}
		p->pClass->Log("%s end", __FUNCTION__);
		return 0;
	}
	static	unsigned	__stdcall	EventThread(LPVOID ptr)
	{
		if (NULL == ptr)	return (unsigned)-1;
		PPORT_INFO		p = (PPORT_INFO)ptr;

		WaitForSingleObject(p->hInit, 30 * 1000);
		p->pClass->Log("%s begin", __FUNCTION__);

		LPOVERLAPPED		pOv		= NULL;
		bool				bRet	= true;
		DWORD				dwBytes;
		ULONG_PTR			pCompletionKey;
		DWORD				dwError;

		PYFILTERCTRL_MESSAGE	pMessage	= &p->message;
		YFILTERCTRL_REPLY		reply;

		HRESULT	hr;
		DWORD	dwMessageSize;

		while (true)
		{
			ZeroMemory(&p->message.ov, sizeof(OVERLAPPED));
			dwMessageSize	= FIELD_OFFSET(YFILTERCTRL_MESSAGE, ov);
			hr = FilterGetMessage(p->hPort, 
				&p->message._header,
				dwMessageSize,
				&p->message.ov);
			if (HRESULT_FROM_WIN32(ERROR_IO_PENDING) != hr)
			{
				//	[TODO] 실패인데.. 이를 어쩌나. 
				p->pClass->Log("%s FilterGetMessage()=%08x", __FUNCTION__, hr);
				break;
			}
			bRet	= GetQueuedCompletionStatus(p->hCompletion, &dwBytes, &pCompletionKey, 
							(LPOVERLAPPED *)&pOv, INFINITE);
			dwError	= GetLastError();
			if( (false == bRet && ERROR_INSUFFICIENT_BUFFER != dwError) || 
				NULL == pCompletionKey )	{
				p->pClass->Log("%s bRet=%d dwBytes=%d, pCompletionKey=%p, pMessage=%p, error code=%d",
					__FUNCTION__, bRet, dwBytes, pCompletionKey, pMessage, GetLastError());
				break;
			}
			pMessage	= CONTAINING_RECORD(pOv, YFILTERCTRL_MESSAGE, ov);
			reply.data.bRet			= false;
			if (YFilter::Message::Mode::Event == pMessage->header.mode)
			{
				if (pMessage->header.size == sizeof(YFILTER_HEADER) + sizeof(YFILTER_DATA))
				{
					PYFILTER_DATA	pData = &pMessage->data;
					if (pData && p->pClass->m_callback.event.pCallback)
					{
						reply.data.bRet = p->pClass->m_callback.event.pCallback(
							pMessage->_header.MessageId,
							p->pClass->m_callback.event.pCallbackPtr,
							pMessage->header.category,
							pMessage->data.subType,
							pData,
							sizeof(YFILTER_DATA)
						);
					}
					else {
						printf("-- pData=%p, pCallback=%p\n", pData, p->pClass->m_callback.event.pCallback);
						reply.data.bRet = true;
					}
				}
				else
				{
					p->pClass->Log("size mismatch: %d != %d", pMessage->header.size, sizeof(YFILTERCTRL_MESSAGE));
				}
			}	
			else {
				printf("-- unknown mode:%d", pMessage->header.mode);
			}
			/*		
				FilterReplyMessage의 데이터 크기 부분 참고.
				https://docs.microsoft.com/en-us/windows/win32/api/fltuser/nf-fltuser-filterreplymessage
				Given this structure, 
				it might seem obvious that the caller of FilterReplyMessage would set the dwReplyBufferSize parameter 
				to sizeof(REPLY_STRUCT) and the ReplyLength parameter of FltSendMessage to the same value. 
				However, because of structure padding idiosyncrasies, 
				sizeof(REPLY_STRUCT) might be larger than sizeof(FILTER_REPLY_HEADER) + sizeof(MY_STRUCT). 
				If this is the case, FltSendMessage returns STATUS_BUFFER_OVERFLOW.

				Therefore, we recommend that you call FilterReplyMessage and FltSendMessage 
				(leveraging the above example) by setting dwReplyBufferSize and ReplyLength 
				both to sizeof(FILTER_REPLY_HEADER) + sizeof(MY_STRUCT) instead of sizeof(REPLY_STRUCT). 
				This ensures that any extra padding at the end of the REPLY_STRUCT structure is ignored.
			*/
			reply._header.Status = ERROR_SUCCESS;
			reply._header.MessageId = pMessage->_header.MessageId;
			hr	= FilterReplyMessage(p->hPort, (PFILTER_REPLY_HEADER)&reply,
					sizeof(reply));
			if (SUCCEEDED(hr) || ERROR_IO_PENDING == hr || ERROR_FLT_NO_WAITER_FOR_REPLY == hr)
			{
				//p->pClass->Log("FilterReplyMessage(SUCCESS)=%08x", hr);
			}	
			else
			{
				//	E_INVALIDARG
				//	https://yoshiki.tistory.com/entry/HRESULT-%EC%97%90-%EB%8C%80%ED%95%9C-%EB%82%B4%EC%9A%A9
				p->pClass->Log("FilterReplyMessage(FAILURE)=%08x, E_INVALIDARG=%08x", 
					hr, E_INVALIDARG);
			}
			ERROR_MORE_DATA;
		}
		p->pClass->Log("%s end", __FUNCTION__);
		return 0;
	}
};
#include <fltWinError.h>