#ifndef OFFSETS_H
#define OFFSETS_H

#include <windows.h>

LPTHREAD_START_ROUTINE pWowTimeKeeperThreadProc = (LPTHREAD_START_ROUTINE)0x0042b9c0;
LPVOID pWowGetTicksPerSecond = (LPVOID)0x0042c060;

PBOOL pWowUseTsc = (PBOOL)0x00884c80;
PDWORD64 pWowTscTicksPerSecond = (PDWORD64)0x008332c0;

double* pWowTscToMilliseconds = (double*)0x008332c8;
double* pWowTimerOffset = (double*)0x00884c88;

typedef DWORD64 (*WowReadTsc_t)();
WowReadTsc_t WowReadTsc = (WowReadTsc_t)0x004293d0;

#endif