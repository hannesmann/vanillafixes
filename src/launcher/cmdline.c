#include <shlwapi.h>
#include <stdlib.h>

typedef struct {
	// Arguments that should be passed to WoW
	LPWSTR *pWowArgs;
	// Number of arguments in pWowArgs
	int nWowArgs;
	// The game executable path
	LPWSTR pWowExePath;
} VF_CMDLINE_PARSE_DATA, *PVF_CMDLINE_PARSE_DATA;

void CmdLineAppendArg(LPWSTR pArg, PVF_CMDLINE_PARSE_DATA pOutput) {
	int argsBufSize = (pOutput->nWowArgs + 1) * sizeof(LPWSTR);

	LPWSTR *temp = pOutput->pWowArgs;
	temp = realloc(temp, argsBufSize);
	if(!temp) {
		return;
	}

	pOutput->pWowArgs = temp;
	pOutput->nWowArgs++;

 	// +2 for potential quotes
	LPWSTR pEscapedArg = malloc((wcslen(pArg) + 3 + 2) * sizeof(WCHAR));
	if(!pEscapedArg) {
		return;
	}

	wcscpy(pEscapedArg, pArg);
	PathQuoteSpaces(pEscapedArg);

	pOutput->pWowArgs[pOutput->nWowArgs - 1] = pEscapedArg;
}

BOOL CmdLineContainsWowExe(LPWSTR pArg) {
	return !!StrStrIW(pArg, L".exe");
}

BOOL CmdLineContainsVfExe(LPWSTR pArg) {
	return !!StrStrIW(pArg, L"VanillaFixes.exe");
}

// Parse the process command line into a list
void CmdLineParse(int argc, WCHAR **argv, PVF_CMDLINE_PARSE_DATA pOutput) {
	for(int i = 1; i < argc; ++i) {
		// Is this argument an executable file?
		if(CmdLineContainsWowExe(argv[i])) {
			// Is this argument the VanillaFixes launcher?
			if(!CmdLineContainsVfExe(argv[i])) {
				// If not, use it as a potential game executable
				pOutput->pWowExePath = argv[i];
			}
		}
		else {
			// If the argument is not an executable, pass it on to WoW
			CmdLineAppendArg(argv[i], pOutput);
		}
	}
}

// Format the game command line
LPWSTR CmdLineFormat(PVF_CMDLINE_PARSE_DATA pInput) {
	LPWSTR pCmdLine = calloc(MAX_PATH, sizeof(WCHAR));
	if(!pCmdLine) {
		return NULL;
	}

	// First, copy the name of the game executable into the command line
	wcscpy(pCmdLine, pInput->pWowExePath);
	PathQuoteSpaces(pCmdLine);

	// Calculate the characters (including NUL) required to hold all arguments
	int requiredChars = wcslen(pCmdLine) + 1;
	for(int i = 0; i < pInput->nWowArgs; ++i) {
		requiredChars += 1 + wcslen(pInput->pWowArgs[i]);
	}

	if(requiredChars > MAX_PATH) {
		LPWSTR temp = realloc(pCmdLine, requiredChars * sizeof(WCHAR));
		if(!temp) {
			return NULL;
		}
		pCmdLine = temp;
	}

	// Copy the arguments into the command line
	LPWSTR pCurrent = pCmdLine + wcslen(pCmdLine);
	for(int i = 0; i < pInput->nWowArgs; ++i) {
		*(pCurrent++) = L' ';
		wcscpy(pCurrent, pInput->pWowArgs[i]);
		pCurrent += wcslen(pInput->pWowArgs[i]);
	}

	// End with NUL
	*pCurrent = L'\0';

	return pCmdLine;
}
