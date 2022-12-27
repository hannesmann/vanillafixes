#include <processthreadsapi.h>

typedef BOOL (WINAPI *PSET_PROCESS_INFORMATION)(HANDLE, PROCESS_INFORMATION_CLASS, LPVOID, DWORD);

BOOL UtilSetPowerThrottlingState(ULONG feature, BOOL enable) {
	static PSET_PROCESS_INFORMATION fnSetProcessInformation = NULL;

	if(!fnSetProcessInformation) {
		fnSetProcessInformation =
			(PSET_PROCESS_INFORMATION)GetProcAddress(GetModuleHandle(L"kernel32"), "SetProcessInformation");
	}

	/* Ignore if function was not found (not available in Windows 7 and Wine) */
	if(fnSetProcessInformation) {
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

	return FALSE;
}