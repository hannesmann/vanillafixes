#include <MinHook.h>
#include <mmsystem.h>

#include "offsets.h"

#include "cpu.c"
#include "util.c"

static HMODULE g_hSelf = NULL;
static BOOL g_mainThreadFinished = FALSE;

/* Improves accuracy of timeouts and disables power throttling */
void DisableWindowsThrottling() {
	/* Will be cleaned up when the process exits */
	timeBeginPeriod(1);

	UtilSetPowerThrottlingState(PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION, FALSE);
	UtilSetPowerThrottlingState(PROCESS_POWER_THROTTLING_EXECUTION_SPEED, FALSE);
}

/* Hook OsTimeManager::TimeKeeper to apply our changes and prevent them from being overwritten once the game starts */
DWORD WINAPI VfTimeKeeperThreadProc(LPVOID lpParameter) {
	DisableWindowsThrottling();

	*g_pWowTimerTicksPerSecond = CpuCalibrateTsc();
	*g_pWowTimerToMilliseconds = 1.0 / (*g_pWowTimerTicksPerSecond * 0.001);
	/* Align TSC with GetTickCount for CHECK_TIMING_VALUES */
	*g_pWowTimerOffset = GetTickCount() - (fnWowReadTsc() * *g_pWowTimerToMilliseconds);
	*g_pWowUseTsc = TRUE;

	while(!g_mainThreadFinished) {
		Sleep(1);
	}

	FreeLibraryAndExitThread(g_hSelf, 0);
}

typedef DWORD (*PLOAD)();

void InitNamPower() {
	if(*g_pSharedData) {
		if((*g_pSharedData)->initNamPower) {
			PLOAD initFn = (PLOAD)GetProcAddress(GetModuleHandle(L"nampower"), "Load");

			if(initFn()) {
				MessageBox(NULL, L"Error when initializing Nampower", L"VanillaFixes", MB_OK | MB_ICONERROR);
			}
		}

		if(!VirtualFree(*g_pSharedData, 0, MEM_RELEASE)) {
			MessageBox(NULL, L"Failed to clean up remote data", L"VanillaFixes", MB_OK | MB_ICONERROR);
		}

		*g_pSharedData = NULL;
	}
}

/* Hook this function to prevent it from hanging the main thread */
DWORD64 VfHwGetCpuFrequency() {
	InitNamPower();

	while(!*g_pWowUseTsc) {
		Sleep(1);
	}

	g_mainThreadFinished = TRUE;

	/* 1.12 assumes TSC frequency == CPU frequency */
	return *g_pWowTimerTicksPerSecond;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	g_hSelf = hinstDLL;

	if(fdwReason == DLL_PROCESS_ATTACH) {
		if(MH_Initialize() != MH_OK) {
			return FALSE;
		}
		if(MH_CreateHook(fnWowTimeKeeperThreadProc, &VfTimeKeeperThreadProc, NULL) != MH_OK) {
			return FALSE;
		}
		if(MH_CreateHook(fnWowHwGetCpuFrequency, &VfHwGetCpuFrequency, NULL) != MH_OK) {
			return FALSE;
		}
		if(MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
			return FALSE;
		}
	}
	else if(fdwReason == DLL_PROCESS_DETACH) {
		MH_DisableHook(MH_ALL_HOOKS);
		MH_Uninitialize();
	}

	return TRUE;
}
