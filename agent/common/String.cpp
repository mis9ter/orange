#include "pch.h"

YAGENT_COMMON_BEGIN

/**	
\brief	ANSI 문자열을 Unicode 문자열로 전환하는 함수
\return	Windows 9x인 경우 true
\sa		FreeString() - 내부적으로 메모리를 할당하므로 사용한 이후 
명시적으로 호출해줘야 하는 함수
*/
LPWSTR	AnsiToWide(LPCSTR szSrcStr, UINT uCodePage)
{
	if( NULL == szSrcStr )	return NULL;

	LPWSTR	pszDestStr	= NULL;
	INT		nLength		= 0;
	INT		nSize		= 0;

	// Get ANSI Size
	nLength		= ::MultiByteToWideChar(uCodePage, 0, szSrcStr, -1, pszDestStr, 0);
	nSize		= nLength * sizeof(WCHAR);

	if( nLength )
	{
		pszDestStr = new WCHAR[nSize];
	}
	else
	{
		pszDestStr = NULL;
	}
	if(NULL != pszDestStr)
	{
		if(0 == ::MultiByteToWideChar(uCodePage, 0, szSrcStr, -1, pszDestStr, nLength))
		{
			pszDestStr = NULL;
		}
		else
		{
			// do nothing
		}
	}
	else
	{
		// do nothing
	}
	return pszDestStr;
}
LPSTR	WideToAnsiString(LPCWSTR pSrc)
{
	if( NULL == pSrc )	return NULL;

	DWORD	dwLen	= (pSrc? lstrlen(pSrc) : 0) * 6 + 1;
	LPSTR	pDest	= new char[dwLen];

	if( NULL == pDest )
	{
		return NULL;
	}

	LPSTR	pIdx;

	for( pIdx = pDest ; *pSrc ; pSrc++ )
	{
		if( *pSrc >= _T('A') && *pSrc <= _T('Z') )
		{
			*pIdx++	= 'A' + *pSrc - _T('A');
		}
		else if(  *pSrc >= _T('a') && *pSrc <= _T('z') )
		{
			*pIdx++	= 'a' + *pSrc - _T('a');
		}
		else if( *pSrc >= _T('0') && *pSrc <= _T('9') )
		{
			*pIdx++	= '0' + *pSrc - _T('0');
		}
		else if( *pSrc == _T('.') )
		{
			*pIdx++	= '.';
		}
		else if( *pSrc == _T(' ') )
		{
			*pIdx++	= ' ';
		}
		else if( *pSrc == _T('/') )
		{
			*pIdx++	= '/';
		}
		else if( *pSrc == _T('\\') )
		{
			*pIdx++	= '\\';
		}
		else
		{
			::StringCbPrintfA(pIdx, dwLen - (pIdx - pDest), "\\U%04x", *pSrc);
			pIdx	+= 6;
		}
	}
	*pIdx	= NULL;

	return pDest;
}

/**	
\brief	Unicode 문자열을 ANSI 문자열로 전환하는 함수
\return	Unicode 문자열의 포인터, 실패의 경우 NULL
\sa		FreeString() - 내부적으로 메모리를 할당하므로 사용한 이후 
명시적으로 호출해줘야 하는 함수
*/
LPSTR	WideToAnsi(LPCWSTR szSrcWStr, UINT uCodePage)
{
	if( NULL == szSrcWStr )	return NULL;

	LPSTR	pszDestStr = NULL;
	INT		nDestSize = 0;

	// Get ANSI Size
	nDestSize = WideCharToMultiByte(uCodePage, 0, szSrcWStr, -1, 
		pszDestStr, 0, NULL, NULL);
	if(0 != nDestSize)
	{

		pszDestStr = new char[nDestSize];
		// 이 함수내에서 _log 를 호출하면 안된다.
		// _log 내에서 WideToAnsi를 사용하므로 무한 재귀 호출이 되어 스택 오버플로우로 죽는다.
		//_logA("WideToAnsi:%d", nDestSize);
	}
	else
	{
		pszDestStr = NULL;
	}

	if(NULL != pszDestStr)
	{
		if(0 == WideCharToMultiByte(uCodePage, 0, szSrcWStr, -1,
			pszDestStr, nDestSize, NULL, NULL))
		{
			_nlogA(_T("xcommon"), "WideCharToMultiByte() failed. code = %d", ::GetLastError());
			delete pszDestStr;
			pszDestStr = NULL;
		}
		else
		{
			// do nothing
			//_nlogA(_T("xcommon"), pszDestStr);
		}
	}
	else
	{
		// do nothing
	}

	return pszDestStr;
}
/**	
\brief	할당된 문자열 버퍼를 해제하는 함수
\param	IN LPVOID szStrBuf: 해제할 문자열 버퍼
\return	void
\sa		AnsiToWide(), WideToAnsi()
*/
void		FreeString(LPVOID szStrBuf)
{
	if( szStrBuf )	delete szStrBuf;
}
YAGENT_COMMON_END
