#include <windows.h>
#include <mmsystem.h>
#include <winternl.h>

#include "macros.h"
#include "os.h"

LPWSTR GetLastErrorMessage() {
	LPWSTR pErrorStr = NULL;
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;
	if(FormatMessage(flags, NULL, GetLastError(), 0, (LPWSTR)&pErrorStr, 0, NULL)) {
		return pErrorStr;
	}
	return L"Unknown error";
}

typedef BOOL (WINAPI *PSET_PROCESS_DPI_AWARENESS_CONTEXT)(DPI_AWARENESS_CONTEXT);

void EnableDPIAwareness() {
	static PSET_PROCESS_DPI_AWARENESS_CONTEXT fnSetProcessDPIAwarenessContext = NULL;

	// If the SetProcessDpiAwarenessContext function is not already loaded, attempt to load it
	if(!fnSetProcessDPIAwarenessContext) {
		// Make sure user32 is loaded
		LoadLibrary(L"user32");
		HMODULE hUser32 = GetModuleHandle(L"user32");
		fnSetProcessDPIAwarenessContext = (PSET_PROCESS_DPI_AWARENESS_CONTEXT)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");

		// If getting the function address failed, fall back to the old API
		if(fnSetProcessDPIAwarenessContext) {
			DebugOutputF(L"Using SetProcessDpiAwarenessContext to set DPI scaling mode");
			if(!fnSetProcessDPIAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
				DebugOutputF(L"DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 not supported");
				// If DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 is not supported, fall back to the old API
				SetProcessDPIAware();
				DebugOutputF(L"Using SetProcessDPIAware to set DPI scaling mode");
			}
		}
		else {
			SetProcessDPIAware();
			DebugOutputF(L"Using SetProcessDPIAware to set DPI scaling mode");
		}
	}
}

typedef NTSTATUS (NTAPI *PNT_QUERY_TIMER_RESOLUTION)(PULONG, PULONG, PULONG);
typedef NTSTATUS (NTAPI *PNT_SET_TIMER_RESOLUTION)(ULONG, BOOLEAN, PULONG);

BOOL IncreaseTimerResolution(ULONG maxRequestedResolution) {
	static PNT_QUERY_TIMER_RESOLUTION fnNtQueryTimerResolution = NULL;
	static PNT_SET_TIMER_RESOLUTION fnNtSetTimerResolution = NULL;

	// If the NT timer functions are not already loaded, attempt to load them
	if(!fnNtQueryTimerResolution || !fnNtSetTimerResolution) {
		HMODULE hNtDll = GetModuleHandle(L"ntdll");
		fnNtQueryTimerResolution = (PNT_QUERY_TIMER_RESOLUTION)GetProcAddress(hNtDll, "NtQueryTimerResolution");
		fnNtSetTimerResolution = (PNT_SET_TIMER_RESOLUTION)GetProcAddress(hNtDll, "NtSetTimerResolution");

		// Use timeBeginPeriod if NtSetTimerResolution is unavailable
		if(!fnNtQueryTimerResolution || !fnNtSetTimerResolution) {
			UINT resolution = max(1, maxRequestedResolution / 10000);
			DebugOutputF(L"Using timeBeginPeriod (desired=%u)", resolution);
			return timeBeginPeriod(resolution) == TIMERR_NOERROR;
		}
	}

	ULONG minSupportedResolution, maxSupportedResolution, currentResolution;
	fnNtQueryTimerResolution(&minSupportedResolution, &maxSupportedResolution, &currentResolution);

	ULONG desiredResolution = max(maxSupportedResolution, maxRequestedResolution);
	DebugOutputF(L"Using NT timer functions (min=%lu max=%lu current=%lu desired=%lu)",
		minSupportedResolution, maxSupportedResolution, currentResolution, desiredResolution);

	if(fnNtSetTimerResolution(desiredResolution, TRUE, &currentResolution) != 0) {
		DebugOutputF(L"NtSetTimerResolution failed!");

		// Fall back to timeBeginPeriod if NtSetTimerResolution failed
		UINT resolution = max(1, maxRequestedResolution / 10000);
		DebugOutputF(L"Using timeBeginPeriod (desired=%u)", resolution);
		return timeBeginPeriod(resolution) == TIMERR_NOERROR;
	}

	return TRUE;
}

typedef BOOL (WINAPI *PSET_PROCESS_INFORMATION)(HANDLE, PROCESS_INFORMATION_CLASS, LPVOID, DWORD);

BOOL SetPowerThrottlingState(ULONG feature, BOOL enable) {
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
