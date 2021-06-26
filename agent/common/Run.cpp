#include "pch.h"
#include <shellapi.h>

YAGENT_COMMON_BEGIN
static bool	_Execute(LPCTSTR lpPath, LPCTSTR szArgument, 
	LPCTSTR lpRunPath,
	DWORD nShowCmd, 
	bool bWait, int nWaitSeconds, DWORD* lpExitCode)
{
	bool		bRet	= false;
	int			nRet	= 0;

	if( bWait )
	{
		SHELLEXECUTEINFO	shInfo;
		ZeroMemory(&shInfo, sizeof(shInfo));

		shInfo.cbSize		= sizeof(SHELLEXECUTEINFO);
		shInfo.lpVerb		= TEXT("open");
		shInfo.fMask		= SEE_MASK_NOCLOSEPROCESS;
		shInfo.lpFile		= lpPath;
		shInfo.lpParameters	= szArgument;
		shInfo.nShow		= nShowCmd;		
		shInfo.lpDirectory	= lpRunPath;

		//::SetErrorMode(SEM_FAILCRITICALERRORS);

		nRet	= ::ShellExecuteEx(&shInfo);

		//_log(_T("SHELLEXECUTE:[%s][%s],RET:%d"), lpPath, szArgument, nRet);

		if( nRet )
		{
			if( shInfo.hProcess )
			{
				if( nWaitSeconds )
				{
					::WaitForSingleObject(shInfo.hProcess, nWaitSeconds);
				}
				else
				{
					::WaitForSingleObject(shInfo.hProcess, INFINITE);
				}
				if( lpExitCode )
				{
					GetExitCodeProcess(shInfo.hProcess, lpExitCode);
				}
				::CloseHandle(shInfo.hProcess);
				bRet	= true;
			}
			//_log(_T("WAIT_DONE"));
		}
		else
		{
			//_log(_T("error: ShellExecuteEx() failed. error code = %d"),
			//	::GetLastError());
		}

	}
	else
	{
		// 파일을 성공적으로 실행하는 것으로 끝이다.
		nRet	= (INT)::ShellExecute(NULL, TEXT("open"), lpPath, szArgument, 
			lpRunPath, nShowCmd);
		if( 32 < nRet )
		{
			bRet	= true;
		}
		else
		{
			//_log(_T("error: ShellExecute() failed. error code = %d"), ::GetLastError());
			//_log(_T("RUN:[PATH=%s][ARG=%s][RUNPATH=%s]"), lpPath, szArgument, lpRunPath);
		}
	}
	return bRet? true : false;
}

bool	Command(IN LPCTSTR lpPath, IN LPCTSTR szArgument, 
	IN LPCTSTR lpRunPath,
	IN DWORD dwType/* =SW_SHOW */, IN bool bWait/* =false */, 
	IN int nWaitSeconds/* =0 */, OUT DWORD* dwExitCode/* =0 */)
{
	bool	bRet;
	LPTSTR	pPath;
	TCHAR	szCurPath[MBUFSIZE]		= _T("");
	TCHAR	szTargetPath[MBUFSIZE]	= _T("");
	TCHAR	szShortPath[MBUFSIZE]	= _T("");
	TCHAR	szArg[LBUFSIZE]			= _T("");
	LPCTSTR	pArg;

	//::GetModuleFileName(NULL, szCurPath, sizeof(szCurPath));
	::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
	::GetCurrentDirectory(sizeof(szCurPath), szCurPath);

	if( NULL == lpRunPath )
	{
		::StringCbCopy(szTargetPath, sizeof(szTargetPath), lpPath);
		pPath	= _tcsrchr(szTargetPath, _T('\\'));
		if( pPath )
		{
			*pPath	= NULL;
		}
		else
		{
			GetModulePath(szTargetPath, sizeof(szTargetPath));
		}
	}
	else
	{
		::StringCbCopy(szTargetPath, sizeof(szTargetPath), lpRunPath);
	}

	pPath	= const_cast<LPTSTR>(lpPath);
#ifdef USE_SHORTPATH
	///////////////////////////////////////////////////////////////////////////
	// Windows 9x 계열에서는 경로를 Short Path로 바꿔서 실행해준다.
	///////////////////////////////////////////////////////////////////////////
	if( ::AhnIsWindows9x() )
	{
		if( ::GetShortPathName(lpPath, szShortPath, sizeof(szShortPath)) )
		{
			pPath	= szShortPath;
		}
	}
#endif

	/*
	Argument를 쌍따옴표를 싸면 ... 태형차장이 싫어하던데.
	if( szArgument && _T('\"') != szArgument[0] )
	{
	::StringCbPrintf(szArg, sizeof(szArg), _T("\"%s\""), szArgument);
	pArg	= szArg;
	}
	else
	{
	pArg	= szArgument;
	}
	*/
	pArg	= szArgument;
	::SetCurrentDirectory(szTargetPath);

	//	2019/11/12	TigerJ
	//	경로는 ""으로 감싸져 있지 않으면 감싸준다. 
	//	경로에 공백 있는 걸 이용해서 또 공격하시는 찌질이들이 있다는 소문.
	TCHAR	szPath[4096] = _T("");
	if (pPath && _T('"') != *pPath)
	{
		::StringCbPrintf(szPath, sizeof(szPath), _T("\"%s\""), pPath);
		pPath = szPath;
	}
	bRet	= _Execute(pPath, pArg, szTargetPath, dwType, bWait, nWaitSeconds, dwExitCode);
	::SetCurrentDirectory(szCurPath);
	return bRet;
}

HANDLE	Run(IN PCWSTR pFilePath, IN PCWSTR pArg) {

	WCHAR	szPath[AGENT_PATH_SIZE];

	if( NULL == pFilePath || NULL == *pFilePath )	return NULL;
	StringCbCopy(szPath, sizeof(szPath), pFilePath);
	if( pArg ) {
		StringCbCat(szPath, sizeof(szPath), L" ");
		StringCbCat(szPath, sizeof(szPath), pArg);
	}
	STARTUPINFO			si	= {NULL,};
	PROCESS_INFORMATION	pi	= {NULL,};

	if( CreateProcess(NULL, szPath, NULL, NULL, FALSE,   
		CREATE_BREAKAWAY_FROM_JOB|CREATE_NEW_PROCESS_GROUP|
		CREATE_DEFAULT_ERROR_MODE|CREATE_UNICODE_ENVIRONMENT,
		NULL, NULL, &si, &pi) ) {
		return pi.hProcess;		
	}
	return NULL;	
}

YAGENT_COMMON_END