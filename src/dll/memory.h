#pragma once

#include <windows.h>

typedef DWORD64 (*PWOW_GET_CPU_FREQUENCY)();

typedef struct {
	// OsTimeManager::TimeKeeper
	LPTHREAD_START_ROUTINE pTimeKeeperThreadProc;
	// Used by the game to determine the TSC frequency
	PWOW_GET_CPU_FREQUENCY pGetCPUFrequency;
	// 0 => OsGetAsyncTime uses GetTickCount
	// 1 => OsGetAsyncTime uses RDTSC
	PBOOL pUseTSC;
	// g_pWowUseTSC = 0 => 1000
	// g_pWowUseTSC = 1 => TSC frequency of processor
	PDWORD64 pTimerTicksPerSecond;
	// Used by OsGetAsyncTimeMs to normalize value from OsGetAsyncTime
	double* pTimerToMilliseconds;
	// Used by OsGetAsyncTimeMs to align GetTickCount and RDTSC
	double* pTimerOffset;
} VF_ADDRESSES, *PVF_ADDRESSES;

BOOL ScanMemory(PVF_ADDRESSES pResult);
