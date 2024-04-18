#include <processthreadsapi.h>

typedef BOOL(WINAPI* PSET_PROCESS_INFORMATION)(HANDLE, PROCESS_INFORMATION_CLASS, LPVOID, DWORD);

BOOL UtilSetPowerThrottlingState(ULONG feature, BOOL enable) {
	static PSET_PROCESS_INFORMATION fnSetProcessInformation = NULL;

	// If the SetProcessInformation function is not already loaded, attempt to load it.
	if(!fnSetProcessInformation) {
		HMODULE hKernel32 = GetModuleHandle(L"kernel32"); // Get a handle to the kernel32 module.

		// Attempt to get the address of the SetProcessInformation function.
		fnSetProcessInformation = (PSET_PROCESS_INFORMATION)GetProcAddress(hKernel32, "SetProcessInformation");

		// If getting the function address failed, return FALSE to indicate an error.
		if(!fnSetProcessInformation) {
			return FALSE;
		}
	}

	// If the SetProcessInformation function was found, use it to set the power throttling state.
	if(fnSetProcessInformation) {
		PROCESS_POWER_THROTTLING_STATE powerThrottlingState = { 0 };

		// Set the version, control mask, and state mask of the power throttling state structure.
		powerThrottlingState.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
		powerThrottlingState.ControlMask = feature;
		powerThrottlingState.StateMask = enable ? feature : 0;

		// Call the SetProcessInformation function to set the power throttling state.
		return fnSetProcessInformation(
			GetCurrentProcess(),
			ProcessPowerThrottling,
			&powerThrottlingState,
			sizeof(PROCESS_POWER_THROTTLING_STATE)
		);
	}

	// If the SetProcessInformation function was not found, return FALSE to indicate an error.
	return FALSE;
}
