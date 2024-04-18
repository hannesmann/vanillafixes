#include <MinHook.h>
#include <windows.h>

#include "offsets_1_12_1.h"

#include "power.c"
#include "timer.c"
#include "util.h"

// Handle to this module, for unloading on exit
static HMODULE g_hSelf = NULL;

// Flag to indicate whether the main thread has finished execution
static volatile BOOL g_mainThreadFinished = FALSE;

// Hook OsTimeManager::TimeKeeper to apply our changes and prevent it from running in the background during gameplay
DWORD WINAPI VfTimeKeeperThreadProc(LPVOID lpParameter) {
	// Increase the process timer resolution up to a cap of 0.5 ms
	IncreaseTimerResolution(5000);

	// Disable Windows 11 power throttling for the process
	SetPowerThrottlingState(PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION, FALSE);
	SetPowerThrottlingState(PROCESS_POWER_THROTTLING_EXECUTION_SPEED, FALSE);

	// Calibrate the game timer using QueryPerformanceFrequency
	// This assumes the TSC is invariant
	*g_pWowTimerTicksPerSecond = CpuCalibrateTsc();
	DebugOutput(L"VanillaFixes: timerTicksPerSecond=%lld", *g_pWowTimerTicksPerSecond);
	*g_pWowTimerToMilliseconds = 1000.0 / *g_pWowTimerTicksPerSecond;
	*g_pWowUseTSC = TRUE;

	// Align TSC with GTC
	*g_pWowTimerOffset = GetTickCount() - (fnWowReadTSC() * *g_pWowTimerToMilliseconds);
	DebugOutput(L"VanillaFixes: timerOffset=%.0lf", *g_pWowTimerOffset);

	while(!g_mainThreadFinished) {
		Sleep(1);
	}

	DebugOutput(L"VanillaFixes: Done! Unloading from process %u...", GetCurrentProcessId());
	FreeLibraryAndExitThread(g_hSelf, 0);
}

// Some mods such as Nampower are initialized by a function call rather than DLL_PROCESS_ATTACH
typedef DWORD (*PLOAD)();

// Function to initialize Nampower
void InitNamPower() {
	if(*g_pSharedData) {
		if((*g_pSharedData)->initNamPower) {
			PLOAD initFn = (PLOAD)GetProcAddress(GetModuleHandle(L"nampower"), "Load");

			// Check if GetProcAddress returned NULL
			if(!initFn) {
				MessageBox(NULL, L"Failed to get address of Load function", L"VanillaFixes", MB_OK | MB_ICONERROR);
				return;
			}

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

// Hooked function to prevent main thread from hanging
DWORD64 VfHwGetCpuFrequency() {
	// The game has built-in UI scaling which means Windows DPI scaling is unnecessary
	// DXVK already enabled this by default (d3d9.dpiAware)
	if(!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
		// Try the old API if the new one is unsupported
		SetProcessDPIAware();
	}

	InitNamPower();

	// Wait until VfTimeKeeperThreadProc has finished calibration and applied changes to the game timer
	while(!*g_pWowUseTSC) {
		Sleep(1);
	}

	g_mainThreadFinished = TRUE;

	// 1.12 assumes TSC frequency == CPU frequency
	return *g_pWowTimerTicksPerSecond;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	g_hSelf = hinstDLL;

	switch(fdwReason) {
		case DLL_PROCESS_ATTACH: {
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

			break;
		}
		case DLL_PROCESS_DETACH: {
			MH_DisableHook(MH_ALL_HOOKS);
			MH_Uninitialize();
			break;
		}
	}

	return TRUE;
}
