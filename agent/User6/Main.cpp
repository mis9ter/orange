#include "pch.h"
#include "CAppLog.h"

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
) {
	CWebApp		app;

	if( app.Initialize(hInstance, nCmdShow)) {
		app.Run();
	}	
	app.Destroy();
	return 0;
}