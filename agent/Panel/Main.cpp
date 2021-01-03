// Panel.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#define USE_ORANGE  1
#define USE_SCITER  0

#include "framework.h"

CAppLog     g_log(L"user.log");

bool    IsExecutingInWow64()
{
#ifndef _WIN64
    static BOOLEAN valid = FALSE;
    static BOOLEAN isWow64;

    if (!valid)
    {
        PhGetProcessIsWow64(NtCurrentProcess(), &isWow64);
        MemoryBarrier();
        valid = TRUE;
    }

    return isWow64;
#else
    return FALSE;
#endif
}



#if 1 == USE_ORANGE

int APIENTRY CreateMainWindow(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR    lpCmdLine,
    int       nCmdShow);


int APIENTRY wWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR    lpCmdLine,
    int       nCmdShow)
{
    return CreateMainWindow(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

#endif

#if 0 == USE_ORANGE && 1 == USE_SCITER
#include "sciter-x-window.hpp"
#include "sciter-sdk\include\sciter-win-main.cpp"
static RECT wrc = { 100, 100, 400, 400 };

class frame: public sciter::window {
public:
    frame() : window(SW_TITLEBAR | SW_RESIZEABLE | SW_CONTROLS | SW_MAIN | SW_ENABLE_DEBUG) {}

    // passport - lists native functions and properties exposed to script:
    SOM_PASSPORT_BEGIN(frame)
        SOM_FUNCS(
            SOM_FUNC(nativeMessage),
            SOM_FUNC(GetProcess)
        )
    SOM_PASSPORT_END

    // function expsed to script:
    sciter::string  nativeMessage() { return WSTR("ORANGE"); }
    std::vector<sciter::value>  GetProcess() {
        std::vector<sciter::value>  val;

        CIPc    client;
        HANDLE  hClient;

        if( hClient = client.Connect(AGENT_PIPE_NAME, __FUNCTION__) ) {
            
            g_log.Log("%-32s connected", __FUNCTION__);
            IPCHeader   header  = {IPCJson, };

            Json::Value doc;

            doc["command"]  = "ProcessList";

            std::string					str;
            Json::StreamWriterBuilder	builder;
            builder["indentation"]	= "";
            str	= Json::writeString(builder, doc);
            header.dwSize   = (DWORD)(str.length() + 1);

            DWORD   dwBytes = 0;

            if( client.Request(__FUNCTION__, hClient, (PVOID)str.c_str(), (DWORD)str.length()+1,
                [&](PVOID pResponseData, DWORD dwResponseSize) {
                    g_log.Log("%-32s response %d bytes", __FUNCTION__, dwResponseSize);
                    g_log.Log("%s", pResponseData);
                    Json::CharReaderBuilder	builder;
                    Json::CharReader		*reader	= NULL;
                    Json::Value				res;
                    std::string				errors;

                    try {					
                        reader	= builder.newCharReader();
                        if( reader->parse((PCSTR)pResponseData, (PCSTR)pResponseData + dwResponseSize, &res, &errors) ) {
                            Json::Value &ProcessList    = res["ProcessList"];
                            for( auto t : ProcessList) {
                                sciter::value   item;
                                item.clear();
                                item.set_item("Count", t["Count"].asInt());
                                item.set_item("ProcName", __utf16(t["ProcName"].asCString()));
                                item.set_item("ProcPath", __utf16(t["ProcPath"].asCString()));
                                val.push_back(item);
                                g_log.Log("%8d %ws", t["Count"].asInt(), __utf16(t["ProcName"].asCString()));
                            }
                        }
                    }
                    catch( std::exception & e) {
                        g_log.Log("%-32s exception %s", __FUNCTION__, e.what());
                    }
                }) ) {



            }
            client.Disconnect(hClient, __FUNCTION__);
        }
        else {
            MessageBox(NULL, L"IPC not connected", L"", 0);
        }
        return val;
    }
};

//#include "resources.cpp" // packed /res/ folder

int uimain(std::function<int()> run) {

    //sciter::debug_output_console console; - uncomment it if you will need console window
    SciterSetOption(NULL, SCITER_SET_DEBUG_MODE, TRUE);
    //sciter::archive::instance().open(aux::elements_of(resources)); // bind resources[] (defined in "resources.cpp") with the archive

    frame *pwin = new frame();
    WCHAR   szResPath[AGENT_PATH_SIZE]  = L"";
    YAgent::GetModulePath(szResPath, sizeof(szResPath));
    StringCbCat(szResPath, sizeof(szResPath), L"\\res\\index.htm");
    // note: this:://app URL is dedicated to the sciter::archive content associated with the application
    //pwin->load(WSTR("this://app/index.html"));
    pwin->load(szResPath);
    pwin->expand();
    return run();
}
#endif