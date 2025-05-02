#include <windows.h>
#include <shlwapi.h>
#include <stdlib.h>

#ifndef _MSC_VER
#include <cpuid.h>
#endif

#include "macros.h"

#include "loader.h"
#include "os.h"

#include "cmdline.h"
#include "selftest.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	// Dialog boxes should be scaled to system DPI
	EnableDPIAwareness();

	// Gather CPU info
	unsigned int cpuInfo[4] = {0};
#ifdef _MSC_VER
	__cpuid(cpuInfo, 0x80000007);
#else
	__get_cpuid(0x80000007, &cpuInfo[0], &cpuInfo[1], &cpuInfo[2], &cpuInfo[3]);
#endif

	// Check if the CPU supports the "Invariant TSC" feature
	AssertMessageBoxF(cpuInfo[3] & (1 << 8),
		L"Your processor does not support the \"Invariant TSC\" feature.\r\n\r\n"
		L"VanillaFixes can only work reliably on modern (2007+) Intel and AMD processors."
	);

	LPWSTR pWowDirectory = malloc(MAX_PATH * sizeof(WCHAR));
	GetModuleFileName(NULL, pWowDirectory, MAX_PATH);
	// Check if the buffer was large enough
	AssertMessageBoxF(GetLastError() != ERROR_INSUFFICIENT_BUFFER, L"GetLastError() == ERROR_INSUFFICIENT_BUFFER");
	// Remove the file name from the directory path
	PathRemoveFileSpec(pWowDirectory);

	LPWSTR pWowExePath = malloc(MAX_PATH * sizeof(WCHAR));
	PathCombine(pWowExePath, pWowDirectory, L"WoW.exe");

	VF_CMDLINE_PARSE_DATA cmdLineData = {0};
	cmdLineData.pWowExePath = pWowExePath;
	CmdLineParse(__argc, __wargv, &cmdLineData);

	// Check to see if self-test should run
	if(cmdLineData.isSelfTestExecutable) {
		return SelfTestMain(hInstance, hPrevInstance, pCmdLine, nCmdShow);
	}

	LPWSTR pConfigPath = malloc(MAX_PATH * sizeof(WCHAR));
	PathCombine(pConfigPath, pWowDirectory, L"dlls.txt");

	// Check if the necessary files exist
	AssertMessageBoxF(GetFileAttributes(pWowExePath) != INVALID_FILE_ATTRIBUTES,
		L"WoW.exe not found. Extract VanillaFixes into the same directory as the game.");

	// Check to see if additional DLLs should be loaded
	VF_DLL_LIST_PARSE_DATA dllListData = {0};
	LoaderParseConfig(pWowDirectory, pConfigPath, &dllListData);

	// If there are additional DLLs, and the list has been modified since last launch, show a message box
	if(dllListData.nAdditionalDLLs && dllListData.listModified) {
		LPCWSTR pFormat =
			L"VanillaFixes will load additional DLLs listed in dlls.txt:\r\n"
			L"%ls\r\n\r\n"
			L"If you don't agree, press Cancel. "
			L"This reminder will be shown when the list of files has changed.";
		LPWSTR pArg = LoaderListText(pWowDirectory, &dllListData);

		// Format message for the user showing the list of additional DLLs
		int chars = wcslen(pFormat) + wcslen(pArg) + 1;
		LPWSTR pMessage = malloc(chars * sizeof(WCHAR));
		swprintf(pMessage, chars, pFormat, pArg);

		int result = MessageBox(NULL, pMessage, L"VanillaFixes", MB_OKCANCEL | MB_ICONEXCLAMATION);
		if(result != IDOK) {
			return result;
		}
	}

	STARTUPINFO startupInfo = {0};
	PROCESS_INFORMATION processInfo = {0};

	startupInfo.cb = sizeof(startupInfo);
	// Pass shortcut properties to WoW executable
	startupInfo.wShowWindow = nCmdShow;
	startupInfo.dwFlags = STARTF_USESHOWWINDOW;

	// Prepare command line arguments for WoW
	LPWSTR pWowCmdLine = CmdLineFormat(&cmdLineData);

	// Create the game process in a suspended state to ensure VanillaFixes can hook functions early
	DWORD flags = CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT;
	BOOL processCreated =
		CreateProcess(NULL, pWowCmdLine, NULL, NULL, FALSE, flags, NULL, NULL, &startupInfo, &processInfo);
	AssertMessageBoxF(processCreated,
		L"Error creating process: %ls\r\n"
		L"This issue can occur if you have enabled compatibility mode on the WoW executable.",
		GetLastErrorMessage());

	int injectError = 0;
	if(dllListData.nAdditionalDLLs) {
		// Load mods in the order specified in dlls.txt (init is done in DLL_PROCESS_ATTACH or by exporting a Load function)
		for(int i = 0; i < dllListData.nAdditionalDLLs; i++) {
			injectError = injectError || RemoteLoadLibrary(dllListData.pAdditionalDLLs[i], processInfo.hProcess);
		}
	}

	// If there were any errors loading DLLs into the game process, cancel launch
	if(injectError) {
		TerminateProcess(processInfo.hProcess, 0);
		return injectError;
	}

	// Resume the WoW process
	ResumeThread(processInfo.hThread);

	// Close process handles once they're no longer needed
	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);

	// Persist DLL list changes to silence the reminder
	LoaderUpdateCacheState(pConfigPath, &dllListData);

	// Run a DXVK self-test to inform the user of errors due to missing Vulkan support or missing drivers
	VF_SELFTEST_RESULT selfTestResult = RunSelfTest(pWowDirectory);
	LPCWSTR pFailReason = TestResultToStr(selfTestResult);

	// Ignore the result if the process creation failed or system D3D9 is in use
	BOOL ignoreResult = selfTestResult == VF_SELFTEST_SUCCESS ||
		selfTestResult == VF_SELFTEST_FAIL_REASON_CREATEPROCESS ||
		selfTestResult == VF_SELFTEST_FAIL_REASON_STILL_ACTIVE ||
		selfTestResult == VF_SELFTEST_FAIL_REASON_SYSTEM_D3D9;
	AssertMessageBoxF(ignoreResult,
		L"DXVK self-test failed (%ls). The game will now crash.\r\n\r\n"
		L"This is likely caused by missing Vulkan 1.3 support. "
		L"The DXVK log file \"VanillaFixes_d3d9.log\" may have more information.\r\n\r\n"
		L"If you have an older GPU (R9 200/300, GTX 700), remove \"d3d9.dll\" to use VanillaFixes without DXVK. "
		L"Otherwise, try updating your drivers to the latest version.",
		pFailReason);

	// End the process normally
	return 0;
}
