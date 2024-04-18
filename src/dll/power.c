#include <windows.h>

typedef BOOL (WINAPI *PSET_PROCESS_INFORMATION)(HANDLE, PROCESS_INFORMATION_CLASS, LPVOID, DWORD);

static BOOL SetPowerThrottlingState(ULONG feature, BOOL enable) {
	static PSET_PROCESS_INFORMATION fnSetProcessInformation = NULL;

	// If the SetProcessInformation function is not already loaded, attempt to load it
	if(!fnSetProcessInformation) {
		HMODULE hKernel32 = GetModuleHandle(L"kernel32");
		fnSetProcessInformation = (PSET_PROCESS_INFORMATION)GetProcAddress(hKernel32, "SetProcessInformation");

		// If getting the function address failed, return FALSE to indicate an error
		if(!fnSetProcessInformation) {
			return FALSE;
		}
	}

	PROCESS_POWER_THROTTLING_STATE powerThrottlingState = {0};
	powerThrottlingState.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
	powerThrottlingState.ControlMask = feature;
	powerThrottlingState.StateMask = enable ? feature : 0;

	return fnSetProcessInformation(
		GetCurrentProcess(),
		ProcessPowerThrottling,
		&powerThrottlingState,
		sizeof(PROCESS_POWER_THROTTLING_STATE)
	);
}