﻿#pragma once

#include "yagent.define.h"
#include "IFilterCtrl.h"
#include "CDriverService.h"
#include <fltUser.h>
#include <atomic>

#pragma comment(lib, "FltLib.lib")

#define	CFILTER_CTRLIMPL_LOGGER		"CFilterCtrlImpl"
#define CFILTER_CTRLIMPL_LOGNAME	"CFilterCtrlImpl.log"

#pragma pack(push, 1)

typedef struct Y_MESSAGE
	:
	public	FILTER_MESSAGE_HEADER,
	public	Y_HEADER
{
	char					szData[MAX_FILTER_MESSAGE_SIZE];
	#pragma pack(pop)
	OVERLAPPED				ov;
	#pragma pack(push, 1)
} Y_MESSAGE, *PY_MESSAGE;

//#define MESSAGE_GET_SIZE	(sizeof(FILTER_MESSAGE_HEADER) + sizeof(YFILTER_HEADER) + sizeof(YFILTER_DATA))
#define Y_MESSAGE_GET_SIZE	(sizeof(Y_MESSAGE))

typedef struct YFILTERCTRL_REPLY
{
	FILTER_REPLY_HEADER		_header;
	Y_REPLY					data;
} YFILTERCTRL_REPLY, * PYFILTERCTRL_REPLY;

typedef struct Y_MESSAGE_REPLY
	:
	public	FILTER_REPLY_HEADER,
	public	Y_REPLY
{

} Y_MESSAGE_REPLY, * PY_MESSAGE_REPLY;

#pragma pack(pop)


class  CFilterCtrlImpl;
typedef struct {
	int					nIdx;
	Y_MESSAGE			ymsg;
	PVOID				pContext;
} THREAD_MESSAGE, *PTHREAD_MESSAGE;

