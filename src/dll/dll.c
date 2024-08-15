#include <windows.h>
#include <intrin.h>
#include <math.h>
#include <processthreadsapi.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <psapi.h>

#include <MinHook.h>

#include "macros.h"
#include "loader.h"
#include "os.h"

// Handle to this module, for unloading on exit
static HMODULE g_hSelf = NULL;

// Flag to indicate whether the main thread has finished execution
static volatile BOOL g_mainThreadFinished = FALSE;

// Dynamically found addresses
static LPTHREAD_START_ROUTINE fnWowTimeKeeperThreadProc = NULL;
typedef DWORD64(*PWOW_GET_CPU_FREQUENCY)();
static PWOW_GET_CPU_FREQUENCY fnWowGetCPUFrequency = NULL;
typedef DWORD64(*PWOW_READ_TSC)();
static PWOW_READ_TSC fnWowReadTSC = NULL;
static PBOOL g_pWowUseTSC = NULL;
static PDWORD64 g_pWowTimerTicksPerSecond = NULL;
static double* g_pWowTimerToMilliseconds = NULL;
static double* g_pWowTimerOffset = NULL;

// Signature definitions
const uint8_t signatureTimeKeeper[] = { 0x8B, 0x0D, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x56, 0x8D, 0x64, 0x24 };
const char* maskTimeKeeper = "xx????x????xxxx";

const uint8_t signatureGetCPUFrequency[] = { 0xE8, 0x00, 0x00, 0x00, 0x00, 0x85, 0xC0, 0x74, 0x00, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x15, 0x00, 0x00, 0x00, 0x00, 0xC3 };
const char* maskGetCPUFrequency = "x????xxx?x????xx????x";

const uint8_t signatureReadTSC[] = { 0x0F, 0x31, 0xC3, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x55 };
const char* maskReadTSC = "xxxxxxxxxxxxxxxxx";

const uint8_t signatureUseTSC[] = { 0xA1, 0x00, 0x00, 0x00, 0x00, 0x85, 0xC0, 0x89, 0x45, 0x00, 0x75 };
const char* maskUseTSC = "x????xxxx?x";

const uint8_t signatureTimerTicksPerSecond[] = { 0x8B, 0x15, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x89, 0x45, 0x00, 0x89, 0x75 };
const char* maskTimerTicksPerSecond = "xx????xx?????xx?xx";

const uint8_t signatureTimerToMilliseconds[] = { 0xDC, 0x0D, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x05, 0x00, 0x00, 0x00, 0x00, 0xDD, 0x45 };
const char* maskTimerToMilliseconds = "xx????xx????xx";

const uint8_t signatureTimerOffset[] = { 0xDC, 0x05, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x8B, 0xE5 };
const char* maskTimerOffset = "xx????x????xx";

// Function to scan memory for a given signature
void* ScanMemory(void* baseAddress, size_t size, const uint8_t* signature, const char* mask, const char* name) {
	if (baseAddress == NULL || size == 0 || signature == NULL || mask == NULL) {
		return NULL;
	}

	uint8_t* memory = (uint8_t*)baseAddress;
	size_t signatureLength = strlen(mask);

	for (size_t i = 0; i <= size - signatureLength; ++i) {
		bool found = true;

		for (size_t j = 0; j < signatureLength; ++j) {
			if (mask[j] == 'x' && signature[j] != memory[i + j]) {
				found = false;
				break;
			}
		}

		if (found) {
			void* foundAddress = (void*)(memory + i);
			DebugOutputF("Found %s at address: 0x%08X\n", name, (uintptr_t)foundAddress);
			return foundAddress;
		}
	}

	DebugOutputF("Signature '%s' not found.\n", name);
	return NULL;
}

// Function to read a DWORD from a specific address
DWORD ReadDWORDFromAddress(void* address) {
	DWORD value = 0;
	memcpy(&value, address, sizeof(value));
	return value;
}

// Function to resolve the xref to find the actual address
void* ResolveXref(void* baseAddress, void* xrefAddress, int additionalOffset) {
	if (xrefAddress == NULL || baseAddress == NULL) {
		DebugOutputF("Invalid xref address or base address.\n");
		return NULL;
	}

	// Adjust the xrefAddress based on the additionalOffset
	void* adjustedXrefAddress = (BYTE*)xrefAddress + additionalOffset;

	// Read the 4 bytes from the adjusted xrefAddress
	DWORD addressOffset = ReadDWORDFromAddress(adjustedXrefAddress);

	// Debug output to verify
	DebugOutputF(L"Resolved xref address: xrefAddress=0x%08X, AddressOffset=0x%08X\n",
		(uintptr_t)xrefAddress, addressOffset);

	return addressOffset;
}

