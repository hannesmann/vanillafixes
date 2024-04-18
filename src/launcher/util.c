#include <shlwapi.h>

#include <stdio.h>
#include <stdlib.h>

#include "cmdline.c"

// Function to concatenate the module directory with the relative file path.
LPWSTR UtilGetPath(LPCWSTR pModuleDirectory, LPCWSTR pRelativeFile) {
	int characters = wcslen(pModuleDirectory) + wcslen(pRelativeFile) + 3; // Add 3 to account for the '\\' and the null terminator
	LPWSTR pBuffer = HeapAlloc(GetProcessHeap(), 0, characters * sizeof(WCHAR));

	if(pBuffer == NULL) {
		return NULL;
	}

	swprintf(pBuffer, characters, L"%ls\\%ls", pModuleDirectory, pRelativeFile);

	return pBuffer; // Caller is responsible for freeing this memory
}

// Function to get the command line for WoW executable
LPWSTR UtilGetWowCmdLine(LPCWSTR pDefaultWowExePath) {
	VF_CMDLINE_PARSE_DATA data = { 0 };
	data.pWowExePath = pDefaultWowExePath;

	CmdLineParse(__argc, __wargv, &data);
	return CmdLineFormat(&data); // Caller is responsible for freeing this memory
}

// Function to get the last error message
LPWSTR UtilGetLastError() {
	LPWSTR pErrorStr = NULL;
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;

	if(FormatMessage(flags, NULL, GetLastError(), 0, (LPWSTR)&pErrorStr, 0, NULL)) {
		return pErrorStr; // Caller is responsible for freeing this memory
	}

	return L"Unknown error"; // Return "Unknown error" if FormatMessage fails
}
