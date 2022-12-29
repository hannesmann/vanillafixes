#pragma once

/* OsTimeManager::TimeKeeper */
static LPTHREAD_START_ROUTINE fnWowTimeKeeperThreadProc = (LPTHREAD_START_ROUTINE)0x0042b9c0;

/* This function is only used for detecting hardware changes */
typedef DWORD64 (*PWOW_HW_GET_CPU_FREQUENCY)();
static PWOW_HW_GET_CPU_FREQUENCY fnWowHwGetCpuFrequency = (PWOW_HW_GET_CPU_FREQUENCY)0x0042c060;

typedef DWORD64 (*PWOW_READ_TSC)();
static PWOW_READ_TSC fnWowReadTsc = (PWOW_READ_TSC)0x004293d0;

/*
 * 0 => OsGetAsyncTime uses GetTickCount
 * 1 => OsGetAsyncTime uses RDTSC
*/
static PBOOL g_pWowUseTsc = (PBOOL)0x00884c80;

/*
 * g_pWowUseTsc = 0 => 1000
 * g_pWowUseTsc = 1 => TSC frequency of processor
 */
static PDWORD64 g_pWowTimerTicksPerSecond = (PDWORD64)0x008332c0;

/* Used by OsGetAsyncTimeMs to normalize value from OsGetAsyncTime */
static double* g_pWowTimerToMilliseconds = (double*)0x008332c8;
/* Used by OsGetAsyncTimeMs to align the two timing sources */
static double* g_pWowTimerOffset = (double*)0x00884c88;