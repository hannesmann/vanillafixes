#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#define AssertMessageBox(condition, message) \
	if(!(condition)) { \
		return MessageBox(NULL, message, L"VanillaFixes", MB_OK | MB_ICONERROR); \
	}