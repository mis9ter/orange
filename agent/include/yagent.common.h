#pragma once
#include <strsafe.h>
#include <algorithm>
#include <memory>
#include <string>
#include "CAppRegistry.h"
#include "CTime.h"


#ifdef _M_X64
#pragma comment(lib, "yagent.common.x64.lib")
#else 
#pragma comment(lib, "yagent.common.win32.lib")
#endif


#ifndef LBUFSIZE
#define	LBUFSIZE	(4096)
#define	MBUFSIZE	(1024)
#define	SBUFSIZE	(256)
#define TBUFSIZE	(64)
#endif

#define PRECOMPILED_HEADER	"pch.h"
#define YAGENT_COMMON_BEGIN	namespace YAgent {
#define YAGENT_COMMON_END	};


#define	__ansi(str)			CANSI(str).Get()
#define __wide(str)			CWIDE(str).Get()
#define __utf8(str)			CUTF8(str).Get()
#define __utf16(str)		CUTF16(str).Get()
#define	__mstricmp(a,b)		_mbsicmp((const unsigned char*)(a? a:""), (const unsigned char*)(b? b:""))
#define	__mstrstr(a,b)		_mbsstr((const unsigned char*)(a? a:""), (const unsigned char*)(b? b:""))


typedef	BOOL(WINAPI* PInitializeCriticalSectionEx)
(
	LPCRITICAL_SECTION lpCriticalSection,
	DWORD dwSpinCount,
	DWORD Flags
);
class CLock
{
public:
	CLock()
	{
		m_proc = (PInitializeCriticalSectionEx)::GetProcAddress(::GetModuleHandle(_T("kernel32")), "InitializeCriticalSectionEx");
		if (m_proc)
		{
			m_proc(&m_section, 0, CRITICAL_SECTION_NO_DEBUG_INFO);
		}
		else
		{
			::InitializeCriticalSection(&m_section);
		}
	}
	virtual ~CLock()
	{
		::DeleteCriticalSection(&m_section);
	}
	inline	void	Lock(IN LPCSTR pCause=NULL)
	{
		UNREFERENCED_PARAMETER(pCause);
		::EnterCriticalSection(&m_section);
	}
	inline	void	Unlock(IN LPCSTR pCause=NULL)
	{
		UNREFERENCED_PARAMETER(pCause);
		::LeaveCriticalSection(&m_section);
	}
	inline	bool	TryLock(IN LPCSTR pCause=NULL)
	{
		UNREFERENCED_PARAMETER(pCause);
		return	::TryEnterCriticalSection(&m_section) ? true : false;
	}
	CRITICAL_SECTION* Get()
	{
		return &m_section;
	}

private:
	TCHAR							m_szName[SBUFSIZE];
	CRITICAL_SECTION				m_section;
	PInitializeCriticalSectionEx	m_proc;
};

typedef std::shared_ptr <CLock>	CLockPtr;

#ifndef __function_lock 
class CFunctionLock
{
public:
	CFunctionLock(IN CRITICAL_SECTION* pSection, IN LPCSTR pCause, IN bool bLog = false)
	{
		m_bLog = bLog;
		m_pCause = pCause;

		::EnterCriticalSection(pSection);
		m_pSection = pSection;
	}
	CFunctionLock(IN CLockPtr lockptr, IN LPCSTR pCause, IN bool bLog = false)
		: CFunctionLock(lockptr.get()->Get(), pCause, bLog)
	{
		m_lockptr = lockptr;
	}
	~CFunctionLock()
	{
		::LeaveCriticalSection(m_pSection);
		m_lockptr = nullptr;
	}

private:
	bool						m_bLog;
	CRITICAL_SECTION			* m_pSection;
	CLockPtr					m_lockptr;
	LPCSTR						m_pCause;
};
#define	__function_lock(lock)	CFunctionLock(lock, __FUNCTION__)
#endif

namespace YAgent {
	PCWSTR		GetDataFolder(IN PCWSTR pName, OUT PWSTR pValue, IN DWORD dwSize);
	HANDLE		Run(IN LPCWSTR pFilePath, IN PCWSTR pArg);
	bool		Alert(PCWSTR pFormat, ...);

	LPSTR		WideToAnsiString(LPCWSTR pSrc);
	LPTSTR		AnsiToWide(LPCSTR szSrc, UINT uCodePage = CP_ACP);
	LPSTR		WideToAnsi(LPCWSTR szSrcWStr, UINT uCodePage = CP_ACP);
	void		FreeString(LPVOID szStrBuf);

	DWORD		GetCurrentSessionId();
	DWORD		GetBootId();
	PCWSTR		GetMachineGuid(IN PWSTR pValue, IN DWORD dwSize);
	BOOL		CreateDirectory(LPCTSTR lpPath);
	BOOL		MakeDirectory(LPCTSTR lpPath);
	LPCTSTR		GetURLPath(IN LPCTSTR pFilePath, OUT LPTSTR lpPath, IN DWORD dwSize);
	bool		GetRealPath(IN LPCTSTR pLinkPath, OUT LPTSTR pRealPath, IN DWORD dwSize);
	int			CompareBeginningString(IN LPCTSTR lpStr1, IN LPCTSTR lpStr2);
	int			IsStartedSameNoCase(IN LPCTSTR lpStr1, IN LPCTSTR lpStr2);
	bool		IsDirectory(LPCTSTR lpPath);
	bool		GetModulePath(OUT LPTSTR szPath, IN DWORD dwSize);
	bool		GetInstancePath(IN HINSTANCE hInstance, OUT LPTSTR szPath, IN DWORD dwSize);
	bool		GetLinkPath(IN LPCTSTR pLinkFile,
		OUT LPTSTR pLinkPath, IN DWORD dwLinkPathSize,
		OUT WIN32_FIND_DATA* pFd);
	bool		GetLinkInfo
	(
		IN	LPCTSTR		pLinkFile,
		OUT LPTSTR		pLinkPath,
		IN	DWORD		dwLinkPathSize,
		OUT WIN32_FIND_DATA* pFd,
		OUT	LPTSTR		pArgument,
		IN	DWORD		dwArgumentSize,
		OUT	LPTSTR		pWorkingDirectory,
		IN	DWORD		dwWorkingDirectorySize,
		OUT	LPTSTR		pDescription,
		IN	DWORD		dwDescriptionSize
	);

	LPCTSTR		GetFileExt(IN LPCTSTR pFilePath);
	LPCTSTR		GetFileFullExt(IN LPCTSTR pFilePath);

	DWORD64		GetFileSize64(IN HANDLE hFile);
	DWORD64		GetFileSize64ByPath(IN LPCTSTR lpPath);
	bool		FindFile(IN LPCTSTR lpPath, OUT WIN32_FIND_DATA* fd);
	bool		IsFileExist(IN LPCTSTR lpPath);
	DWORD		GetFileSize32(IN LPCTSTR lpPath, OUT bool* bExist);
	DWORD		GetFileSize(IN LPCTSTR lpPath, OUT bool* bExist);

	LPTSTR		ReplaceString(IN LPTSTR lpSrc, IN DWORD dwSrcSize,
		IN LPCTSTR lpKey, IN LPCTSTR lpRep);
};
