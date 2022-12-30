#include <shlwapi.h>

#include <stdio.h>
#include <stdlib.h>

#include "cmdline.c"

LPWSTR UtilGetPath(LPCWSTR pModuleDirectory, LPCWSTR pRelativeFile) {
	int characters = wcslen(pModuleDirectory) + wcslen(pRelativeFile) + 2;
	LPWSTR pBuffer = HeapAlloc(GetProcessHeap(), 0, characters * sizeof(WCHAR));

	swprintf(pBuffer, characters, L"%ls\\%ls", pModuleDirectory, pRelativeFile);

	return pBuffer;
}

LPWSTR UtilGetWowCmdLine(LPCWSTR pDefaultWowExePath) {
	VF_CMDLINE_PARSE_DATA data = {0};
	data.pWowExePath = pDefaultWowExePath;

	CmdLineParse(__argc, __wargv, &data);
	return CmdLineFormat(&data);
}

/* Unused right now but it could make sense to override some default settings (sound channels, etc.) */
int UtilSetDefaultConfigValue(LPCWSTR pWowDirectory, LPCSTR pKey, LPCSTR pValue) {
	LPWSTR pWowConfigDirectory = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
	swprintf(pWowConfigDirectory, MAX_PATH, L"%ls\\WTF", pWowDirectory);

	/* We don't care if this function fails, as long as the directory exists */
	CreateDirectory(pWowConfigDirectory, NULL);

	LPWSTR pWowConfigPath = pWowConfigDirectory;
	swprintf(pWowConfigPath, MAX_PATH, L"%ls\\WTF\\Config.wtf", pWowDirectory);

	HANDLE hWowConfigFile =
		CreateFile(pWowConfigPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	AssertMessageBox(hWowConfigFile, L"Failed to open WTF\\Config.wtf");

	LPSTR pBuffer = HeapAlloc(GetProcessHeap(), 0, 1024);
	DWORD bufferSize = 1024;

	ZeroMemory(pBuffer, 1024);

	while(TRUE) {
		DWORD numberOfBytesRead = 0;
		ReadFile(hWowConfigFile, pBuffer + bufferSize - 1024, 1024, &numberOfBytesRead, NULL);

		if(numberOfBytesRead > 0) {
			bufferSize += 1024;
			pBuffer = HeapReAlloc(GetProcessHeap(), 0, pBuffer, bufferSize);
			ZeroMemory(pBuffer + bufferSize - 1024, 1024);
		}
		else {
			break;
		}
	}

	LPSTR pSearch = HeapAlloc(GetProcessHeap(), 0, strlen("SET ") + strlen(pKey) + 1);
	sprintf(pSearch, "SET %s", pKey);

	if(!StrStrIA(pBuffer, pSearch)) {
		DWORD writeBufferLen = strlen("\r\n \"\"\r\n") + strlen(pSearch) + strlen(pValue) + 1;
		LPSTR pWriteBuffer = HeapAlloc(GetProcessHeap(), 0, writeBufferLen);

		BOOL insertNewline = *pBuffer && pBuffer[strlen(pBuffer) - 1] != '\n';
		sprintf(pWriteBuffer, "%s%s \"%s\"\r\n", insertNewline ? "\r\n" : "", pSearch, pValue);

		BOOL writeError = WriteFile(hWowConfigFile, pWriteBuffer, strlen(pWriteBuffer), NULL, NULL);
		AssertMessageBox(hWowConfigFile, L"Failed to write to WTF\\Config.wtf");
	}

	CloseHandle(hWowConfigFile);

	return 0;
}

LPWSTR UtilGetLastError() {
	LPWSTR pErrorStr = NULL;
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;

	if(FormatMessage(flags, NULL, GetLastError(), 0, (LPWSTR)&pErrorStr, 0, NULL)) {
		return pErrorStr;
	}

	return L"Unknown error";
}