#include "framework.h"

void    RunInService(CAgent * pAgent)
{
    DWORD   dwSessionId = YAgent::GetCurrentSessionId();

    //Log("%s dwSessionId=%d", __FUNCTION__, dwSessionId);
    if( 0 == dwSessionId ) {
        //  ���񽺷� ����.

    }
    else {
        //  �Ϲ� ���� ���Ϸ� ����.

    }
}