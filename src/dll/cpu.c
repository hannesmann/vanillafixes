#include "offsets.h"

inline void CpuTimeSample(PLARGE_INTEGER pQpc, PDWORD64 pTsc) {
	/* Always request a new timeslice before sampling */
	Sleep(0);

	QueryPerformanceCounter(pQpc);
	*pTsc = fnWowReadTsc();

	/* Wait for QPC and RDTSC to finish */
	_mm_lfence();
}

DWORD64 CpuCalibrateTsc() {
	int oldPriority = GetThreadPriority(GetCurrentThread());
	/* Pin on current core and run with high priority */
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	SetThreadAffinityMask(GetCurrentThread(), 1 << (GetCurrentProcessorNumber()));

	LARGE_INTEGER baselineQpc;
	DWORD64 baselineTsc;
	CpuTimeSample(&baselineQpc, &baselineTsc);

	Sleep(500);

	LARGE_INTEGER diffQpc;
	DWORD64 diffTsc;
	CpuTimeSample(&diffQpc, &diffTsc);

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
