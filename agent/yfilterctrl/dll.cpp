// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

HMODULE	g_hModule;
BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	UNREFERENCED_PARAMETER(hModule);
	UNREFERENCED_PARAMETER(ul_reason_for_call);
	UNREFERENCED_PARAMETER(lpReserved);
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
#ifdef _DEBUG
			//_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
#else

#endif
			g_hModule = (HMODULE)hModule;
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
LONG __stdcall __ExceptionHandler(struct _EXCEPTION_POINTERS * pExceptionInfo)
{
	UNREFERENCED_PARAMETER(pExceptionInfo);
	TCHAR	szPath[AGENT_PATH_SIZE] = _T("");
	::GetModuleFileName((HINSTANCE)g_hModule, szPath, sizeof(szPath));
	return EXCEPTION_EXECUTE_HANDLER;
}
IFilterCtrl *	__stdcall CreateInstance_CFilterCtrlImpl(IN LPVOID p1, LPVOID p2)
{
	UNREFERENCED_PARAMETER(p1);
	UNREFERENCED_PARAMETER(p2);
	IFilterCtrl *		p		= NULL;
	CFilterCtrlImpl	*	pClass	= new CFilterCtrlImpl();
	if( pClass )
		p	= dynamic_cast<IFilterCtrl *>(pClass);
	return p;
}