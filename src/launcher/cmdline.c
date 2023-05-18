#include <shlwapi.h>


typedef struct
{
	LPCWSTR* pWowArgs;
	int nWowArgs;

	LPCWSTR pWowExePath;
} VF_CMDLINE_PARSE_DATA, * PVF_CMDLINE_PARSE_DATA;

BOOL CmdLineContainsWowExe(LPWSTR pArg)
{
	return !!StrStrIW(pArg, L".exe");
}

BOOL CmdLineContainsVfExe(LPWSTR pArg)
{
	return !!StrStrIW(pArg, L"VanillaFixes.exe");
}

void CmdLineAppendArg(LPWSTR pArg, PVF_CMDLINE_PARSE_DATA pOutput)
{
	int argsBufSize = (pOutput->nWowArgs + 1) * sizeof(LPWSTR);

	LPWSTR* temp = pOutput->pWowArgs;
	temp = HeapReAlloc(GetProcessHeap(), 0, temp, argsBufSize);
	if (temp == NULL)
	{
		return;
	}

	pOutput->pWowArgs = temp;
	pOutput->nWowArgs++;

	LPWSTR pEscapedArg = HeapAlloc(GetProcessHeap(), 0, (wcslen(pArg) + 3 + 2) * sizeof(WCHAR)); // +2 for potential quotes
	if (!pEscapedArg)
	{
		return;
	}

	StrCpy(pEscapedArg, pArg);
	PathQuoteSpaces(pEscapedArg);

	pOutput->pWowArgs[pOutput->nWowArgs - 1] = pEscapedArg;
}

void CmdLineParse(int argc, WCHAR** argv, PVF_CMDLINE_PARSE_DATA pOutput)
{
	for (int i = 1; i < argc; ++i)
	{
		if (CmdLineContainsWowExe(argv[i]))
		{
			if (!CmdLineContainsVfExe(argv[i]))
			{
				pOutput->pWowExePath = argv[i];
			}
		}
		else
		{
			CmdLineAppendArg(argv[i], pOutput);
		}
	}
}

LPWSTR CmdLineFormat(PVF_CMDLINE_PARSE_DATA pInput)
{
	LPWSTR pCmdLine = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
	if (!pCmdLine)
	{
		return NULL;
	}

	ZeroMemory(pCmdLine, MAX_PATH * sizeof(WCHAR));

	StrCpy(pCmdLine, pInput->pWowExePath);
	PathQuoteSpaces(pCmdLine);

	int requiredChars = wcslen(pCmdLine) + 1;
	for (int i = 0; i < pInput->nWowArgs; ++i)
	{
		requiredChars += 1 + wcslen(pInput->pWowArgs[i]);
	}

	if (requiredChars > MAX_PATH)
	{
		LPWSTR temp = HeapReAlloc(GetProcessHeap(), 0, pCmdLine, requiredChars * sizeof(WCHAR));
		if (temp == NULL)
		{
			return pCmdLine;
		}

		pCmdLine = temp;
	}

	LPWSTR pCurrent = pCmdLine + wcslen(pCmdLine);
	for (int i = 0; i < pInput->nWowArgs; ++i)
	{
		*(pCurrent++) = L' ';
		StrCpy(pCurrent, pInput->pWowArgs[i]);
		pCurrent += wcslen(pInput->pWowArgs[i]);
	}

	*pCurrent = L'\0';

	return pCmdLine;
}
