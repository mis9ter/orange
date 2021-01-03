#include "pch.h"

YAGENT_COMMON_BEGIN

DWORD	GetCurrentSessionId()
{
	DWORD	dwProcessId;
	DWORD	dwSessionId;

	dwProcessId = ::GetCurrentProcessId();
	::ProcessIdToSessionId(dwProcessId, &dwSessionId);

	return dwSessionId;
}

YAGENT_COMMON_END