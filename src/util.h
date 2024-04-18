#pragma once

#include <windows.h>
#include <stdio.h>

#define AssertMessageBox(condition, message) \
	if(!(condition)) { \
		return MessageBox(NULL, message, L"VanillaFixes", MB_OK | MB_ICONERROR); \
	}

static WCHAR msgBuffer[512] = {0};

#define DebugOutput(...) \
	swprintf(msgBuffer, 512, __VA_ARGS__); \
	OutputDebugString(msgBuffer);

typedef struct {
	// Stores path to DLL for LoadLibrary
	WCHAR pDllPath[MAX_PATH];
	// If VfPatcher.dll should initialize Nampower before exiting
	BOOL initNamPower;
} VF_SHARED_DATA, *PVF_SHARED_DATA;

// Reserve some space in the game process to store a pointer
// This address is never used by the game when VanillaFixes is active
static PVF_SHARED_DATA* g_pSharedData = (PVF_SHARED_DATA*)0x00884060;

// Get the absolute path of a file in the game directory
static LPWSTR UtilGetPath(LPCWSTR pModuleDirectory, LPCWSTR pRelativeFile) {
	// Add 3 to account for the '\\' and the null terminator
	int characters = wcslen(pModuleDirectory) + wcslen(pRelativeFile) + 3;
	LPWSTR pBuffer = malloc(characters * sizeof(WCHAR));

	if(!pBuffer) {
		return NULL;
	}

	swprintf(pBuffer, characters, L"%ls\\%ls", pModuleDirectory, pRelativeFile);
	return pBuffer;
}

// Utility function to get the last error message
static LPWSTR UtilGetLastError() {
	LPWSTR pErrorStr = NULL;
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;

	if(FormatMessage(flags, NULL, GetLastError(), 0, (LPWSTR)&pErrorStr, 0, NULL)) {
		return pErrorStr;
	}

	return L"Unknown error";
}