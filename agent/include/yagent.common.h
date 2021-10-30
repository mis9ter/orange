#pragma once
#include <strsafe.h>
#include <algorithm>
#include <memory>
#include <string>
#include <functional>
#include <iostream>
#include <fstream>

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

#include "yagent.string.h"

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
	inline	void	Lock(PVOID pContext, std::function<void (PVOID)> pCallback) {
		::EnterCriticalSection(&m_section);
		if( pCallback )	pCallback(pContext);
		::LeaveCriticalSection(&m_section);
	
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

		m_pSection = pSection;
		::EnterCriticalSection(pSection);
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

typedef UINT64	BootUID;

namespace YAgent {
	bool		DeleteDirectoryFiles(IN LPCTSTR lpPath);
	bool		DeleteDirectory(IN LPCTSTR lpPath);
	bool		Command(IN LPCTSTR lpPath, IN LPCTSTR szArgument, 
		IN LPCTSTR lpRunPath,
		IN DWORD dwType =SW_SHOW , 
		IN bool bWait =false , 
		IN int nWaitSeconds =0 , 
		OUT DWORD* dwExitCode =0 );
	bool		RequestFile
	(
		IN	LPCTSTR		pUrl,		//
		OUT	LPCTSTR		lpPath,		// 응답은 파일로 만들어진다.
		IN	DWORD		*pdwSize,	// IN lpPath의 크기, OUT 응답 파일의 크기 
		IN	LPVOID		pContext,
		IN	std::function<bool (LPVOID pContext, LPCWSTR lpPath, IN HANDLE hFile, IN DWORD dwTotalBytes, IN DWORD dwCurrentBytes)>	pCallback
	);
	bool		RequestUrl(IN LPCTSTR pUrl, OUT char ** pResponse, OUT DWORD * pdwResponseSize);
	bool		GetAppTempPath(OUT LPTSTR szPath, IN DWORD dwSize);
	bool		SetPrivilege(HANDLE hToken, LPCTSTR Privilege, BOOL bEnablePrivilege);
	HANDLE		SetDebugPrivilege();
	void		UnsetDebugPrivilege(IN HANDLE hToken);
	HANDLE		GetProcessHandle(IN DWORD PID);
	bool		SetFileContent(IN LPCTSTR lpPath, IN PVOID lpContent, IN DWORD dwSize);
	PVOID		GetFileContent(IN LPCTSTR lpPath, OUT DWORD * pSize);
	void		FreeFileContent(PVOID p);
	PCWSTR		GetDataFolder(IN PCWSTR pName, OUT PWSTR pValue, IN DWORD dwSize);
	inline	PCWSTR		GetDataFolder(OUT PWSTR pValue, DWORD dwSize) {
		return GetDataFolder(AGENT_DATA_FOLDERNAME, pValue, dwSize);
	}
	inline	PCWSTR		GetDataFolder(std::wstring& path) {
		WCHAR	szPath[AGENT_PATH_SIZE]	= L"";
		path	= GetDataFolder(szPath, sizeof(szPath));
		return path.c_str();
	}
	HANDLE		Run(IN LPCWSTR pFilePath, IN PCWSTR pArg);
	bool		Alert(PCWSTR pFormat, ...);

	LPSTR		WideToAnsiString(LPCWSTR pSrc);
	LPTSTR		AnsiToWide(LPCSTR szSrc, UINT uCodePage = CP_ACP);
	LPSTR		WideToAnsi(LPCWSTR szSrcWStr, UINT uCodePage = CP_ACP);
	void		FreeString(LPVOID szStrBuf);

	DWORD		GetCurrentSessionId();
	DWORD		GetBootId();
	BootUID		GetBootUID();
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


class CAppLog
{
public:
	CAppLog(IN PCWSTR pFilePath = AGENT_DEFAULT_LOG_NAME)
	{
		InitializeCriticalSection(&m_lock);
		MakeLogPath(pFilePath);
	}
	~CAppLog()
	{
		DeleteCriticalSection(&m_lock);
	}
	bool	Log(IN const char* pFmt, ...)
	{
		if (NULL == pFmt)
		{
			return DeleteFile(m_szLogPath) ? true : false;
		}
		SYSTEMTIME	st;
		GetLocalTime(&st);
		va_list		argptr;
		char		szBuf[4096] = "";
		StringCbPrintfA(szBuf, sizeof(szBuf), "%02d/%02d %02d:%02d:%02d.%03d P:%06d T:%06d ",
			st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
			GetCurrentProcessId(), GetCurrentThreadId());
		va_start(argptr, pFmt);
		int		nLength = lstrlenA(szBuf);
		::StringCbVPrintfA(szBuf + nLength, sizeof(szBuf) - nLength, pFmt, argptr);
		va_end(argptr);
		nLength = lstrlenA(szBuf);
		puts(szBuf);
		StringCbCopyA(szBuf + nLength, sizeof(szBuf) - nLength, "\n");
		bool	bRet = WriteLog(szBuf, nLength + 1) ? true : false;
		return bRet;
	}

private:
	CRITICAL_SECTION	m_lock;
	HANDLE		m_hFile;
	WCHAR		m_szLogPath[AGENT_PATH_SIZE];
	void		MakeLogPath(PCWSTR pFilePath)
	{
		WCHAR		szPath[AGENT_PATH_SIZE];

		YAgent::GetDataFolder(szPath, sizeof(szPath));
		StringCbPrintf(m_szLogPath, sizeof(m_szLogPath), L"%s\\%s", szPath, pFilePath);
	}
	bool		WriteLog(IN const char* pMsg, IN size_t dwSize)
	{
		HANDLE		hFile = INVALID_HANDLE_VALUE;
		DWORD		dwBytes;

		hFile = ::CreateFile(m_szLogPath, FILE_APPEND_DATA | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == hFile)
		{
			return false;
		}
		::SetFilePointer(hFile, 0, NULL, FILE_END);
		::WriteFile(hFile, pMsg, (DWORD)dwSize, &dwBytes, NULL);
		if (INVALID_HANDLE_VALUE != hFile)	::CloseHandle(hFile);
		return true;
	}
};