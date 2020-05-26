#pragma once

#include "yagent.define.h"
#include "IFilterCtrl.h"
#include "CDriverService.h"
#include <fltUser.h>
#include <atomic>

#pragma comment(lib, "FltLib.lib")

class CFilterCtrlImpl;
typedef struct {
	WCHAR		szName[100];
	HANDLE		hCompletion;
	HANDLE		hInit;
	HANDLE		hShutdown;
	HANDLE		hPort;
	DWORD		dwThreadCount;
	HANDLE *	hThreads;
	CFilterCtrlImpl	*pClass;
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
			Log("%s connected to %ws", __FUNCTION__, p->szName);
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
			LPOVERLAPPED	pOverlapped;
			ULONG_PTR		pCompletionKey;

			bRet = GetQueuedCompletionStatus(p->hCompletion, &dwBytes, &pCompletionKey, &pOverlapped, INFINITE);
			p->pClass->Log("%s %d dwBytes=%d", __FUNCTION__, bRet, dwBytes);
			if (false == bRet || 0 == dwBytes)	break;

		}
		p->pClass->Log("%s end", __FUNCTION__);
		return 0;
	}
	static	unsigned	__stdcall	EventThread(LPVOID ptr)
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
			LPOVERLAPPED	pOverlapped;
			ULONG_PTR		pCompletionKey;

			bRet	=  GetQueuedCompletionStatus(p->hCompletion, &dwBytes, &pCompletionKey, &pOverlapped, INFINITE);
			p->pClass->Log("%s %d dwBytes=%d", __FUNCTION__, bRet, dwBytes);
			if( false == bRet || 0 == dwBytes )	break;

			

		}
		p->pClass->Log("%s end", __FUNCTION__);
		return 0;
	}
};