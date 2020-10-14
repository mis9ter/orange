#include "framework.h"

void    RunInService(CAgent * pAgent)
{
    DWORD   dwSessionId = YAgent::GetCurrentSessionId();

    //Log("%s dwSessionId=%d", __FUNCTION__, dwSessionId);
    if( 0 == dwSessionId ) {
        //  서비스로 실행.

    }
    else {
        //  일반 실행 파일로 실행.

    }
}