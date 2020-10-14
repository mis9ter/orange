#include "framework.h"
#include "stdio.h"

void    RunInConsole(CAgent * pAgent)
{
    /*
    if (agent.Start(NULL, [&](HANDLE hShutdown, LPVOID pCallbackPtr) {
    bool    bRun    = true;    
    while( bRun )
    {
    if( WAIT_OBJECT_0 == WaitForSingleObject(hShutdown, 0) )
    break;
    printf("command(q,i,u,s,t):");
    gets_s(szCmd);
    if (1 == lstrlenA(szCmd))
    {
    switch (szCmd[0])
    {
    case 's':
    puts("  -- start");
    agent.Start(NULL, AgentRunLoop);
    break;
    case 't':
    puts("  -- shutdown");
    agent.Shutdown();
    break;
    case 'i':
    puts("  -- install");
    agent.Install();
    break;
    case 'u':
    puts("  -- uninstall");
    agent.Uninstall();
    break;
    case 'q':
    puts("  -- quit");
    bRun    = false;
    break;
    case '\r':
    case '\n':
    break;
    default:
    puts("  -- unknown command");
    break;
    }
    }
    //printf("%-30s pCallbackPtr=%p\n", __FUNCTION__, pCallbackPtr);
    }
    }))
    {
    agent.RunLoop();
    }
    */
}
