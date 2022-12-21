#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <MinHook.h>
#include <mmsystem.h>
#include <processthreadsapi.h>

#include <stdio.h>

#include "offsets.h"
#include "cpu.c"

static HMODULE hSelf = NULL;
static BOOL mainThreadFinished = FALSE;

typedef BOOL (WINAPI *SETPROCESSINFORMATION)(HANDLE, PROCESS_INFORMATION_CLASS, LPVOID, DWORD);

/* Executed on the "time keeper" thread */
DWORD WINAPI VfTimeKeeperThreadProc(LPVOID lpParameter) {
	static char scratchBuffer[128];

	SETPROCESSINFORMATION pSetProcessInformation =
		(SETPROCESSINFORMATION)GetProcAddress(GetModuleHandle("kernel32"), "SetProcessInformation");

	if(pSetProcessInformation) {
		PROCESS_POWER_THROTTLING_STATE powerThrottling;
		ZeroMemory(&powerThrottling, sizeof(PROCESS_POWER_THROTTLING_STATE));

		powerThrottling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
		powerThrottling.ControlMask = PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;

		OutputDebugString("VanillaFixes: Disabing Windows 11 timer resolution throttling");
		BOOL setProcessInformationError = pSetProcessInformation(
			GetCurrentProcess(),
			ProcessPowerThrottling,
			&powerThrottling,
			sizeof(PROCESS_POWER_THROTTLING_STATE)
		);

		sprintf(scratchBuffer, "VanillaFixes: SetProcessInformation returned %d", setProcessInformationError);
		OutputDebugString(scratchBuffer);

		powerThrottling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;

		OutputDebugString("VanillaFixes: Disabing execution speed throttling");
		setProcessInformationError = pSetProcessInformation(
			GetCurrentProcess(),
			ProcessPowerThrottling,
			&powerThrottling,
			sizeof(PROCESS_POWER_THROTTLING_STATE)
		);

		sprintf(scratchBuffer, "VanillaFixes: SetProcessInformation returned %d", setProcessInformationError);
		OutputDebugString(scratchBuffer);
	}
	else {
		OutputDebugString("VanillaFixes: SetProcessInformation not supported");
	}

	*pWowTscTicksPerSecond = CpuCalibrateTsc();
	*pWowTscToMilliseconds = 1.0 / (*pWowTscTicksPerSecond * 0.001);
	/* Align TSC with GetTickCount for CHECK_TIMING_VALUES */
	*pWowTimerOffset = GetTickCount() - (WowReadTsc() * *pWowTscToMilliseconds);
	*pWowUseTsc = TRUE;

	sprintf(scratchBuffer, "VanillaFixes: TSC frequency is %.3lf MHz", *pWowTscTicksPerSecond * 0.000001);
	OutputDebugString(scratchBuffer);

	while(!mainThreadFinished) {
		Sleep(1);
	}

	FreeLibraryAndExitThread(hSelf, 0);
}

/* Executed on main thread */
DWORD64 VfGetTicksPerSecond() {
	while(!*pWowTscTicksPerSecond || !*pWowTimerOffset) {
		Sleep(1);
	}

	mainThreadFinished = TRUE;
	return *pWowTscTicksPerSecond;
}

/* Executed on injection thread */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
	hSelf = hinstDLL;

    if(fdwReason == DLL_PROCESS_ATTACH) {
		OutputDebugString("VanillaFixes: DLL_PROCESS_ATTACH");

		if(MH_Initialize() != MH_OK) {
			OutputDebugString("VanillaFixes: MH_Initialize failed");
			return FALSE;
		}
		if(MH_CreateHook(pWowTimeKeeperThreadProc, &VfTimeKeeperThreadProc, NULL) != MH_OK) {
			OutputDebugString("VanillaFixes: Time keeper hook failed");
			return FALSE;
		}
		if(MH_CreateHook(pWowGetTicksPerSecond, &VfGetTicksPerSecond, NULL) != MH_OK) {
			OutputDebugString("VanillaFixes: Ticks per second hook failed");
			return FALSE;
		}
		if(MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
			OutputDebugString("VanillaFixes: MH_EnableHook(MH_ALL_HOOKS) failed");
			return FALSE;
		}

		/* Will be cleaned up when the process exits */
		timeBeginPeriod(1);
	}
	else if(fdwReason == DLL_PROCESS_DETACH) {
		OutputDebugString("VanillaFixes: DLL_PROCESS_DETACH");

		MH_DisableHook(MH_ALL_HOOKS);
		MH_Uninitialize();
	}

	return TRUE;
}
