#pragma once

#define WIN32_LEAN_AND_MEAN

#ifdef _DEBUG
#pragma comment (lib, "Debug\\ServerCore.lib")
#else
#pragma comment (lib, "Release\\ServerCore.lib")
#endif

// 여기에 미리 컴파일하려는 헤더 추가
#include "CorePch.h"
