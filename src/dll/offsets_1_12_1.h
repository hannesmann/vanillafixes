// This file contains offsets for client version 1.12.1 (5875)
#pragma once

#include <windows.h>

// OsTimeManager::TimeKeeper
static LPTHREAD_START_ROUTINE fnWowTimeKeeperThreadProc = (LPTHREAD_START_ROUTINE)0x0042b9c0;

typedef DWORD64 (*PWOW_GET_CPU_FREQUENCY)();
// This function is used for detecting hardware changes and will hang if TimeKeeper is killed
static PWOW_GET_CPU_FREQUENCY fnWowGetCPUFrequency = (PWOW_GET_CPU_FREQUENCY)0x0042c060;

typedef DWORD64 (*PWOW_READ_TSC)();
static PWOW_READ_TSC fnWowReadTSC = (PWOW_READ_TSC)0x004293d0;

// 0 => OsGetAsyncTime uses GetTickCount
// 1 => OsGetAsyncTime uses RDTSC
static PBOOL g_pWowUseTSC = (PBOOL)0x00884c80;

// g_pWowUseTSC = 0 => 1000
// g_pWowUseTSC = 1 => TSC frequency of processor
static PDWORD64 g_pWowTimerTicksPerSecond = (PDWORD64)0x008332c0;

// Used by OsGetAsyncTimeMs to normalize value from OsGetAsyncTime
static double* g_pWowTimerToMilliseconds = (double*)0x008332c8;
// Used by OsGetAsyncTimeMs to align the two timing sources
static double* g_pWowTimerOffset = (double*)0x00884c88;
