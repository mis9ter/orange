#include "pch.h"

YAGENT_COMMON_BEGIN

LPCTSTR		GetFileFullExt(IN LPCTSTR pFilePath)
{
	LPCTSTR	pDot		= NULL;
	LPCTSTR	pBackSlash	= _tcsrchr(pFilePath, _T('\\'));
	LPCTSTR	pSlash		= _tcsrchr(pFilePath, _T('/'));
	LPCTSTR	pStart		= NULL;

	pStart	= pBackSlash? pBackSlash : pSlash;
	if( NULL == pStart )
	{
		pStart	= pFilePath;
	}

	pDot	= _tcschr(pStart, _T('.'));

	return pDot? pDot + 1 : pDot;
}

LPCTSTR		GetFileExt(IN LPCTSTR pFilePath)
{
	LPCTSTR	pDot		= NULL;
	LPCTSTR	pBackSlash	= _tcsrchr(pFilePath, _T('\\'));
	LPCTSTR	pSlash		= _tcsrchr(pFilePath, _T('/'));
	LPCTSTR	pStart		= NULL;

	pStart	= pBackSlash? pBackSlash : pSlash;
	if( NULL == pStart )
	{
		pStart	= pFilePath;
	}

	pDot	= _tcsrchr(pStart, _T('.'));

	return pDot? pDot + 1 : pDot;
}

YAGENT_COMMON_END