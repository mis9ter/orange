#include "pch.h"

YAGENT_COMMON_BEGIN

/*
	2006.06.13
	조인중: 이따위로 함수명을 붙이면 나중에 사용하는 사람이 헷갈리는 거 아니냐?
	김범진: 그렇다 -- (씨봉아, 졸라 미안하다..);

	이 함수에 걸맞는 이름을 부여하고 점차적으로 걸맞는 이름으로의 사용을 권장하자. 

	AhnCompareString 안의 내용을 AhnIsStartedSameNoCase로 변경하고,
	AhnCompareString에서 AhnIsStartedSameNoCase 를 호출하도록 했다.

*/

int		IsStartedSameNoCase(IN LPCTSTR lpStr1, IN LPCTSTR lpStr2)
{
	while( *lpStr1 && *lpStr2 )
	{
		if( _totupper(*lpStr1) != _totupper(*lpStr2) )
		{
			return (_totupper(*lpStr1) - _totupper(*lpStr2) );
		}
		
		lpStr1++;
		lpStr2++;
	}
	return 0;	
}
///////////////////////////////////////////////////////////////////////////////
// 이 함수는 두 문자열중 짧은 하나의 문자열 동안 서로 같은가 비교하는 것이다.
// lstrcmp와는 다른 함수이므로 사용에 주의하기 바람. 대소문자 구분도 안함.
// KimPro
// 2011/9/21 CompareString => CompareBeginningString으로 이름 변경함.
// CompareString은 WIN32 API에 동일하게 제공되는 함수명이다.
///////////////////////////////////////////////////////////////////////////////
int		CompareBeginningString(IN LPCTSTR lpStr1, IN LPCTSTR lpStr2)
{
	// 이 함수보다는 AhnIsStartedSameNoCase 사용을 권장합니다.
	return IsStartedSameNoCase(lpStr1, lpStr2);
}

YAGENT_COMMON_END