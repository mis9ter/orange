#include "pch.h"

YAGENT_COMMON_BEGIN

LPCTSTR	GetURLPath(IN LPCTSTR pFilePath, OUT LPTSTR lpPath, IN DWORD dwSize)
{
	LPCTSTR	pExt = GetFileExt(pFilePath);
	if (NULL == pExt || _tcsicmp(pExt, _T("url")))
	{
		return NULL;
	}
	::GetPrivateProfileString(_T("InternetShortcut"), _T("URL"), _T(""), lpPath, dwSize / sizeof(TCHAR), pFilePath);
	return lpPath;
}
bool	GetRealPath(IN LPCTSTR pLinkPath, OUT LPTSTR pRealPath, IN DWORD dwSize)
{
	bool			bRet	= false;
	TCHAR			szLinkPath[LBUFSIZE], szBuf[LBUFSIZE];
	LPTSTR			pToken, pExt, pContext;
	WIN32_FIND_DATA	fd;
	bool			bLink;

	if( NULL == pRealPath || 0 == dwSize )
	{
		return false;
	}

	::StringCbCopy(szLinkPath, sizeof(szLinkPath), pLinkPath);
	*pRealPath	= NULL;

	for(	pToken = _tcstok_s(szLinkPath, _T("\\"), &pContext) ; pToken && *pToken ; 
			pToken = _tcstok_s(NULL, _T("\\"), &pContext) )
	{

		pExt	= _tcsrchr(pToken, _T('.'));
		bLink	= false;

		if( NULL == pExt )
		{
			
		}
		else if( !_tcsicmp(pExt, _T(".lnk")) )
		{
			// 파일의 확장자 링크 파일
			::StringCbPrintf(szBuf, sizeof(szBuf), _T("%s\\%s"), pRealPath, pToken);
			if( bLink = GetLinkPath(szBuf, pRealPath, dwSize, &fd) )
			{
				//_log(_T("TRUE"));
				// 올바른 링크이다.
			}
			else
			{
				//_log(_T("FALSE"));
				// 잘못된 링크라면 그냥 일반 파일로 처리해야 한다.
				::StringCbCopy(pRealPath, dwSize, szBuf);
				bLink	= true;
			}
		}
		else if( !_tcsicmp(pExt, _T(".url")) )
		{
			::StringCbPrintf(szBuf, sizeof(szBuf), _T("%s\\%s"), pRealPath, pToken);
			if( GetURLPath(szBuf, pRealPath, dwSize) )
			{
				break;
			}
			else
			{
				::StringCbCopy(pRealPath, dwSize, szBuf);
			}
		}

		if( false == bLink )
		{
			if( NULL != *pRealPath )
			{
				::StringCbCat(pRealPath, dwSize, _T("\\"));
			}
			::StringCbCat(pRealPath, dwSize, pToken);
		}
	}
	return IsFileExist(pRealPath);
}
YAGENT_COMMON_END