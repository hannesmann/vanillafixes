#include <windows.h>
#include <shlwapi.h>

#include "macros.h"

#include "loader.h"
#include "textfile.h"

static void AppendArg(LPWSTR pArg, PVF_DLL_LIST_PARSE_DATA pOutput) {
	int argsBufSize = (pOutput->nAdditionalDLLs + 1) * sizeof(LPWSTR);
	pOutput->pAdditionalDLLs = realloc(pOutput->pAdditionalDLLs, argsBufSize);
	pOutput->pAdditionalDLLs[pOutput->nAdditionalDLLs++] = pArg;
}

// Normalize a path relative to pBase
static LPWSTR NormalizePath(LPCWSTR pBase, LPCWSTR pPath) {
	LPWSTR pAbsolutePath = calloc(MAX_PATH, sizeof(WCHAR));
	// If the path is relative, assume the file exists in pBase (and NOT the current directory)
	if(PathIsRelative(pPath)) {
		PathCombine(pAbsolutePath, pBase, pPath);
	}
	else {
		wcscpy(pAbsolutePath, pPath);
	}

	// Normalize path separators
	for(int i = 0; i < MAX_PATH && pAbsolutePath[i]; i++) {
		if(pAbsolutePath[i] == L'/') {
			pAbsolutePath[i] = L'\\';
		}
	}

	// Remove navigation elements such as "." and ".."
	LPWSTR pCanonicalizedPath = calloc(MAX_PATH, sizeof(WCHAR));
	PathCanonicalize(pCanonicalizedPath, pAbsolutePath);
	free(pAbsolutePath);

	// Return the absolute canonicalized path
	return pCanonicalizedPath;
}

void LoaderParseConfig(LPCWSTR pModulePath, LPCWSTR pConfigPath, PVF_DLL_LIST_PARSE_DATA pOutput) {
	// Store a cache of the DLLs that have been accepted and loaded by the user in the past
	LPWSTR pConfigCachePath = calloc(MAX_PATH, sizeof(WCHAR));
	swprintf(pConfigCachePath, MAX_PATH, L"%ls.cache", pConfigPath);

	// If we have a list of DLLs that should be loaded
	if(GetFileAttributes(pConfigPath) != INVALID_FILE_ATTRIBUTES) {
		int nLines = 0;
		int nCachedLines = 0;

		// Read dlls.txt and dlls.txt.cache
		LPWSTR* pLines = FromTextFile(pConfigPath, &nLines, TRUE);
		LPWSTR* pCachedLines = FromTextFile(pConfigCachePath, &nCachedLines, FALSE);
		if(pLines) {
			for(int i = 0; i < nLines; i++) {
				// Normalize the path entered by the user
				LPWSTR pNormalizedPath = NormalizePath(pModulePath, pLines[i]);
				free(pLines[i]);

				// If the file exists, consider it as a valid candidate and save it in the cache
				if(GetFileAttributes(pNormalizedPath) != INVALID_FILE_ATTRIBUTES) {
					AppendArg(pNormalizedPath, pOutput);
				}
				else {
					free(pNormalizedPath);
				}
			}

			free(pLines);
		}

		if(pCachedLines) {
			// Does the list length match the cached length?
			if(pOutput->nAdditionalDLLs == nCachedLines) {
				for(int i = 0; i < pOutput->nAdditionalDLLs; i++) {
					// Do list items match and are in the same order?
					if(StrCmpI(pOutput->pAdditionalDLLs[i], pCachedLines[i])) {
						pOutput->listModified = TRUE;
						break;
					}
				}
			}
			// If the list length has changed, the list is modified
			else {
				pOutput->listModified = TRUE;
			}

			for(int i = 0; i < nCachedLines; i++) {
				free(pCachedLines[i]);
			}
			free(pCachedLines);
		}
		// If no cache was present, the list is new/modified
		else {
			pOutput->listModified = TRUE;
		}
	}

	free(pConfigCachePath);
}

