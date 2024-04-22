#pragma once

#include <windows.h>

typedef struct {
	// Arguments that should be passed to WoW
	LPWSTR *pWowArgs;
	// Number of arguments in pWowArgs
	int nWowArgs;
	// The game executable path
	LPWSTR pWowExePath;
} VF_CMDLINE_PARSE_DATA, *PVF_CMDLINE_PARSE_DATA;

// Parse the process command line into a list
void CmdLineParse(int argc, WCHAR **argv, PVF_CMDLINE_PARSE_DATA pOutput);

// Format the game command line
LPWSTR CmdLineFormat(PVF_CMDLINE_PARSE_DATA pInput);
