#include "pch.h"

#define SZ_MAX_DEST_SIZE	(64*1024)

YAGENT_COMMON_BEGIN
LPTSTR	ReplaceString(IN LPTSTR lpSrc, IN DWORD dwSrcSize, 
								IN LPCTSTR lpKey, IN LPCTSTR lpRep)
{
	int		i, j, nSrcLen, nKeyLen, nRepLen;
	TCHAR	szDest[SZ_MAX_DEST_SIZE]	= _T("");

	nSrcLen	= lstrlen(lpSrc);
	nKeyLen	= lstrlen(lpKey);
	nRepLen	= lstrlen(lpRep);

	if(SZ_MAX_DEST_SIZE < dwSrcSize)	return NULL;

	::ZeroMemory(szDest, sizeof(szDest));
	for( i = 0, j = 0 ; (i < nSrcLen) && (j < SZ_MAX_DEST_SIZE); )
	{
		if( !CompareBeginningString(lpSrc+i, lpKey) )
		{
			::StringCbCopy(szDest+j, sizeof(szDest)-j*sizeof(TCHAR), lpRep);
			i	+= nKeyLen;
			j	+= nRepLen;
		}
		else
		{
			szDest[j++]	= lpSrc[i++];
		}
	}
	szDest[j]	= NULL;
	::StringCbCopy(lpSrc, dwSrcSize, szDest);
	return lpSrc;
}
YAGENT_COMMON_END