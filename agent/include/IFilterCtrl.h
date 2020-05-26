#pragma once
#include "yagent.define.h"
#include "IModule.h"
#include "CAppLog.h"

__interface IFilterCtrl
	:
	public	IModule
{
	virtual	bool	IsInstalled()	= NULL;
	virtual	bool	Install
	(
		STRSAFE_LPCWSTR		pDriverName,
		STRSAFE_LPCWSTR		pDriverPath,
		bool				bRunInSafeMode
	)		= NULL;
	virtual	void	Uninstall()		= NULL;
	virtual	bool	IsRunning()		= NULL;
	virtual	bool	Start()			= NULL;
	virtual	void	Shutdown()		= NULL;
	virtual	bool	Connect()		= NULL;
	virtual bool	IsConnected()	= NULL;
	virtual void	Disconnect()	= NULL;
};
IFilterCtrl *	__stdcall CreateInstance_CFilterCtrlImpl(IN LPVOID p1, LPVOID p2);

class CFilterCtrl
	:
	virtual	public	CAppLog,
	public			IFilterCtrl
{
public:
	CFilterCtrl(IN LPVOID p1, IN LPVOID p2)
		:
		m_pInstance(NULL)
	{
		Log(__FUNCTION__);
		m_pInstance	= (IFilterCtrl *)CModuleFactory::CreateInstance((PCreateInstance)CreateInstance_CFilterCtrlImpl, p1, p2);
		if (NULL == m_pInstance)
		{
			Log("%s m_pInstance is NULL.", __FUNCTION__);
		}
	}
	~CFilterCtrl()
	{
		Destroy();
		Log(__FUNCTION__);
	}
	void	Destroy()
	{
		if (m_pInstance)
		{
			m_pInstance->Destroy();
			m_pInstance	= NULL;
		}
	}
	bool	IsInstalled()
	{
		if( NULL == m_pInstance )	return false;
		return m_pInstance->IsInstalled();
	}
	bool	Install
	(
		STRSAFE_LPCWSTR		pDriverName,
		STRSAFE_LPCWSTR		pDriverPath,
		bool				bRunInSafeMode
	)
	{
		if (NULL == m_pInstance)	return false;
		return m_pInstance->Install(pDriverName, pDriverPath, bRunInSafeMode);
	}
	void	Uninstall()
	{
		if (NULL == m_pInstance)	return;
		return m_pInstance->Uninstall();
	}
	bool	IsRunning()
	{
		if (NULL == m_pInstance)	return false;
		return m_pInstance->IsRunning();
	}
	bool	Start()
	{
		Log(__FUNCTION__);
		if (NULL == m_pInstance)	return false;
		return m_pInstance->Start();
	}
	void	Shutdown()
	{
		if (NULL == m_pInstance)	return;
		m_pInstance->Shutdown();
	}
	bool	IsConnected()
	{
		if (NULL == m_pInstance)	return false;
		return m_pInstance->IsConnected();
	}
	bool	Connect()
	{
		if (NULL == m_pInstance)	return false;
		return m_pInstance->Connect();
	}
	void	Disconnect()
	{
		if (NULL == m_pInstance)	return;
		m_pInstance->Disconnect();
	}
private:
	IFilterCtrl	*	m_pInstance;
};
