#pragma once
#include "DbgHelp.h"
#include "yagent.common.h"

#define	MAXIMUM_DUMP_COUNT	5
#define DUMP_PATH			L"dump"
#define DUMP_EXT			L"dmp"
typedef BOOL(WINAPI* PMiniDumpWriteDump)
(
	HANDLE hProcess,
	DWORD ProcessId,
	HANDLE hFile,
	MINIDUMP_TYPE DumpType,
	PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	PMINIDUMP_CALLBACK_INFORMATION CallbackParam
);

class CException
{
public:
	CException()
	{

	}
	virtual ~CException()
	{

	}

	static	LPCWSTR		GetDumpPath(IN LPTSTR pPath, IN DWORD dwSize, IN LPCWSTR pExt)
	{
		TCHAR	szName[SBUFSIZE] = L"";

		SYSTEMTIME		st;
		static DWORD	dwCount;
		TCHAR			szBasePath[1024];

		::GetLocalTime(&st);
		YAgent::GetModulePath(szBasePath, sizeof(szBasePath));
		::StringCbCat(szBasePath, sizeof(szBasePath), DUMP_PATH);
		if (!YAgent::IsDirectory(szBasePath))
		{
			YAgent::MakeDirectory(szBasePath);
		}

		//	2017/05/31	BEOM
		//	DUMP파일을 기존에는 매번 다른 이름으로 만들고 있었다.
		//	이 경우 다양한 덤프 파일을 확보할 수 있다는 장점이 있으나, 
		//	프로그램 문제로 생성 덤프의 개수가 많을 경우 덤프 땜에 하드디스크가 꽉 찰수도 있다.
		//	해서 덤프는 5개 이하로만 생성되도록 한다.

#ifdef MAXIMUM_DUMP_COUNT

		TCHAR	szThisPath[1024];
		TCHAR	szDumpPath[1024];
		LPCTSTR	pFileName;

		::GetModuleFileName(NULL, szThisPath, sizeof(szThisPath));
		pFileName = _tcsrchr(szThisPath, _T('\\'));
		if (pFileName)
			pFileName++;
		else
			pFileName = szThisPath;

		for (unsigned i = 0; i < MAXIMUM_DUMP_COUNT; i++)
		{
			::StringCbPrintf(szName, sizeof(szName), L"\\%s(%d).%s", pFileName, i, pExt);
			::StringCbPrintf(szDumpPath, sizeof(szDumpPath), L"%s%s", szBasePath, szName);
			if (YAgent::IsFileExist(szDumpPath))
			{
				//::StringCbPrintf(szName, sizeof(szName), _T("\\%s(%d).%s"), pFileName, dwCount, pExt);
			}
			else
			{
				break;
			}
		}

#else

		::StringCbPrintf(szName, sizeof(szName), _T("\\%04d%02d%02d_%02d%02d%02d_%03d_%04d_%08x.%s"),
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
			dwCount++, ::GetCurrentThreadId(), pExt);
#endif

		YAgent::GetModulePath(pPath, dwSize);
		::StringCbCat(pPath, dwSize, DUMP_PATH);
		if (!YAgent::IsDirectory(pPath))
		{
			YAgent::MakeDirectory(pPath);
		}
		::StringCbCat(pPath, dwSize, szName);
		return pPath;
	}

	static	LONG WINAPI	Handler(LPCSTR pCause, DWORD dwLine, struct _EXCEPTION_POINTERS* p)
	{
		TCHAR	szPath[MBUFSIZE] = _T("");

		GetDumpPath(szPath, sizeof(szPath), (LPTSTR)DUMP_EXT);
		//xcommon::_nlogA(_T("exception"), __line);
		//xcommon::_nlogA(_T("exception"), "exception - %s", pCause);
		Dump(pCause, dwLine, szPath, p);

		return EXCEPTION_EXECUTE_HANDLER;
	}
	static	LONG WINAPI GlobalFilter(struct _EXCEPTION_POINTERS* p)
	{
		return Handler(__FUNCTION__, __LINE__, p);
	}
	static	bool	Save(IN LPCSTR pCause, IN char* p, IN DWORD dwSize)
	{
		TCHAR	szPath[MBUFSIZE] = _T("");

		if (p && dwSize)
		{
			GetDumpPath(szPath, sizeof(szPath), DUMP_EXT);
		}
		else
		{
		}
		return true;
	}
	static	bool	Dump(IN LPCSTR pFile, IN DWORD dwLine, IN LPCTSTR lpPath, IN struct _EXCEPTION_POINTERS* p,
		bool bExitProcess = true, IN int nExitCode = 0)
	{
		bool		bRet = false;
		HINSTANCE	hDll = ::LoadLibrary(_T("DBGHELP.DLL"));
		HANDLE		hFile = INVALID_HANDLE_VALUE;

		__try
		{
			if (NULL == hDll)	__leave;
			PMiniDumpWriteDump pDump = (PMiniDumpWriteDump)::GetProcAddress(hDll, "MiniDumpWriteDump");
			if (NULL == pDump)	__leave;
			hFile = ::CreateFile(lpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (INVALID_HANDLE_VALUE == hFile)	__leave;

			_MINIDUMP_EXCEPTION_INFORMATION   ExInfo;
			ExInfo.ThreadId = ::GetCurrentThreadId();
			ExInfo.ExceptionPointers = p;       // Exception 정보 설정
			ExInfo.ClientPointers = NULL;
			if (pDump(::GetCurrentProcess(), ::GetCurrentProcessId(), hFile, MiniDumpWithFullMemory, &ExInfo, NULL, NULL))
			{
				bRet = true;
			}
		}
		__finally
		{
			if (INVALID_HANDLE_VALUE != hFile)
			{
				::CloseHandle(hFile);
			}

			if (hDll)	FreeLibrary(hDll);
		}
		if (bExitProcess)
			::ExitProcess(nExitCode);
		return bRet;
	}

};