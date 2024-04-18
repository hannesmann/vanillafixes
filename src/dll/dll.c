#include <MinHook.h>
#include <mmsystem.h>

#include "offsets.h"

#include "cpu.c"
#include "util.c"

// Define PLOAD as a pointer to a function that takes no arguments and returns a DWORD.
typedef DWORD(*PLOAD)();

// Handle to the DLL module.
static HMODULE g_hSelf = NULL;

// Boolean flag to indicate whether the main thread has finished execution.
static volatile BOOL g_mainThreadFinished = FALSE; // declared as volatile because it is shared between threads

// Function to disable Windows throttling for better timing accuracy.
void DisableWindowsThrottling() {
	// This reduces the system timer resolution, improving accuracy of timeouts.
	timeBeginPeriod(1);

	// Disable power throttling for the process.
	UtilSetPowerThrottlingState(PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION, FALSE);
	UtilSetPowerThrottlingState(PROCESS_POWER_THROTTLING_EXECUTION_SPEED, FALSE);
}

// Hooked function to apply our changes to the timekeeper.
DWORD WINAPI VfTimeKeeperThreadProc(LPVOID lpParameter) {
	DisableWindowsThrottling();

	*g_pWowTimerTicksPerSecond = CpuCalibrateTsc();
	*g_pWowTimerToMilliseconds = 1.0 / (*g_pWowTimerTicksPerSecond * 0.001);

	// Align the timestamp counter with GetTickCount(), because Vanilla WoW uses GetTickCount by default.
	// Also "GetTickCount64()" is not supported on Windows versions older then Vista.
	*g_pWowTimerOffset = GetTickCount() - (fnWowReadTsc() * *g_pWowTimerToMilliseconds);

	*g_pWowUseTsc = TRUE;

	while(!g_mainThreadFinished) {
		Sleep(1);
	}

	FreeLibraryAndExitThread(g_hSelf, 0);
}

// Function to initialize Nampower.
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

// Hooked function to prevent main thread from hanging.
DWORD64 VfHwGetCpuFrequency() {
	InitNamPower();

	while(!*g_pWowUseTsc) {
		Sleep(1);
	}

	g_mainThreadFinished = TRUE;

	// 1.12 assumes TSC frequency == CPU frequency
	return *g_pWowTimerTicksPerSecond;
}

// DLL entry point.
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
