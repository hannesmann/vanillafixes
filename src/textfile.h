#pragma once

#include <windows.h>

// Read from a UTF-8 text file, optionally ignoring comments
LPWSTR* FromTextFile(LPCWSTR pFileName, int* nLinesRead, BOOL ignoreComments);

// Write lines to a UTF-8 text file, overwriting old content
BOOL ToTextFile(LPCWSTR pFileName, LPWSTR* pLines, int nLines);
