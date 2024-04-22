#include <windows.h>
#include <intrin.h>
#include <math.h>
#include <processthreadsapi.h>

#include "tsc.h"
#include "offsets_1_12_1.h"

inline void TimeSample(PLARGE_INTEGER pQpc, PDWORD64 pTsc) {
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
