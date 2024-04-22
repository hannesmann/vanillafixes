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

typedef enum {
	VF_LAUNCHER_FLAG_NONE = 0,
	// Indicates that VfPatcher is responsible for initializing additional DLLs
	VF_LAUNCHER_FLAG_INIT_DLLS = (1 << 31)
} VF_LAUNCHER_FLAGS, *PVF_LAUNCHER_FLAGS;

// Reserve space in the game process to store flags from the launcher
// This address is never used by the game when VanillaFixes is active
static PVF_LAUNCHER_FLAGS g_pLauncherFlags = (PVF_LAUNCHER_FLAGS)0x00884060;

// Parse dlls.txt into a list, excluding any DLLs that are not found in the game directory
void LoaderParseConfig(LPCWSTR pModulePath, LPCWSTR pConfigPath, PVF_DLL_LIST_PARSE_DATA pOutput);

// Update dlls.txt.cache
void LoaderUpdateCacheState(LPCWSTR pConfigPath, PVF_DLL_LIST_PARSE_DATA pOutput);

// Convert VF_DLL_LIST_PARSE_DATA to a list of DLLs
LPWSTR LoaderListText(LPCWSTR pModulePath, PVF_DLL_LIST_PARSE_DATA pInput);

// Function to load a DLL into the game process
int RemoteLoadLibrary(LPWSTR pDllPath, HANDLE hTargetProcess);

// Function to set launcher flags in game process
int RemoteSetFlags(VF_LAUNCHER_FLAGS newFlags, HANDLE hTargetProcess);
