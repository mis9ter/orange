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
		//	DUMP������ �������� �Ź� �ٸ� �̸����� ����� �־���.
		//	�� ��� �پ��� ���� ������ Ȯ���� �� �ִٴ� ������ ������, 
		//	���α׷� ������ ���� ������ ������ ���� ��� ���� ���� �ϵ��ũ�� �� ������ �ִ�.
		//	�ؼ� ������ 5�� ���Ϸθ� �����ǵ��� �Ѵ�.

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
			ExInfo.ExceptionPointers = p;       // Exception ���� ����
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