// Function to get module information
MODULEINFO GetModuleInfo(HMODULE hModule) {
	MODULEINFO moduleInfo = { 0 };
	GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo));
	return moduleInfo;
}

static inline void TimeSample(PLARGE_INTEGER pQpc, PDWORD64 pTsc) {
	// Try to request a new timeslice before sampling timestamp counters
	Sleep(0);

	QueryPerformanceCounter(pQpc);
	*pTsc = fnWowReadTSC();

	// Wait for QPC and RDTSC to finish
	_mm_lfence();
}

DWORD64 CalibrateTSC() {
	int oldPriority = GetThreadPriority(GetCurrentThread());
	// Pin on current core and run with high priority
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	SetThreadAffinityMask(GetCurrentThread(), 1 << (GetCurrentProcessorNumber()));

	// Obtain a baseline time for comparison
	LARGE_INTEGER baselineQpc;
	DWORD64 baselineTsc;
	TimeSample(&baselineQpc, &baselineTsc);

	Sleep(500);

	// Obtain new value after sleep
	LARGE_INTEGER diffQpc;
	DWORD64 diffTsc;
	TimeSample(&diffQpc, &diffTsc);

	// Calculate the relative time spent in sleep (>= 500 ms)
	diffQpc.QuadPart -= baselineQpc.QuadPart;
	diffTsc -= baselineTsc;

	LARGE_INTEGER performanceFrequency;
	QueryPerformanceFrequency(&performanceFrequency);

	double elapsedTime = diffQpc.QuadPart / (double)performanceFrequency.QuadPart;
	double estimatedTscFrequency = diffTsc / elapsedTime;

	// Restore old priority and affinity
	SetThreadPriority(GetCurrentThread(), oldPriority);
	SetThreadAffinityMask(GetCurrentThread(), 0);

	return (DWORD64)round(estimatedTscFrequency);
}

// Hooked OsTimeManager::TimeKeeper to apply our changes and prevent it from running in the background during gameplay
DWORD WINAPI VfTimeKeeperThreadProc(LPVOID lpParameter) {
	// Increase the process timer resolution up to a cap of 0.5 ms
	IncreaseTimerResolution(5000);

	// Disable Windows 11 power throttling for the process
	SetPowerThrottlingState(PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION, FALSE);
	SetPowerThrottlingState(PROCESS_POWER_THROTTLING_EXECUTION_SPEED, FALSE);

	// Calibrate the game timer using QueryPerformanceFrequency
	*g_pWowTimerTicksPerSecond = CalibrateTSC();
	DebugOutputF(L"timerTicksPerSecond=%lld", *g_pWowTimerTicksPerSecond);
	*g_pWowTimerToMilliseconds = 1000.0 / *g_pWowTimerTicksPerSecond;
	*g_pWowUseTSC = TRUE;

	// Align TSC with GTC
	*g_pWowTimerOffset = GetTickCount() - (fnWowReadTSC() * *g_pWowTimerToMilliseconds);
	DebugOutputF(L"timerOffset=%.0lf", *g_pWowTimerOffset);

	while (!g_mainThreadFinished) {
		Sleep(1);
	}

	DebugOutputF(L"Done! Unloading from process %lu...", GetCurrentProcessId());
	FreeLibraryAndExitThread(g_hSelf, 0);
}

typedef DWORD(*PLOAD)();

// Function to call load functions in DLLs loaded by the included launcher
void InitAdditionalDLLs() {
	LPWSTR pWowDirectory = malloc(MAX_PATH * sizeof(WCHAR));
	GetModuleFileName(NULL, pWowDirectory, MAX_PATH);
	// Remove the file name from the directory path
	PathRemoveFileSpec(pWowDirectory);

	LPWSTR pConfigPath = malloc(MAX_PATH * sizeof(WCHAR));
	PathCombine(pConfigPath, pWowDirectory, L"dlls.txt");

	// Obtain a list of any additional DLLs that should have been loaded by the launcher
	VF_DLL_LIST_PARSE_DATA dllListData = { 0 };
	LoaderParseConfig(pWowDirectory, pConfigPath, &dllListData);

	if (dllListData.pAdditionalDLLs) {
		for (int i = 0; i < dllListData.nAdditionalDLLs; i++) {
			// Is the DLL loaded into the process?
			HMODULE moduleHandle = GetModuleHandle(dllListData.pAdditionalDLLs[i]);

			if (moduleHandle) {
				DebugOutputF(L"Handle was found for %ls", dllListData.pAdditionalDLLs[i]);
				PLOAD pLoad = (PLOAD)GetProcAddress(moduleHandle, "Load");
				// Does this DLL export a load function?
				if (pLoad) {
					DWORD result = pLoad();
					DebugOutputF(L"Load function was found for %ls, result=%d", dllListData.pAdditionalDLLs[i], result);

					// Did the load function return an error?
					if (result) {
						MessageBoxF(L"DLL %ls was not loaded properly (load function failed)",
							dllListData.pAdditionalDLLs[i]);
					}
				}
			}
			else {
				MessageBoxF(L"DLL %ls was not loaded properly (no module handle)",
					dllListData.pAdditionalDLLs[i]);
			}

			free(dllListData.pAdditionalDLLs[i]);
		}

		free(dllListData.pAdditionalDLLs);
	}

	free(pConfigPath);
	free(pWowDirectory);
}

