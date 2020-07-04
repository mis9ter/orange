#pragma once

#include "yagent.define.h"
#include "IFilterCtrl.h"
#include "CDriverService.h"
#include <fltUser.h>
#include <atomic>

#pragma comment(lib, "FltLib.lib")

#define MESSAGE_DATA_SIZE	(MESSAGE_MAX_SIZE - sizeof(OVERLAPPED) - sizeof(FILTER_MESSAGE_HEADER) - sizeof(MESSAGE_HEADER))

#pragma pack(1)
typedef struct YFILTER_MESSAGE
{
	FILTER_MESSAGE_HEADER	_header;
	YFILTER_MESSAGE_HEADER	header;
	YFILTER_MESSAGE_PROCESS	data;
	OVERLAPPED				ov;
} YFILTER_MESSAGE, *PYFILTER_MESSAGE;
#define MESSAGE_GET_SIZE	(sizeof(FILTER_MESSAGE_HEADER) + sizeof(YFILTER_MESSAGE_HEADER) + sizeof(YFILTER_MESSAGE_PROCESS))

typedef struct YFILTER_REPLY
{
	FILTER_REPLY_HEADER		_header;
	YFILTER_REPLY_DATA		data;
} YFILTER_REPLY, * PYFILTER_REPLY;

#pragma pack()

