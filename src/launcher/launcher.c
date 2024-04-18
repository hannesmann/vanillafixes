#include <shlwapi.h>

#include "sync.c"
#include "util.c"

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow) {
	// Gather CPU info
	int cpuInfo[4] = { 0 };
	__cpuid(cpuInfo, 0x80000007);

	// Check if the CPU supports the "Invariant TSC" feature
	AssertMessageBox(cpuInfo[3] & (1 << 8),
		L"Your processor does not support the \"Invariant TSC\" feature.\r\n\r\n"
		L"VanillaFixes can only work reliably on modern (2007+) Intel and AMD processors."
	);

	LPWSTR pWowDirectory = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
	if(pWowDirectory == NULL) {
		MessageBox(NULL, L"Failed to allocate memory.", L"VanillaFixes", MB_OK | MB_ICONERROR);
		return 1;
	}

	GetModuleFileName(NULL, pWowDirectory, MAX_PATH);

	// Check if the buffer was large enough
	AssertMessageBox(GetLastError() != ERROR_INSUFFICIENT_BUFFER, L"GetLastError() == ERROR_INSUFFICIENT_BUFFER");

	// Remove the file name from the directory path
	PathRemoveFileSpec(pWowDirectory);

	// Get paths for the WoW exe, patcher dll, and nampower dll
	LPWSTR pWowExePath = UtilGetPath(pWowDirectory, L"WoW.exe");
	LPWSTR pPatcherPath = UtilGetPath(pWowDirectory, L"VfPatcher.dll");
	LPWSTR pNamPowerPath = UtilGetPath(pWowDirectory, L"nampower.dll");

	// Check if the necessary files exist
	AssertMessageBox(GetFileAttributes(pWowExePath) != INVALID_FILE_ATTRIBUTES,
		L"WoW.exe not found. Extract VanillaFixes into the same directory as the game.");
	AssertMessageBox(GetFileAttributes(pPatcherPath) != INVALID_FILE_ATTRIBUTES,
		L"VfPatcher.dll not found. Extract all files into the game directory.");

	STARTUPINFO startupInfo = { 0 };
	PROCESS_INFORMATION processInfo = { 0 };

	startupInfo.cb = sizeof(startupInfo);
	// Pass shortcut properties to WoW executable
	startupInfo.wShowWindow = nCmdShow;
	startupInfo.dwFlags = STARTF_USESHOWWINDOW;

	// Prepare command line arguments for WoW
	LPWSTR pWowCmdLine = UtilGetWowCmdLine(pWowExePath);
	DWORD flags = CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT;

	// Create the WoW process
	if(!CreateProcess(NULL, pWowCmdLine, NULL, NULL, FALSE, flags, NULL, NULL, &startupInfo, &processInfo)) {
		LPWSTR errorMessage = UtilGetLastError();
		WCHAR scratchBuffer[512] = { 0 };

		swprintf(scratchBuffer,
			512,
			L"Error creating process: %ls\r\n"
			L"This issue can occur if you have enabled compatibility mode on the WoW executable.",
			errorMessage
		);

		return MessageBox(NULL, scratchBuffer, L"VanillaFixes", MB_OK | MB_ICONERROR);
	}

	int injectError = RemoteLoadLibrary(pPatcherPath, processInfo.hProcess);

	// Also inject Nampower if present in the game directory
	if(GetFileAttributes(pNamPowerPath) != INVALID_FILE_ATTRIBUTES) {
		injectError = injectError || RemoteLoadLibrary(pNamPowerPath, processInfo.hProcess);

		g_sharedData.initNamPower = TRUE;
		RemoteSyncData(processInfo.hProcess);
	}

	if(injectError) {
		TerminateProcess(processInfo.hProcess, 0);
		return injectError;
	}

	// Remote data will be freed by VfPatcher.dll
	g_pRemoteData = NULL;

	// Resume the WoW process
	ResumeThread(processInfo.hThread);

	// Close handles to the process and the main thread after the process has been resumed
	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);

	// End the process normally
	return 0;
}
