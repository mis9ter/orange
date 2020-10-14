#include "pch.h"

YAGENT_COMMON_BEGIN

/**	
\brief	ANSI ���ڿ��� Unicode ���ڿ��� ��ȯ�ϴ� �Լ�
\return	Windows 9x�� ��� true
\sa		FreeString() - ���������� �޸𸮸� �Ҵ��ϹǷ� ����� ���� 
��������� ȣ������� �ϴ� �Լ�
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
\brief	Unicode ���ڿ��� ANSI ���ڿ��� ��ȯ�ϴ� �Լ�
\return	Unicode ���ڿ��� ������, ������ ��� NULL
\sa		FreeString() - ���������� �޸𸮸� �Ҵ��ϹǷ� ����� ���� 
��������� ȣ������� �ϴ� �Լ�
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
		// �� �Լ������� _log �� ȣ���ϸ� �ȵȴ�.
		// _log ������ WideToAnsi�� ����ϹǷ� ���� ��� ȣ���� �Ǿ� ���� �����÷ο�� �״´�.
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
\brief	�Ҵ�� ���ڿ� ���۸� �����ϴ� �Լ�
\param	IN LPVOID szStrBuf: ������ ���ڿ� ����
\return	void
\sa		AnsiToWide(), WideToAnsi()
*/
void		FreeString(LPVOID szStrBuf)
{
	if( szStrBuf )	delete szStrBuf;
}
YAGENT_COMMON_END
