#pragma once
#include "yagent.h"

class CFilterCtrl
	:
	virtual public CAppLog,
	public	IFilterCtrl
{
public:
	CFilterCtrl
	(

	)
	{
		Log(NULL);
		Log(__FUNCTION__);
		ZeroMemory(&m_module, sizeof(m_module));
	}
	~CFilterCtrl()
	{
		Log(__FUNCTION__);
	}
	void	Initialize(
		IN	LPCWSTR				pDllPath,
		IN	PDllLoadCallback	pDllLoadCallback,
		IN	LPVOID				pCallbackPtr
	)
	{
		Log("%s %s", __FUNCTION__, __DATE__);
		Log("  pDllPath        : %ws", pDllPath);
		Log("  pDllLoadCallback: %p", pDllLoadCallback);
		Log("  pCallbackPtgr   : %p", pCallbackPtr);

		TCHAR	szModule[AGENT_PATH_SIZE] = L"";

		if (PathIsRelative(pDllPath))
		{
			WCHAR	szCurPath[SELFPT_PATH_SIZE];
			GetModuleFileName(NULL, szCurPath, sizeof(szCurPath));
			PathRemoveFileSpec(szCurPath);
			StringCbPrintf(m_szDllPath, sizeof(m_szDllPath), L"%s\\%s", szCurPath, pDllPath);
		}
		else
		{
			StringCbCopy(m_szDllPath, sizeof(m_szDllPath), pDllPath);
		}
		Log("  m_szDllPath     : %ws", m_szDllPath);
		if (PathFileExists(m_szDllPath))
		{
			m_module.hModule = LoadLibrary(m_szDllPath);
			if (m_module.hModule)
			{
				m_module.pCreateInstance = (PSelfpt_CreateInstance)GetProcAddress(m_module.hModule, "CreateInstance");
				m_module.pDeleteInstance = (PSelfpt_DeleteInstance)GetProcAddress(m_module.hModule, "DeleteInstance");
			}
			else
			{
				DWORD	dwError = GetLastError();
				Log("error: %ws, code=%d", m_szDllPath, dwError);
			}
		}
		else
		{
			Log("%ws is not found.\n", m_szDllPath);
		}
		if (m_module.pCreateInstance)
		{
			m_module.pInstance = m_module.pCreateInstance(pDllLoadCallback, pCallbackPtr);
		}
		SetDllLoadCallback(pDllLoadCallback, pCallbackPtr);
		Log("  hModule         : %p", m_module.hModule);
		Log("  pInstance       : %p", m_module.pInstance);
		Log("  CreateInstance  : %p", m_module.pCreateInstance);
		Log("  DeleteInstance  : %p", m_module.pDeleteInstance);
	}
	void		Destroy()
	{
		Log(__FUNCTION__);
		if (m_module.hModule)
		{
			Log("  hModule");
			if (m_module.pInstance)
			{
				Log("  pInstance");
				if (m_module.pDeleteInstance)
				{
					Log("  pDeleteInstance");
					m_module.pDeleteInstance(m_module.pInstance);
					Log("  deleted");
				}
			}
			FreeLibrary(m_module.hModule);
			ZeroMemory(&m_module, sizeof(m_module));
		}
	}
	void		SetDllLoadCallback
	(
		IN	PDllLoadCallback	pCallback,
		IN	LPVOID				pCallbackPtr
	)
	{
		m_callback.pDllLoadCallback = pCallback;
		m_callback.pCallbackPtr = pCallbackPtr;
	}
	bool		Install
	(
		IN	LPCWSTR		pProductCode,
		IN	LPCWSTR		pDriverPath,
		IN	bool		bRunInSafeMode = false
	)
	{
		try
		{
			if (m_module.pInstance)
				return m_module.pInstance->Install(pProductCode, pDriverPath, bRunInSafeMode);
		}
		catch (std::exception & e)
		{
			Log(e.what());
		}
		return false;
	}
	bool		IsInstalled()
	{
		try
		{
			if (m_module.pInstance)	return m_module.pInstance->IsInstalled();
		}
		catch (std::exception & e)
		{
			Log(e.what());
		}
		return false;
	}
	bool		Uninstall(IN PCWSTR pProductCode)
	{
		try
		{
			if (m_module.pInstance)	return m_module.pInstance->Uninstall(pProductCode);
		}
		catch (std::exception & e)
		{
			Log(e.what());
		}
		return false;
	}
	bool		Start()
	{
		try
		{
			if (m_module.pInstance) {
				return m_module.pInstance->Start();
			}
		}
		catch (std::exception & e)
		{
			Log(e.what());
		}
		return false;
	}
	bool		IsRunning()
	{
		try
		{
			if (m_module.pInstance)	return m_module.pInstance->IsRunning();
		}
		catch (std::exception & e)
		{
			Log(e.what());
		}
		return false;
	}
	bool		Shutdown()
	{
		try
		{
			if (m_module.pInstance)	return m_module.pInstance->Shutdown();
		}
		catch (std::exception & e)
		{
			Log(e.what());
		}
		return false;
	}
	bool		GetList(Selfpt::Object::Mode mode, Selfpt::Object::Type type)
	{
		try
		{
			if (m_module.pInstance)	return m_module.pInstance->GetList(mode, type);
		}
		catch (std::exception & e)
		{
			Log(e.what());
		}

		return false;
	}
	bool		Connect()
	{
		try
		{
			if (m_module.pInstance)
				return m_module.pInstance->Connect();
		}
		catch (std::exception & e)
		{
			Log(e.what());
		}

		return false;
	}
	void		Disconnect()
	{
		try
		{
			if (m_module.pInstance)
				return m_module.pInstance->Disconnect();
		}
		catch (std::exception & e)
		{
			Log(e.what());
		}
	}
	bool		IsConnected()
	{
		try
		{
			if (m_module.pInstance)	return m_module.pInstance->IsConnected();
		}
		catch (std::exception & e)
		{
			Log(e.what());
		}
		return false;
	}
	bool		RegisterProcess(IN DWORD dwProcessId = 0, IN bool bRegister = true)
	{
		try
		{
			if (m_module.pInstance)	return m_module.pInstance->RegisterProcess(dwProcessId, bRegister);
		}
		catch (std::exception & e)
		{
			Log(e.what());
		}
		return false;
	}
	bool		GetEmegentState()
	{
		try
		{
			if (m_module.pInstance)	return m_module.pInstance->GetEmegentState();
		}
		catch (std::exception & e)
		{
			Log(e.what());
		}
		return false;
	}
	void		SetEmergentState(IN bool bSet)
	{
		try
		{
			if (m_module.pInstance)	return m_module.pInstance->SetEmergentState(bSet);
		}
		catch (std::exception & e)
		{
			Log(e.what());
		}
	}
	bool		AddObject(Selfpt::Object::Mode mode, Selfpt::Object::Type type, PCWSTR pObjectValue)
	{
		try
		{
			if (m_module.pInstance)	return m_module.pInstance->AddObject(mode, type, pObjectValue);
		}
		catch (std::exception & e)
		{
			Log(e.what());
		}
		return false;
	}
	bool		RemoveObject(Selfpt::Object::Mode mode, Selfpt::Object::Type type, PCWSTR pObjectValue)
	{
		try
		{
			if (m_module.pInstance)	return m_module.pInstance->RemoveObject(mode, type, pObjectValue);
		}
		catch (std::exception & e)
		{
			Log(e.what());
		}
		return false;
	}
	bool		RemoveAllObject(Selfpt::Object::Mode mode, Selfpt::Object::Type type)
	{
		try
		{
			if (m_module.pInstance)	return m_module.pInstance->RemoveAllObject(mode, type);
		}
		catch (std::exception & e)
		{
			Log(e.what());
		}
		return false;
	}
	void		Release()
	{

	}
private:
	WCHAR			m_szDllPath[SELFPT_PATH_SIZE];
	WCHAR			m_szDriverPath[SELFPT_PATH_SIZE];
	struct
	{
		HMODULE		hModule;
		ISelfpt *	pInstance;
		PSelfpt_CI	pCreateInstance;
		PSelfpt_DI	pDeleteInstance;
	} m_module;
	struct
	{
		PDllLoadCallback		pDllLoadCallback;
		PVOID					pCallbackPtr;
	} m_callback;
};