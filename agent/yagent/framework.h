// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

//#define _CONSOLE
//#define _WINDOWS

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <iostream>

#include "resource.h"
#include "CAgent.h"
//	https://devblogs.microsoft.com/cppblog/making-cpp-exception-handling-smaller-x64/

/*
kernel32.lib; 
user32.lib; 
gdi32.lib; 
winspool.lib; 
comdlg32.lib; 
advapi32.lib; 
shell32.lib; 
ole32.lib; 
oleaut32.lib; 
uuid.lib; 
odbc32.lib; odbccp32.lib; % (AdditionalDependencies)
*/