#pragma once

#include <windows.h>

typedef struct {
	// Additional DLLs that the launcher should load
	LPWSTR *pAdditionalDLLs;
	// Number of arguments in pAdditionalDLLs
	int nAdditionalDLLs;
	// If the list of additional DLLs was updated since last launch
	BOOL listModified;
} VF_DLL_LIST_PARSE_DATA, *PVF_DLL_LIST_PARSE_DATA;

// Parse dlls.txt into a list, excluding any DLLs that are not found in the game directory
void LoaderParseConfig(LPCWSTR pModulePath, LPCWSTR pConfigPath, PVF_DLL_LIST_PARSE_DATA pOutput);

// Update dlls.txt.cache
void LoaderUpdateCacheState(LPCWSTR pConfigPath, PVF_DLL_LIST_PARSE_DATA pOutput);

// Convert VF_DLL_LIST_PARSE_DATA to a list of DLLs
LPWSTR LoaderListText(LPCWSTR pModulePath, PVF_DLL_LIST_PARSE_DATA pInput);

// Function to load a DLL into the game process
int RemoteLoadLibrary(LPWSTR pDllPath, HANDLE hTargetProcess);
