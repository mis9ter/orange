#pragma once

#include <windows.h>
#include <strsafe.h>
#include <tchar.h>

#include "yagent.define.h"
#include "yagent.common.h"
#include "CAppLog.h"
#include "CDialog.h"

#pragma comment(lib, "yfilterctrl.lib")

#ifndef __function_lock
class CFunctionLock
{
public:
	CFunctionLock(CRITICAL_SECTION * p, IN LPCSTR pCause)
	{
		UNREFERENCED_PARAMETER(pCause);
		m_p = p;
		if (m_p)	EnterCriticalSection(m_p);
	}
	~CFunctionLock()
	{
		if (m_p)		LeaveCriticalSection(m_p);
	}
private:
	CRITICAL_SECTION *	m_p;
};
#define __function_lock(pCriticalSection)	CFunctionLock	__lock(pCriticalSection, __FUNCTION__)
#endif