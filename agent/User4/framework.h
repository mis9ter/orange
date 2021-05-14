// header.h: 표준 시스템 포함 파일
// 또는 프로젝트 특정 포함 파일이 들어 있는 포함 파일입니다.
//

#pragma once

#define _CRT_SECURE_NO_WARNINGS

#ifndef __FUNCTION__
#define __FUNCTION__	__func__
#endif

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
// Windows 헤더 파일
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS

#include <winternl.h>
#include <ntstatus.h>
//#include <ntxcapi.h>
//출처: https://cheesehack.tistory.com/88 [HIGH ABOVE MORE]

#include <windowsx.h>
// C 런타임 헤더 파일입니다.

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <cguid.h>
#include <assert.h>

#include "User4.h"
#include "CMainDialog.h"