void LoaderUpdateCacheState(LPCWSTR pConfigPath, PVF_DLL_LIST_PARSE_DATA pOutput) {
	LPWSTR pConfigCachePath = calloc(MAX_PATH, sizeof(WCHAR));
	swprintf(pConfigCachePath, MAX_PATH, L"%ls.cache", pConfigPath);

	// If we have active DLLs, save them to a text file
	if(pOutput->nAdditionalDLLs) {
		ToTextFile(pConfigCachePath, pOutput->pAdditionalDLLs, pOutput->nAdditionalDLLs);
	}
	// If we have no additional DLLs, but an old "dlls.txt.cache" sitting around
	else if(GetFileAttributes(pConfigCachePath) != INVALID_FILE_ATTRIBUTES) {
		DeleteFile(pConfigCachePath);
	}

	free(pConfigCachePath);
}

LPWSTR LoaderListText(LPCWSTR pModulePath, PVF_DLL_LIST_PARSE_DATA pInput) {
	// Determine the number of required characters for the list
	int requiredChars = 1;
	for(int i = 0; i < pInput->nAdditionalDLLs; i++) {
		BOOL useNewLine = (i < pInput->nAdditionalDLLs - 1);
		requiredChars += wcslen(pInput->pAdditionalDLLs[i]) + (useNewLine ? 2 : 0);
	}

	// Pointer to start of text
	LPWSTR pText = calloc(requiredChars, sizeof(WCHAR));
	// Pointer to current line
	LPWSTR pCurrent = pText;

	for(int i = 0; i < pInput->nAdditionalDLLs; i++) {
		// Append CRLF unless this is the last line
		BOOL useNewLine = (i < pInput->nAdditionalDLLs - 1);
		LPWSTR pDllName = pInput->pAdditionalDLLs[i];
		// If the start of the string contains the module path, truncate it
		if(StrStrI(pDllName, pModulePath) == pDllName) {
			pDllName += wcslen(pModulePath) + 1;
		}

		wcscpy(pCurrent, pDllName);
		pCurrent += wcslen(pDllName);

		if(useNewLine) {
			*pCurrent = L'\r';
			pCurrent++;
			*pCurrent = L'\n';
			pCurrent++;
		}
	}

	return pText;
}

int RemoteLoadLibrary(LPWSTR pDllPath, HANDLE hTargetProcess) {
	// Allocate memory for DLL path
	int dllPathLen = (wcslen(pDllPath) + 1) * sizeof(WCHAR);
	LPVOID pRemoteDllPath = VirtualAllocEx(
		hTargetProcess,
		NULL,
		dllPathLen,
		MEM_RESERVE | MEM_COMMIT,
		PAGE_READWRITE
	);

	// Copy the DLL path to the target process
	BOOL success = WriteProcessMemory(hTargetProcess, pRemoteDllPath, pDllPath, dllPathLen, NULL);
	AssertMessageBoxF(success, L"Failed to write process memory (pDllPath)");

	// Create a new thread in the target process that will load the DLL
	HANDLE hThread =
		CreateRemoteThread(hTargetProcess, NULL, 0, (LPTHREAD_START_ROUTINE)&LoadLibraryW, pRemoteDllPath, 0, NULL);
	AssertMessageBoxF(hThread, L"Failed to create remote thread");

	// Wait for the created thread to terminate
	WaitForSingleObject(hThread, 10000);

	DWORD threadResult = 0;
	// Retrieve the termination status of the thread
	GetExitCodeThread(hThread, &threadResult);

	// Close the thread handle
	CloseHandle(hThread);

	// Assert that the thread terminated successfully and the DLL was loaded
	AssertMessageBoxF(threadResult && threadResult != STILL_ACTIVE,
		L"DLL entry point returned an error (%ld).\r\n\r\n"
		L"Make sure you have a compatible game client.",
		threadResult
	);

	// Clean up DLL path in target process
	AssertMessageBoxF(VirtualFreeEx(hTargetProcess, pRemoteDllPath, 0, MEM_RELEASE),
		L"Failed to clean up process memory (pDllPath)");

	return 0;
}

int RemoteSetFlags(VF_LAUNCHER_FLAGS newFlags, HANDLE hTargetProcess) {
	BOOL success = WriteProcessMemory(hTargetProcess, g_pLauncherFlags, &newFlags, sizeof(VF_LAUNCHER_FLAGS), NULL);
	// Assert that flags were successfully written to the target process
	AssertMessageBoxF(success, L"Failed to write process memory (newFlags)");
	return 0;
}