// Hooked function on main thread to perform initialization and prevent the game from hanging
DWORD64 VfGetCPUFrequency() {
	// The game has built-in UI scaling which means Windows DPI scaling is unnecessary
	// DXVK already enabled this by default (d3d9.dpiAware)
	EnableDPIAwareness();

	DebugOutputF(L"launcherFlags=%u", *g_pLauncherFlags);
	if (*g_pLauncherFlags & VF_LAUNCHER_FLAG_INIT_DLLS) {
		InitAdditionalDLLs();
	}
	*g_pLauncherFlags = 0;

	// Wait until all threads have finished
	while (!*g_pWowUseTSC) {
		Sleep(1);
	}

	g_mainThreadFinished = TRUE;

	// 1.12 assumes TSC frequency == CPU frequency
	return *g_pWowTimerTicksPerSecond;
}

// Initialization function for VanillaFixes
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	g_hSelf = hinstDLL;

	switch (fdwReason) {
	case DLL_PROCESS_ATTACH: {
		if (MH_Initialize() != MH_OK) {
			return FALSE;
		}

		// Scan for function signatures and setup pointers
		MODULEINFO modInfo = GetModuleInfo(GetModuleHandle(NULL));
		void* baseAddress = modInfo.lpBaseOfDll;
		size_t moduleSize = modInfo.SizeOfImage;

		// Initialize function pointers by scanning memory
		fnWowTimeKeeperThreadProc = (LPTHREAD_START_ROUTINE)ScanMemory(baseAddress, moduleSize, signatureTimeKeeper, maskTimeKeeper, "TimeKeeperThreadProc");
		fnWowGetCPUFrequency = (PWOW_GET_CPU_FREQUENCY)ScanMemory(baseAddress, moduleSize, signatureGetCPUFrequency, maskGetCPUFrequency, "GetCPUFrequency");
		fnWowReadTSC = (PWOW_READ_TSC)ScanMemory(baseAddress, moduleSize, signatureReadTSC, maskReadTSC, "ReadTSC");

		void*xrefAddress = ScanMemory(baseAddress, moduleSize, signatureUseTSC, maskUseTSC, "UseTSC");
		g_pWowUseTSC = (PBOOL)ResolveXref(baseAddress, xrefAddress, 1);

		xrefAddress = ScanMemory(baseAddress, moduleSize, signatureTimerTicksPerSecond, maskTimerTicksPerSecond, "TimerTicksPerSecond");
		g_pWowTimerTicksPerSecond = (PDWORD64)ResolveXref(baseAddress, xrefAddress, 2);

		xrefAddress = ScanMemory(baseAddress, moduleSize, signatureTimerToMilliseconds, maskTimerToMilliseconds, "TimerToMilliseconds");
		g_pWowTimerToMilliseconds = (double*)ResolveXref(baseAddress, xrefAddress, 2);

		xrefAddress = ScanMemory(baseAddress, moduleSize, signatureTimerOffset, maskTimerOffset, "TimerOffset");
		g_pWowTimerOffset = (double*)ResolveXref(baseAddress, xrefAddress, 2);

		// Hook OsTimeManager::TimeKeeper before the thread is created
		if (MH_CreateHook(fnWowTimeKeeperThreadProc, &VfTimeKeeperThreadProc, NULL) != MH_OK) {
			return FALSE;
		}
		// Hook function on the main thread to perform extra tasks
		if (MH_CreateHook(fnWowGetCPUFrequency, &VfGetCPUFrequency, NULL) != MH_OK) {
			return FALSE;
		}

		if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
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
