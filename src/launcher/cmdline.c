#include <shlwapi.h>

typedef struct {
	LPCWSTR* pWowArgs;
	int nWowArgs;

	LPCWSTR pWowExePath;
} VF_CMDLINE_PARSE_DATA, *PVF_CMDLINE_PARSE_DATA;

BOOL CmdLineIsWowExe(LPWSTR pArg) {
	return !!StrStrIW(pArg, L".exe");
}

BOOL CmdLineIsVfExe(LPWSTR pArg) {
	return !!StrStrIW(pArg, L"VanillaFixes.exe");
}

void CmdLineAppendArg(LPWSTR pArg, PVF_CMDLINE_PARSE_DATA pOutput) {
	int argsBufSize = ++pOutput->nWowArgs * sizeof(LPWSTR);

	if(pOutput->pWowArgs) {
		pOutput->pWowArgs = HeapReAlloc(GetProcessHeap(), 0, pOutput->pWowArgs, argsBufSize);
	}
	else {
		pOutput->pWowArgs = HeapAlloc(GetProcessHeap(), 0, argsBufSize);
	}

	LPWSTR pEscapedArg = HeapAlloc(GetProcessHeap(), 0, (wcslen(pArg) + 3) * sizeof(WCHAR));
	StrCpy(pEscapedArg, pArg);
	PathQuoteSpaces(pEscapedArg);

	pOutput->pWowArgs[pOutput->nWowArgs - 1] = pEscapedArg;
}

void CmdLineParse(int argc, WCHAR** argv, PVF_CMDLINE_PARSE_DATA pOutput) {
	for(int i = 1; i < argc; i++) {
		if(CmdLineIsWowExe(argv[i])) {
			if(!CmdLineIsVfExe(argv[i])) {
				pOutput->pWowExePath = argv[i];
			}
		}
		else {
			CmdLineAppendArg(argv[i], pOutput);
		}
	}
}

LPWSTR CmdLineFormat(PVF_CMDLINE_PARSE_DATA pInput) {
	LPWSTR pCmdLine = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
	ZeroMemory(pCmdLine, MAX_PATH * sizeof(WCHAR));

	StrCpy(pCmdLine, pInput->pWowExePath);
	PathQuoteSpaces(pCmdLine);

	int requiredChars = wcslen(pCmdLine) + 1;
	for(int i = 0; i < pInput->nWowArgs; i++) {
		requiredChars += 1 + wcslen(pInput->pWowArgs[i]);
	}

	if(requiredChars > MAX_PATH) {
		pCmdLine = HeapReAlloc(GetProcessHeap(), 0, pCmdLine, requiredChars * sizeof(WCHAR));
	}

	LPWSTR pCurrent = pCmdLine + wcslen(pCmdLine);
	for(int i = 0; i < pInput->nWowArgs; i++) {
		*(pCurrent++) = L' ';
		StrCpy(pCurrent, pInput->pWowArgs[i]);
		pCurrent += wcslen(pInput->pWowArgs[i]);
	}
	*pCurrent = L'\0';

	return pCmdLine;
}