typedef struct {
	WCHAR				szName[100];
	HANDLE				hCompletion;
	HANDLE				hInit;
	HANDLE				hShutdown;
	HANDLE				hPort;
	DWORD				dwThreadCount;
	HANDLE *			hThreads;
	//YFILTERCTRL_MESSAGE	message;
	PTHREAD_MESSAGE		pMessage;

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
		:	
		CAppLog(L"CFilterCtrlImpl.log")
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
		Log(__func__);
		__try
		{
			if( DriverIsInstalled() )
				bRet	= true;
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

		Log("%-32s pDriverPath:%ws", __func__, pDriverPath);
		Log("%-32s pDriverName:%ws", __func__, pDriverName);
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
		Log("%-32s pEventCallback=%p, pEventCallbackPtr=%p", 
			__FUNCTION__, pEventCallbackPtr, pEventCallback);
		m_callback.command.pCallback		= pCommandCallback;
		m_callback.command.pCallback2		= NULL;
		m_callback.command.pContext			= pCommandCallbackPtr;
		m_callback.event.pCallback			= pEventCallback;
		m_callback.event.pCallback2			= NULL;
		m_callback.event.pContext			= pEventCallbackPtr;
		if (DriverIsRunning())	{
			return true;
		}
		Log("%-32s driver is not running.", __func__);
		return DriverStart();
	}
	bool	Start2(
		IN PVOID pCommandContext, 
		IN CommandCallbackProc2		pCommandCallback,
		IN PVOID pEventContext, 
		IN EventCallbackProc2 pEventCallback
	)
	{
		m_callback.command.pCallback		= NULL;
		m_callback.command.pCallback2		= pCommandCallback;
		m_callback.command.pContext			= pCommandContext;

		m_callback.event.pCallback			= NULL;
		m_callback.event.pCallback2			= pEventCallback;
		m_callback.event.pContext			= pEventContext;
		if (DriverIsRunning())	{
			return true;
		}
		Log("%-32s driver is not running.", __func__);
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
	bool	CreatePortInfo(
			PPORT_INFO p, 
			PCWSTR pPortName, 
			DWORD dwThreadCount,
			_beginthreadex_proc_type proc
	)
	{
		bool	bRet	= false;
		HRESULT	hResult	= S_FALSE;

		Log("%-32s name:%ws", __func__, pPortName);
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
			Log("%-32s comp:%p", __func__, p->hCompletion);
			p->hThreads			= (HANDLE *)malloc(sizeof(HANDLE) * dwThreadCount);
			if (NULL == p->hThreads) {
				Log("%s error: alloca() failed. code=%d", __FUNCTION__, GetLastError());
				__leave;
			}
			p->pMessage			= (PTHREAD_MESSAGE)malloc(sizeof(THREAD_MESSAGE) * dwThreadCount);
			if( NULL == p->pMessage ) {
				Log("%s error: alloca() failed. code=%d", __FUNCTION__, GetLastError());
				__leave;			
			}
			ZeroMemory(p->pMessage, sizeof(THREAD_MESSAGE) * p->dwThreadCount);
			for (DWORD i = 0; i < p->dwThreadCount; i++)
			{
				p->pMessage[i].nIdx		= i;
				p->pMessage[i].pContext	= p;
				p->hThreads[i]	= (HANDLE)_beginthreadex(NULL, 0, proc, &p->pMessage[i], 0, NULL);
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
			if( false == bRet ) {
				if( p && p->hThreads )	free(p->hThreads);
				if( p && p->pMessage )	free(p->pMessage);
			}
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
		Log("%-32s %08x", __FUNCTION__, dwCommand);
		Y_COMMAND	command;
		Y_REPLY		reply;
		DWORD		dwBytes;

		command.dwCommand = dwCommand;
		if (S_OK == FilterSendMessage(m_driver.command.hPort, &command, sizeof(command), 
					&reply, sizeof(reply), &dwBytes)) {
			Log("%-32s reply.bRet=%d, dwBytes=%d", __FUNCTION__, reply.bRet, dwBytes);
			return true;
		}
		else {
			Log("%-32s FilterSendMessage() failed.", __FUNCTION__);
		}
		return true;
	}
	bool	SendCommand2(PY_COMMAND pCommand)
	{
		Log("%-32s", __FUNCTION__);
		DWORD		dwBytes;

		if (S_OK == FilterSendMessage(m_driver.command.hPort, pCommand, pCommand->nSize,
			NULL, 0, &dwBytes)) {
			return true;
		}
		else {
			Log("%-32s FilterSendMessage() failed.", __FUNCTION__);
		}
		return true;
	}
	bool	Connect()
	{
		Log(__FUNCTION__);
		SYSTEM_INFO		si;
		GetSystemInfo(&si);
		DWORD	dwCPU	= min(si.dwNumberOfProcessors, 2);		
		Log("-- CPU:%d", dwCPU);
		__try
		{
			if (CreatePortInfo(&m_driver.command, DRIVER_COMMAND_PORT, 1, CommandThread))
			{

			}
			else
			{
				__leave;
			}
			if (CreatePortInfo(&m_driver.event, DRIVER_EVENT_PORT, 1, EventThread2))
			{

			}
			else
			{
				__leave;
			}
			//	
			m_driver.bConnected = SendCommand(Y_COMMAND_START);
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
			SendCommand(Y_COMMAND_STOP);
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
			PVOID					pContext;
			CommandCallbackProc		pCallback;
			CommandCallbackProc2	pCallback2;
		} command;
		struct {
			PVOID					pContext;
			EventCallbackProc		pCallback;
			EventCallbackProc2		pCallback2;
		} event;
	} m_callback;
	static	unsigned	__stdcall	CommandThread(LPVOID ptr)
	{
		PTHREAD_MESSAGE		p		= (PTHREAD_MESSAGE)ptr;
		PPORT_INFO			pp		= (PPORT_INFO)p->pContext;
		CFilterCtrlImpl		*pClass	= (CFilterCtrlImpl *)pp->pClass;

		pClass->Log("%s begin", __FUNCTION__);
		//WaitForSingleObject(p->hInit, 30 * 1000);
		bool			bLoop = false;
		while (bLoop)
		{
			bool			bRet;
			DWORD			dwBytes;
			LPOVERLAPPED	pOverlapped	= NULL;
			ULONG_PTR		pCompletionKey;

			__try 
			{
				bRet = GetQueuedCompletionStatus(pp->hCompletion, &dwBytes, &pCompletionKey, &pOverlapped, INFINITE);
				pClass->Log("%s bRet=%d dwBytes=%d, pCompletionKey=%p, pOverlapped=%p",
					__FUNCTION__, bRet, dwBytes, pCompletionKey, pOverlapped);
				if (false == bRet || NULL == pCompletionKey)	break;
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				pClass->Log("%s exception", __FUNCTION__);
				break;
			}
		}
		pClass->Log("%s end", __FUNCTION__);
		return 0;
	}
	static	unsigned	__stdcall	EventThread2(LPVOID ptr)
	{
		PTHREAD_MESSAGE		p		= (PTHREAD_MESSAGE)ptr;
		PPORT_INFO			pp		= (PPORT_INFO)p->pContext;
		CFilterCtrlImpl		*pClass	= (CFilterCtrlImpl *)pp->pClass;

		//WaitForSingleObject(p->hInit, 30 * 1000);
		pClass->Log("%s(%d) begin", __FUNCTION__, p->nIdx);

		PY_MESSAGE			pMessage	= &p->ymsg;
		Y_MESSAGE_REPLY		reply;

		HRESULT				hr;
		DWORD				dwMessageSize;

		LPOVERLAPPED		pOv		= NULL;
		bool				bRet	= true;
		DWORD				dwBytes	= 0;
		ULONG_PTR			pCompletionKey;
		DWORD				dwError;

		dwMessageSize	= FIELD_OFFSET(Y_MESSAGE, ov);
		while (true)
		{
			//pClass->Log("%-32s port:%p data:%p size:%d", __func__, pp->hPort, pMessage, dwMessageSize);
			hr = FilterGetMessage(
				pp->hPort,
				pMessage,
				dwMessageSize,
				&pMessage->ov);
			if (HRESULT_FROM_WIN32(ERROR_IO_PENDING) != hr)
			{
				//	[TODO] 실패인데.. 이를 어쩌나. 
				pClass->Log("%s FilterGetMessage()=%08x", __FUNCTION__, hr);
				break;
			}	

			//pClass->Log("%-32s comp:%p", __func__, pp->hCompletion);
			bRet		= GetQueuedCompletionStatus(pp->hCompletion, &dwBytes, &pCompletionKey, 
				(LPOVERLAPPED *)&pOv, INFINITE);
			dwError	= GetLastError();
			//pClass->Log("%-32s bRet:%d dwBytes:%d, pCompletionKey:%p, pOv:%p, error:%d",
			//	__func__, bRet, dwBytes, pCompletionKey, pOv, dwError);

			if( (false == bRet && ERROR_INSUFFICIENT_BUFFER != dwError) || 
				0		== dwBytes	||
				NULL	== pCompletionKey )	{
				pClass->Log("%s bRet=%d dwBytes=%d, pCompletionKey=%p, pMessage=%p, error code=%d",
					__FUNCTION__, bRet, dwBytes, pCompletionKey, pMessage, GetLastError());
				break;
			}
			pMessage	= CONTAINING_RECORD(pOv, Y_MESSAGE, ov);
			reply.bRet	= false;

			//PY_HEADER	ph	= dynamic_cast<PY_HEADER>(pMessage);
			//pClass->Log("message total:%d message:%d mode:%d category:%d data:%d string:%d", 
			//	dwBytes, dwBytes - sizeof(FILTER_MESSAGE_HEADER), 
			//	ph->mode, ph->category, ph->wDataSize, ph->wStringSize);
			/*
			if( YFilter::Message::Category::Module == ph->category ) {
				PY_MODULE_MESSAGE	pm	= (PY_MODULE_MESSAGE)ph;
				SetStringOffset(pm, &pm->DevicePath);
				pClass->Log("%ws", pm->DevicePath.pBuf);
				pClass->Log("T:%d H:%d M:%d=%d+%d D:%d S:%d", 
					dwBytes, sizeof(FILTER_MESSAGE_HEADER), 
					sizeof(Y_MODULE_MESSAGE),
					sizeof(Y_MODULE_DATA), sizeof(Y_MODULE_STRING),
					pm->wDataSize, pm->wStringSize);
			}
			else if( YFilter::Message::Category::Process == ph->category ) {
				PY_PROCESS_MESSAGE	pr	= (PY_PROCESS_MESSAGE)ph;
				SetStringOffset(pr, &pr->DevicePath);
				pClass->Log("%d", sizeof(Y_PROCESS_DATA));
				pClass->Log("T:%d H:%d M:%d=%d+%d D:%d S:%d", 
					dwBytes, sizeof(FILTER_MESSAGE_HEADER), 
					sizeof(Y_PROCESS_MESSAGE),
					sizeof(Y_PROCESS_DATA), sizeof(Y_PROCESS_STRING),
					pr->wDataSize, pr->wStringSize);
				pClass->Log("%d %d", pr->DevicePath.wOffset, pr->DevicePath.wSize);

			}
			*/
			if (YFilter::Message::Mode::Event == pMessage->mode)
			{
				if( pMessage->wRevision ) {
					if (pClass->m_callback.event.pCallback2)
					{
						reply.bRet = 
							pClass->m_callback.event.pCallback2(
								dynamic_cast<PY_HEADER>(pMessage),
								pClass->m_callback.event.pContext								
							);
					}
				}
				else {
					pClass->Log("revision    :%d", pMessage->wRevision);
					pClass->Log("data size   :%d/%d", pMessage->wDataSize, dwMessageSize);
					pClass->Log("string size :%d/%d", pMessage->wStringSize, dwMessageSize);
				}
			}	
			else {
				pClass->Log("-- mode :%d", pMessage->mode);
				pClass->Log("category:%d", pMessage->category);
				pClass->Log("data size  :%d", pMessage->wDataSize);
				pClass->Log("string size:%d", pMessage->wDataSize);
				pClass->Log("revision:%d", pMessage->wRevision);
			}
		}
		pClass->Log("%s(%d) end", __FUNCTION__, p->nIdx);
		return 0;
	}
	#define USE_SYNC_MESSAGE
	#ifdef __EVENT_THREAD__
	static	unsigned	__stdcall	EventThread(LPVOID ptr)
	{
		if (NULL == ptr)	{
			
			return (unsigned)-1;
		}

		PTHREAD_MESSAGE	p	= (PTHREAD_MESSAGE)ptr;

		//WaitForSingleObject(p->hInit, 30 * 1000);
		p->pClass->Log("%s begin", __FUNCTION__);

		LPOVERLAPPED		pOv		= NULL;
		bool				bRet	= true;
		DWORD				dwBytes	= 0;
		ULONG_PTR			pCompletionKey;
		DWORD				dwError;

		PY_MESSAGE			pMessage	= NULL;
		Y_MESSAGE_REPLY		reply;

		HRESULT				hr;
		DWORD				dwMessageSize;

		pMessage	= p->message

		while (true)
		{
			if( bSync ) {
				pMessage	= &msg;
				ZeroMemory(pMessage, sizeof(Y_MESSAGE));
				dwMessageSize	= FIELD_OFFSET(Y_MESSAGE, ov);

				p->pClass->Log("%-32s port:%p", __func__, p->hPort);
				hr = FilterGetMessage(
					p->hPort,
					pMessage,
					dwMessageSize,
					NULL);
				if( S_OK == hr ) {

					p->pClass->Log("%-32s hr:%08x HRESULT_FROM_WIN32(ERROR_IO_PENDING)=%08x", __func__, hr, HRESULT_FROM_WIN32(ERROR_IO_PENDING));
				}
				else {
				
					p->pClass->Log("%-32s hr:%08x", __func__, hr);
					break;
				}
				pMessage	= NULL;
			}
			else {
				pMessage	= (PY_MESSAGE)CMemoryOperator::Allocate(sizeof(Y_MESSAGE));
				if( NULL == pMessage )	break;

				ZeroMemory(pMessage, sizeof(Y_MESSAGE));
				dwMessageSize	= FIELD_OFFSET(Y_MESSAGE, ov);

				p->pClass->Log("%-32s port:%p", __func__, p->hPort);
				hr = FilterGetMessage(
					p->hPort,
					pMessage,
					dwMessageSize,
					&pMessage->ov);
				p->pClass->Log("%-32s hr:%08x", __func__, hr);

				if (HRESULT_FROM_WIN32(ERROR_IO_PENDING) != hr)
				{
					//	[TODO] 실패인데.. 이를 어쩌나. 
					p->pClass->Log("%s FilterGetMessage()=%08x", __FUNCTION__, hr);
					break;
				}
				pMessage	= NULL;
				pOv			= NULL;
				p->pClass->Log("%-32s comp:%p", __func__, p->hCompletion);
				bRet		= GetQueuedCompletionStatus(p->hCompletion, &dwBytes, &pCompletionKey, 
									(LPOVERLAPPED *)&pOv, INFINITE);
				dwError	= GetLastError();
				p->pClass->Log("%-32s bRet:%d dwBytes:%d, pCompletionKey:%p, pOv:%p, error:%d",
					__func__, bRet, dwBytes, pCompletionKey, pOv, dwError);



				if( (false == bRet && ERROR_INSUFFICIENT_BUFFER != dwError) || 
					0		== dwBytes	||
					NULL	== pCompletionKey )	{
					p->pClass->Log("%s bRet=%d dwBytes=%d, pCompletionKey=%p, pMessage=%p, error code=%d",
						__FUNCTION__, bRet, dwBytes, pCompletionKey, pMessage, GetLastError());
					break;
				}
				pMessage	= CONTAINING_RECORD(pOv, Y_MESSAGE, ov);
			}

		
			reply.bRet	= false;
			if( pMessage ) {
				if (YFilter::Message::Mode::Event == pMessage->mode)
				{
					if( pMessage->wRevision ) {
						if (p->pClass->m_callback.event.pCallback2)
						{
							reply.bRet = 
								p->pClass->m_callback.event.pCallback2(
									pMessage,
									p->pClass->m_callback.event.pContext								
								);
						}
					
					}
					else {
						p->pClass->Log("revision    :%d", pMessage->wRevision);
						p->pClass->Log("message size:%d", dwMessageSize);
						p->pClass->Log("iocp bytes  :%d", dwBytes);
				
					}
				}	
				else {
					p->pClass->Log("-- mode :%d", pMessage->mode);
					p->pClass->Log("category:%d", pMessage->category);
					p->pClass->Log("size    :%d", pMessage->wSize);
					p->pClass->Log("revision:%d", pMessage->wRevision);

				}
				if( false == bSync )
					CMemoryOperator::Free(pMessage);
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
			reply.Status = ERROR_SUCCESS;
			reply.MessageId = pMessage->MessageId;
			/*
			hr	= FilterReplyMessage(p->hPort, (PFILTER_REPLY_HEADER)&reply, sizeof(reply));
			if (SUCCEEDED(hr) || ERROR_IO_PENDING == hr || ERROR_FLT_NO_WAITER_FOR_REPLY == hr)
			{
				p->pClass->Log("FilterReplyMessage(SUCCESS)=%08x", hr);
			}	
			else
			{
				//	E_INVALIDARG
				//	https://yoshiki.tistory.com/entry/HRESULT-%EC%97%90-%EB%8C%80%ED%95%9C-%EB%82%B4%EC%9A%A9
				p->pClass->Log("FilterReplyMessage(FAILURE)=%08x, E_INVALIDARG=%08x", 
					hr, E_INVALIDARG);
			}
			*/
			ERROR_MORE_DATA;
		}
		p->pClass->Log("%s end", __FUNCTION__);
		return 0;
	}
	#endif
	
	#ifdef ____
	static	unsigned	__stdcall	EventThread_OLD(LPVOID ptr)
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

			p->pClass->Log("%-32s dwMessageSize=%d", __func__, dwMessageSize);
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
				if (pMessage->header.wSize == sizeof(YFILTER_HEADER) + sizeof(YFILTER_DATA))
				{
					PYFILTER_DATA	pData = &pMessage->data;
					if (pData && p->pClass->m_callback.event.pCallback)
					{
						reply.data.bRet = p->pClass->m_callback.event.pCallback(
							pMessage->_header.MessageId,
							p->pClass->m_callback.event.pContext,
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
					p->pClass->Log("size mismatch: %d != %d", pMessage->header.wSize, sizeof(YFILTERCTRL_MESSAGE));
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
	#endif
};
#include <fltWinError.h>