#include <shlwapi.h>

#include <stdio.h>
#include <stdlib.h>

#include "cmdline.c"


// Function to concatenate the module directory with the relative file path.
LPWSTR UtilGetPath(LPCWSTR pModuleDirectory, LPCWSTR pRelativeFile)
{
	int characters = wcslen(pModuleDirectory) + wcslen(pRelativeFile) + 3; // Add 3 to account for the '\\' and the null terminator
	LPWSTR pBuffer = HeapAlloc(GetProcessHeap(), 0, characters * sizeof(WCHAR));

	if (pBuffer == NULL) // Check if memory allocation was successful
	{
		return NULL;
	}

	swprintf(pBuffer, characters, L"%ls\\%ls", pModuleDirectory, pRelativeFile);

	return pBuffer; // Caller is responsible for freeing this memory
}

// Function to get the command line for WoW executable
LPWSTR UtilGetWowCmdLine(LPCWSTR pDefaultWowExePath)
{
	VF_CMDLINE_PARSE_DATA data = { 0 };
	data.pWowExePath = pDefaultWowExePath;

	CmdLineParse(__argc, __wargv, &data);
	return CmdLineFormat(&data); // Caller is responsible for freeing this memory
}

// Function to set default config value. Not used currently, but could be useful for overriding default settings
int UtilSetDefaultConfigValue(LPCWSTR pWowDirectory, LPCSTR pKey, LPCSTR pValue)
{
	LPWSTR pWowConfigDirectory = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
	swprintf(pWowConfigDirectory, MAX_PATH, L"%ls\\WTF", pWowDirectory);

	// We don't care if this function fails, as long as the directory exists
	CreateDirectory(pWowConfigDirectory, NULL);

	LPWSTR pWowConfigPath = pWowConfigDirectory;
	swprintf(pWowConfigPath, MAX_PATH, L"%ls\\WTF\\Config.wtf", pWowDirectory);

	HANDLE hWowConfigFile = CreateFile(pWowConfigPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	AssertMessageBox(hWowConfigFile, L"Failed to open WTF\\Config.wtf");

	LPSTR pBuffer = HeapAlloc(GetProcessHeap(), 0, 1024);
	DWORD bufferSize = 1024;

	ZeroMemory(pBuffer, 1024);

	while (TRUE)
	{
		DWORD numberOfBytesRead = 0;
		BOOL readResult = ReadFile(hWowConfigFile, pBuffer + bufferSize - 1024, 1024, &numberOfBytesRead, NULL);
		if (!readResult)
		{
			HeapFree(GetProcessHeap(), 0, pBuffer); // Avoid memory leak
			return -1;
		}

		if (numberOfBytesRead > 0)
		{
			bufferSize += 1024;

			LPSTR pNewBuffer = HeapReAlloc(GetProcessHeap(), 0, pBuffer, bufferSize);

			if (pNewBuffer == NULL)
			{
				HeapFree(GetProcessHeap(), 0, pBuffer); // Avoid memory leak
				return -1;
			}

			pBuffer = pNewBuffer;
			ZeroMemory(pBuffer + bufferSize - 1024, 1024);
		}
		else
		{
			break;
		}
	}

	LPSTR pSearch = HeapAlloc(GetProcessHeap(), 0, strlen("SET ") + strlen(pKey) + 1);
	if (pSearch == NULL)
	{
		// Handle the error, probably by terminating the function
		HeapFree(GetProcessHeap(), 0, pBuffer); // Avoid memory leak
		return -1;
	}

	sprintf(pSearch, "SET %s", pKey);

	if (!StrStrIA(pBuffer, pSearch))
	{
		DWORD writeBufferLen = strlen("\r\n \"\"\r\n") + strlen(pSearch) + strlen(pValue) + 1;
		LPSTR pWriteBuffer = HeapAlloc(GetProcessHeap(), 0, writeBufferLen);

		if (pWriteBuffer == NULL)
		{
			HeapFree(GetProcessHeap(), 0, pBuffer); // Avoid memory leak
			HeapFree(GetProcessHeap(), 0, pSearch); // Avoid memory leak
			return -1;
		}

		BOOL insertNewline = *pBuffer && pBuffer[strlen(pBuffer) - 1] != '\n';
		sprintf(pWriteBuffer, "%s%s \"%s\"\r\n", insertNewline ? "\r\n" : "", pSearch, pValue);

		BOOL writeError = WriteFile(hWowConfigFile, pWriteBuffer, strlen(pWriteBuffer), NULL, NULL);
		AssertMessageBox(!writeError, L"Failed to write to WTF\\Config.wtf");

		HeapFree(GetProcessHeap(), 0, pWriteBuffer); // Free memory
	}

	HeapFree(GetProcessHeap(), 0, pBuffer); // Free memory
	HeapFree(GetProcessHeap(), 0, pSearch); // Free memory

	return 0;
}

// Function to get the last error message
LPWSTR UtilGetLastError()
{
	LPWSTR pErrorStr = NULL;
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;

	if (FormatMessage(flags, NULL, GetLastError(), 0, (LPWSTR)&pErrorStr, 0, NULL))
	{
		return pErrorStr; // Caller is responsible for freeing this memory
	}

	return L"Unknown error"; // Return "Unknown error" if FormatMessage fails
}
