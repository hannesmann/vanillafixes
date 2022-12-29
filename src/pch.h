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

typedef struct {
	/* Stores path to DLL for LoadLibrary */
	WCHAR pDllPath[MAX_PATH];
	/* If VfPatcher.dll should initialize Nampower before exiting */
	BOOL initNamPower;
} VF_SHARED_DATA, *PVF_SHARED_DATA;

/* This address is never used by the game when VanillaFixes is active */
static PVF_SHARED_DATA* g_pSharedData = (PVF_SHARED_DATA)0x00884060;