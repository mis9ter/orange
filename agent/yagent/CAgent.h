#pragma once

#include <Windows.h>
#include <functional>
#include <thread>

#include "yagent.h"
#include "CThread.h"
#include "IFilterCtrl.h"
#include "CDialog.h"
#include "CException.h"

#include "yagent.common.h"
#pragma comment(lib, "yagent.common.lib")

#include "CEventCallback.h"

typedef std::function<void(HANDLE hShutdown, void * pCallbackPtr)>	PFUNC_AGENT_RUNLOOP;

class CAgent
	:
	public	CFilterCtrl,
	public	CEventCallback
{
public:
	CAgent();
	virtual	~CAgent();

	bool			Install();
	void			Uninstall();
	bool			Start(void* pCallbackPtr, PFUNC_AGENT_RUNLOOP pCallback);
	void			Shutdown();
	void			RunLoop();

	static	CMemoryPtr	GetResourceData(IN HMODULE hModule, IN UINT nResourceId)
	{
		HRSRC		hRes = NULL;
		HGLOBAL		hData = NULL;
		CMemoryPtr	ptr;

		auto		GetModulePath = [&](IN HMODULE hModule, OUT LPTSTR pPath, IN DWORD dwSize)
		{
			GetModuleFileName(hModule, pPath, dwSize);
			return pPath;
		};

		TCHAR		szPath[AGENT_PATH_SIZE] = _T("");
		do
		{
			if (NULL == (hRes = ::FindResource(hModule, MAKEINTRESOURCE(nResourceId), RT_RCDATA)))
			{
				break;
			}
			if (NULL == (hData = ::LoadResource(hModule, hRes)))
			{
				break;
			}
			DWORD	dwSize = ::SizeofResource(hModule, hRes);
			LPVOID	pData = ::LockResource(hData);
			if (pData && dwSize)
			{
				if (ptr = CMemoryFactory::Instance()->AllocatePtr(__FUNCTION__, pData, dwSize))
				{

				}
				else
				{

				}
			}
			else
			{
#ifdef __log_config
				_nlogA(__log_config, "LockResource() failed. path=%s, code=%d",
					GetModulePath(hModule, szPath, sizeof(szPath)), ::GetLastError());
#endif
			}
		} while (false);
		/*
			hData, pData 이런 것들을 해제하지 않는 이유:
			이 프로세스의 내용이고 이미 메모리에 올라가 있어요.
			프로세스 종료시 알아서 사라집니다.
			걱정마세요.
		*/
		return ptr;
	}

private:
	struct m_config {
		bool				bRun;
		HANDLE				hShutdown;
		void				*pRunLoopPtr;
		PFUNC_AGENT_RUNLOOP	pRunLoopFunc;
	} m_config;
	CThread					m_main;
	static	void			MainThread(void * ptr, HANDLE hShutdown);
};