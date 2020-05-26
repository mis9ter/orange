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

typedef std::function<void(HANDLE hShutdown, void * pCallbackPtr)>	PFUNC_AGENT_RUNLOOP;

class CAgent
	:
	public	CFilterCtrl
{
public:
	CAgent();
	virtual	~CAgent();

	bool			Install();
	void			Uninstall();
	bool			Start(void* pCallbackPtr, PFUNC_AGENT_RUNLOOP pCallback);
	void			Shutdown();
	void			RunLoop();
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