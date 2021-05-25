#pragma once

#include <windows.h>
#include <strsafe.h>
#include <tchar.h>

#include "yagent.define.h"
#include "yagent.common.h"
#include "CAppLog.h"
#include "CDialog.h"
#include "CMemory.h"

#ifdef _M_X64
#pragma comment(lib, "yfilterctrl.x64.lib")
#pragma comment(lib, "jsoncpp.x64.lib")
#else 
#pragma comment(lib, "yfilterctrl.win32.lib")
#pragma comment(lib, "jsoncpps.win32.lib")
#endif

/*
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
*/


LPCSTR	SystemTime2String(IN SYSTEMTIME* p, OUT LPSTR pStr, IN DWORD dwSize,
			IN bool bTimer = false);
LPCSTR	UUID2StringA(IN UUID* p, OUT LPSTR pStr, IN DWORD dwSize);
LPCWSTR	UUID2StringW(IN UUID* p, OUT LPWSTR pStr, IN DWORD dwSize);

