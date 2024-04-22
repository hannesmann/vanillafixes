#include <windows.h>
#include <shlwapi.h>
#include <stdlib.h>

#include "textfile.h"

// Read a text file into memory
static LPSTR ReadFileIntoMemory(HANDLE hFile) {
	DWORD readBufferSize = GetFileSize(hFile, NULL);
	// Ensure a valid string is always returned
	if(readBufferSize == INVALID_FILE_SIZE) {
		readBufferSize = 0;
	}
	LPSTR readBuffer = malloc(readBufferSize + 1);

	DWORD read = 0;
	BOOL readSuccess = ReadFile(hFile, readBuffer, readBufferSize, &read, NULL);

	if(readSuccess) {
		readBuffer[readBufferSize] = '\0';
	}
	else {
		// Zero length if the operation failed
		readBuffer[0] = '\0';
	}
	return readBuffer;
}

LPWSTR* FromTextFile(LPCWSTR pFileName, int* nLinesRead, BOOL ignoreComments) {
	// Only open if existing
	HANDLE hFile = CreateFile(pFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	// Read text file into memory
	LPSTR pContent = ReadFileIntoMemory(hFile);
	LPWSTR* pResult = NULL;
	*nLinesRead = 0;

	// Allocate a rolling buffer of lines
	LPSTR* pLines = calloc(4, sizeof(LPSTR));
	int pMaxLines = 4;

	LPSTR pCurrent = pContent;
	// Keep reading from the current position in the buffer until we reach NUL
	while(*pCurrent) {
		// Find the end of the current line
		LPSTR pEOL = strchr(pCurrent, '\n');
		// If we have a newline, we replace it with NUL to create a series of contiguous separate lines
		if(pEOL) {
			*pEOL = '\0';
		}

		// If line starts with '#', it's a comment
		BOOL isComment = pCurrent[0] == '#';
		// If line is zero-length or starts with a newline character, it's empty
		BOOL isEmptyLine = pCurrent[0] == '\r' || pCurrent[0] == '\n' || pCurrent[0] == '\0';

		if((!isComment || !ignoreComments) && !isEmptyLine) {
			// Reallocate the rolling buffer if we have too many lines
			if(*nLinesRead >= pMaxLines) {
				pMaxLines *= 2;
				pLines = realloc(pLines, pMaxLines * sizeof(LPSTR));
			}
			// Keep a reference to the current line in pLines
			// pLines[i] should not be freed manually
			pLines[*nLinesRead] = pCurrent;
			*nLinesRead += 1;
		}
		if(pEOL) {
			// Move forward to the next line
			pCurrent = pEOL + 1;
		}
		else {
			// No EOL means end of file
			break;
		}
	}

	// Convert the list of UTF-8 strings to UTF-16
	pResult = calloc(*nLinesRead, sizeof(LPWSTR));
	for(int i = 0; i < *nLinesRead; i++) {
		// Determine how many characters are necessary to fit the converted line
		int chars = MultiByteToWideChar(CP_UTF8, 0, pLines[i], -1, NULL, 0);
		pResult[i] = malloc(chars * sizeof(WCHAR));

		// Convert using CP_UTF8
		MultiByteToWideChar(CP_UTF8, 0, pLines[i], -1, pResult[i], chars);
		// Trim any whitespace
		StrTrim(pResult[i], L" \r\n");
	}

	free(pLines);
	free(pContent);

	CloseHandle(hFile);
	return pResult;
}

BOOL ToTextFile(LPCWSTR pFileName, LPWSTR* pLines, int nLines) {
	// Always create a new file or overwrite
	HANDLE hFile = CreateFile(pFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	for(int i = 0; i < nLines; i++) {
		// Determine how many characters are necessary to fit the converted line
		// 2 extra characters are added for the case of CRLF being appended
		int chars = WideCharToMultiByte(CP_UTF8, 0, pLines[i], -1, NULL, 0, NULL, NULL) + 2;
		LPSTR writeBuffer = calloc(chars, 1);

		// Convert using CP_UTF8
		WideCharToMultiByte(CP_UTF8, 0, pLines[i], -1, writeBuffer, chars, NULL, NULL);

		// Append CRLF unless this is the last line
		BOOL useNewLine = (i < nLines - 1);
		if(useNewLine) {
			writeBuffer[strlen(writeBuffer)] = '\r';
			writeBuffer[strlen(writeBuffer)] = '\n';
		}

		DWORD written = 0;
		BOOL writeSuccess = WriteFile(hFile, writeBuffer, strlen(writeBuffer), &written, NULL);
		free(writeBuffer);

		if(!writeSuccess) {
			CloseHandle(hFile);
			// Delete the file if writing failed
			DeleteFile(pFileName);
			return FALSE;
		}
	}

	CloseHandle(hFile);
	return TRUE;
}
