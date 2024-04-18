#include <windows.h>

#include <mmsystem.h>
#include <processthreadsapi.h>
#include <winternl.h>

#include "offsets_1_12_1.h"

typedef NTSTATUS (NTAPI *PNT_QUERY_TIMER_RESOLUTION)(PULONG, PULONG, PULONG);
typedef NTSTATUS (NTAPI *PNT_SET_TIMER_RESOLUTION)(ULONG, BOOLEAN, PULONG);

// Use NtSetTimerResolution (if available) or timeBeginPeriod to increase process timer resolution
static BOOL IncreaseTimerResolution(ULONG maxRequestedResolution) {
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
			DebugOutput(L"VanillaFixes: Using timeBeginPeriod (desired=%u)", resolution);
			return timeBeginPeriod(resolution) == TIMERR_NOERROR;
		}
	}

	ULONG minSupportedResolution, maxSupportedResolution, currentResolution;
	fnNtQueryTimerResolution(&minSupportedResolution, &maxSupportedResolution, &currentResolution);

	ULONG desiredResolution = max(maxSupportedResolution, maxRequestedResolution);
	DebugOutput(L"VanillaFixes: Using NT timer functions (min=%lu max=%lu current=%lu desired=%lu)",
		minSupportedResolution, maxSupportedResolution, currentResolution, desiredResolution);

	if(fnNtSetTimerResolution(desiredResolution, TRUE, &currentResolution) != 0) {
		DebugOutput(L"VanillaFixes: NtSetTimerResolution failed!");

		// Fall back to timeBeginPeriod if NtSetTimerResolution failed
		UINT resolution = max(1, maxRequestedResolution / 10000);
		DebugOutput(L"VanillaFixes: Using timeBeginPeriod (desired=%u)", resolution);
		return timeBeginPeriod(resolution) == TIMERR_NOERROR;
	}

	return TRUE;
}

inline void CpuTimeSample(PLARGE_INTEGER pQpc, PDWORD64 pTsc) {
	// Try to request a new timeslice before sampling timestamp counters
	Sleep(0);

	QueryPerformanceCounter(pQpc);
	*pTsc = fnWowReadTSC();

	// Wait for QPC and RDTSC to finish
	_mm_lfence();
}

DWORD64 CpuCalibrateTsc() {
	int oldPriority = GetThreadPriority(GetCurrentThread());
	// Pin on current core and run with high priority
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	SetThreadAffinityMask(GetCurrentThread(), 1 << (GetCurrentProcessorNumber()));

	LARGE_INTEGER baselineQpc;
	DWORD64 baselineTsc;
	CpuTimeSample(&baselineQpc, &baselineTsc);

	Sleep(500);

	LARGE_INTEGER diffQpc;
	DWORD64 diffTsc;
	CpuTimeSample(&diffQpc, &diffTsc);

	// Calculate the relative time spent in sleep (>= 500 ms)
	diffQpc.QuadPart -= baselineQpc.QuadPart;
	diffTsc -= baselineTsc;

	LARGE_INTEGER performanceFrequency;
	QueryPerformanceFrequency(&performanceFrequency);

	double elapsedTime = diffQpc.QuadPart / (double)performanceFrequency.QuadPart;
	double estimatedTscFrequency = diffTsc / elapsedTime;

	SetThreadPriority(GetCurrentThread(), oldPriority);
	SetThreadAffinityMask(GetCurrentThread(), 0);

	return estimatedTscFrequency;
}
