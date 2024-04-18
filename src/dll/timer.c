#include <windows.h>

#include <mmsystem.h>
#include <processthreadsapi.h>
#include <winternl.h>

#include "offsets_1_12_1.h"

// Use timeBeginPeriod to increase process timer resolution
static BOOL IncreaseTimerResolution(ULONG maxRequestedResolution) {
	UINT resolution = max(1, maxRequestedResolution / 10000);
	DebugOutput(L"VanillaFixes: Using timeBeginPeriod to increase timer resolution (desired=%u)", resolution);
	return timeBeginPeriod(resolution) == TIMERR_NOERROR;
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
