#include "pch.h"

bool	YAgent::Alert(PCWSTR fmt, ... )
{
	va_list	argptr;
	TCHAR	szBuf[LBUFSIZE];
	DWORD	dwCnt;
	TCHAR	szCaption[MBUFSIZE];

	if( NULL == fmt )
	{
		return false;
	}

	//	2017/07/27	BEOM
	//	아래 구문을 세션 아이디로 검사하는걸로 대체한다.
	if( 0 == GetCurrentSessionId() )
		return 0;

	// 2014/02/17 개선 사항
	// 현 사용자의 윈도 로긴 아이디가 "SYSTEM"인 경우 서비스 모드로 동작 중이거나 서비스 모드에 의해 실행된 상태.
	// 이 경우에는 사용자에게 UI를 출력하지 않고, 로그 파일로 내용을 출력한다.

	::GetModuleFileName(NULL, szCaption, sizeof(szCaption));
	va_start(argptr, fmt);
	dwCnt	= ::StringCbVPrintf(szBuf, sizeof(szBuf), fmt, argptr);

	{
		TCHAR	szUserId[SBUFSIZE]	= _T("");
		DWORD	dwUserIdSize		= sizeof(szUserId)/sizeof(TCHAR);

		::GetUserName(szUserId, &dwUserIdSize);
		if( !_tcsicmp(szUserId, _T("SYSTEM")) )
		{
			// 서비스 모드로 동작 중
			//_log(_T("ALERT:%s"), szBuf);
		}
		else
		{
			MessageBox(NULL, szBuf, szCaption, MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
		}
	}
	va_end(argptr);
	/*
	2006.06.13	김&조
	return 값을 출력한 문자의 갯수로 하려고 했나 본데 그럴리 없다. 으하하하
	StringcbPrintf는 버퍼의 길이를 리턴하지 않는다. 대세에 지장 없어서 그냥 간다.
	*/
	return (dwCnt / sizeof(TCHAR))? true : false;


}
