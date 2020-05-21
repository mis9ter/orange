/**
	\file	GetModulePathA.cpp
	\author	KimPro
	\date	2005.04.04

  */
#include "pch.h"
YAGENT_COMMON_BEGIN

/**	\brief	ÇöÀç È£ÃâµÈ ¸ðµâ(½ÇÇàÆÄÀÏ)ÀÇ °æ·Î¸¦ ¾ò´Â ÇÔ¼ö.

	\param	LPCTSTR	szPath	º¹»ç ´ë»ó ¹®ÀÚ¿­
	\param	DWORD	dwSize	º¹»ç °¡´ÉÇÑ ¹®ÀÚ¿­¯M ÃÖ´ë ±æÀÌ.
	\return	bool: ¼º°ø - TRUE, ½ÇÆÐ - FALSE


*/
bool	GetModulePath(OUT LPTSTR szPath, IN DWORD dwSize)
{
	return GetInstancePath(NULL, szPath, dwSize);
}

bool	GetInstancePath(IN HINSTANCE hInstance, OUT LPTSTR szPath, IN DWORD dwSize)
{
	LPTSTR	ps;
	TCHAR	szBuf[LBUFSIZE];

	if( ::GetModuleFileName(hInstance, szBuf, sizeof(szBuf)) )
	{
		ps	= _tcsrchr(szBuf, TEXT('\\'));
		if( ps )
		{
			*ps	= NULL;

			/*
				2013/05/05	kimpro
				µµ´ëÃ¼ ¾Æ·¡ Á¦ÀÏ Ã³À½ÀÌ ¿ª½½·¡½ÃÀÎ °æ¿ì ¿Ö +4ºÎÅÍ º¹»çÇßÀ»±î??
				
			if( _T('\\') == szBuf[0] )
			{
				::StringCbCopy(szPath, dwSize, szBuf + 4);
			}
			else
			{
				::StringCbCopy(szPath, dwSize, szBuf);
			}
			*/
			::StringCbCopy(szPath, dwSize, szBuf);

			return true;
		}
	}
	return false;
}
YAGENT_COMMON_END
