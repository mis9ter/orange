// header.h: 표준 시스템 포함 파일
// 또는 프로젝트 특정 포함 파일이 들어 있는 포함 파일입니다.
//

#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
// Windows 헤더 파일
#include "phapp.h"
#include <windows.h>
#include <shellapi.h>  
// C 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <cguid.h>
#include "resource.h"
#include "yagent.h"
#include "yagent.common.h"
#include "CIpc.h"

#include ".\resource.h"

#include "Panel.h"
#include "json\json.h"
#include "ProcessHacker\ProcessHacker.h"
#include "phconfig.h"
#include "mainwnd.h"
#include "emenu.h"