class CFilterCtrlImpl;
typedef struct {
	WCHAR				szName[100];
	HANDLE				hCompletion;
	HANDLE				hInit;
	HANDLE				hShutdown;
	HANDLE				hPort;
	DWORD				dwThreadCount;
	HANDLE *			hThreads;
	YFILTER_MESSAGE		message;
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
			if (DriverIsRunning())
			{

			}
			else
			{
				if (DriverStart())
				{

				}
				else
				{
					Log("error: can not start.");
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
	bool	Start()
	{
		Log("%s", __FUNCTION__);
		if (DriverIsRunning())	return true;
		return DriverStart();
	}
	void	Shutdown()
	{
		Log("%s", __FUNCTION__);
		if (IsConnected())
		{
			Disconnect();
		}
		if (DriverIsRunning())
		{
			DriverStop();
		}
	}

	bool	CreatePortInfo(PPORT_INFO p, PCWSTR pPortName, DWORD dwThreadCount,
		_beginthreadex_proc_type proc)
	{
		bool	bRet	= false;
		HRESULT	hResult	= S_FALSE;

		Log("%s PPORT_INFO=%p, pPortName=%ws, dwThreadCount=%d", 
			__FUNCTION__, p, pPortName, dwThreadCount);
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
			Log("%s connected to %ws, %p", __FUNCTION__, p->szName, p->hPort);
			p->pClass			= this;
			p->hInit			= CreateEvent(NULL, FALSE, FALSE, NULL);
			p->hShutdown		= CreateEvent(NULL, FALSE, FALSE, NULL);
			p->dwThreadCount	= dwThreadCount;
			p->hCompletion		= CreateIoCompletionPort(p->hPort, NULL, (ULONG_PTR)this, dwThreadCount);
			if (NULL == p->hCompletion) {
				Log("%s error: CreateIoCompletionPort() failed. code=%d", __FUNCTION__, GetLastError());
				__leave;
			}
			p->hThreads			= (HANDLE *)alloca(sizeof(HANDLE));
			if (NULL == p->hThreads) {
				Log("%s error: alloca() failed. code=%d", __FUNCTION__, GetLastError());
				__leave;
			}
			ZeroMemory(p->hThreads, sizeof(HANDLE) * p->dwThreadCount);
			for (DWORD i = 0; i < p->dwThreadCount; i++)
			{
				p->hThreads[i]	= (HANDLE)_beginthreadex(NULL, 0, proc, p, 0, NULL);
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
		Log("%s PPORT_INFO=%p", __FUNCTION__, p);
		if( NULL == p )	return;
		PostQueuedCompletionStatus(p->hCompletion, 0, NULL, NULL);
		if (p->hThreads && p->dwThreadCount)
		{
			for (DWORD i = 0; i < p->dwThreadCount; i++)
			{
				WaitForSingleObject(p->hThreads[i], 1000 * 3);
				CloseHandle(p->hThreads[i]);
			}
		}
		if (p->hPort && INVALID_HANDLE_VALUE != p->hPort)
		{
			Log("%s disconnected from %ws", __FUNCTION__, p->szName);
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
	bool	Connect()
	{
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
			m_driver.bConnected	= true;
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
		if( m_driver.bConnected )
		{
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


	static	unsigned	__stdcall	CommandThread(LPVOID ptr)
	{
		if (NULL == ptr)	return (unsigned)-1;
		PPORT_INFO		p = (PPORT_INFO)ptr;

		WaitForSingleObject(p->hInit, 30 * 1000);
		bool			bLoop = true;
		p->pClass->Log("%s begin", __FUNCTION__);
		while (bLoop)
		{
			bool			bRet;
			DWORD			dwBytes;
			LPOVERLAPPED	pOverlapped	= NULL;
			ULONG_PTR		pCompletionKey;

			bRet = GetQueuedCompletionStatus(p->hCompletion, &dwBytes, &pCompletionKey, &pOverlapped, INFINITE);
			p->pClass->Log("%s bRet=%d dwBytes=%d, pCompletionKey=%p, pOverlapped=%p", 
				__FUNCTION__, bRet, dwBytes, pCompletionKey, pOverlapped);
			if (false == bRet || NULL == pCompletionKey)	break;

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

		PYFILTER_MESSAGE	pMessage	= &p->message;
		YFILTER_REPLY		reply;

		HRESULT	hr;

		while (true)
		{
			ZeroMemory(&p->message.ov, sizeof(OVERLAPPED));
			hr = FilterGetMessage(p->hPort, 
				&p->message._header,
				FIELD_OFFSET(YFILTER_MESSAGE, ov),
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
				p->pClass->Log("%s break condition", __FUNCTION__);
				p->pClass->Log("%s bRet=%d dwBytes=%d, pCompletionKey=%p, pMessage=%p, error code=%d",
					__FUNCTION__, bRet, dwBytes, pCompletionKey, pMessage, GetLastError());
				break;
			}
			pMessage	= CONTAINING_RECORD(pOv, YFILTER_MESSAGE, ov);
			//p->pClass->Log("HEADER: category=%d, type=%d, size=%d", 
			//	pMessage->header.category, pMessage->header.type, pMessage->header.size);
			reply.data.bRet			= false;
			if (YFilter::Message::Category::Event == pMessage->header.category)
			{
				switch (pMessage->header.type)
				{
					case YFilter::Message::Type::ProcessStart:
						if (pMessage->header.size == sizeof(YFILTER_MESSAGE_HEADER) + sizeof(YFILTER_MESSAGE_PROCESS))
						{
							PYFILTER_MESSAGE_PROCESS	pData	= &pMessage->data;
							p->pClass->Log("PROCESS_START:%d", pData->dwProcessId);
							p->pClass->Log("MessageId    :%I64d", pMessage->_header.MessageId);
							p->pClass->Log("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
								pData->uuid.Data1, pData->uuid.Data2, pData->uuid.Data3,
								pData->uuid.Data4[0], pData->uuid.Data4[1], pData->uuid.Data4[2], pData->uuid.Data4[3],
								pData->uuid.Data4[4], pData->uuid.Data4[5], pData->uuid.Data4[6], pData->uuid.Data4[7]);

							p->pClass->Log("%ws", pData->szPath);
							p->pClass->Log("%ws", pData->szCommand);
							p->pClass->Log("%s", TEXTLINE);
							reply.data.bRet = true;
						}
						else
						{
							p->pClass->Log("size mismatch: %d != %d", pMessage->header.size, sizeof(YFILTER_MESSAGE_PROCESS));
						}
						break;
					case YFilter::Message::Type::ProcessStop:
						if (pMessage->header.size == sizeof(YFILTER_MESSAGE_HEADER) + sizeof(YFILTER_MESSAGE_PROCESS))
						{
							PYFILTER_MESSAGE_PROCESS	pData = &pMessage->data;
							if ( pData->dwProcessId)
							{
								//	드라이버에서 생성을 감지했던 프로세스
								
							}
							else
							{
								//	드라이버 로드전에 이미 생성되었던 프로세스
							}
							p->pClass->Log("PROCESS_STOP :%d", pData->dwProcessId);
							p->pClass->Log("MessageId    :%I64d", pMessage->_header.MessageId);
							p->pClass->Log("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
								pData->uuid.Data1, pData->uuid.Data2, pData->uuid.Data3,
								pData->uuid.Data4[0], pData->uuid.Data4[1], pData->uuid.Data4[2], pData->uuid.Data4[3],
								pData->uuid.Data4[4], pData->uuid.Data4[5], pData->uuid.Data4[6], pData->uuid.Data4[7]);
							p->pClass->Log("%ws", pData->szPath);
							p->pClass->Log("%ws", pData->szCommand);
							p->pClass->Log("%s", TEXTLINE);
							reply.data.bRet = true;
						}
						else
						{
							p->pClass->Log("size mismatch: %d != %d", pMessage->header.size, 
								sizeof(YFILTER_MESSAGE_HEADER) + sizeof(YFILTER_MESSAGE_PROCESS));
						}
						break;
				}
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
				p->pClass->Log("reply hPort=%p, Status=%d, MessageId=%I64d, size: %d %d",
					p->hPort, reply._header.Status, reply._header.MessageId,
					sizeof(FILTER_REPLY_HEADER) + sizeof(YFILTER_REPLY_DATA),
					sizeof(YFILTER_REPLY));
			}
			ERROR_MORE_DATA;
		}
		p->pClass->Log("%s end", __FUNCTION__);
		return 0;
	}
};
#include <fltWinError.h>