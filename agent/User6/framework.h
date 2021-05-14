#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
// Windows 헤더 파일
#include <windows.h>
// C 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <wrl.h>
#include <wil/com.h>
using namespace Microsoft::WRL;

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>

#include "Resource.h"
// include WebView2 header
#include "WebView2.h"
#include "CheckFailure.h"

#include "CAppRegistry.h"
#include "CAppLog.h"
#include "yagent.common.h"
#include "Chttp.h"


