#pragma once

#include <windows.h>

// Utility function to get the last error message
LPWSTR GetLastErrorMessage();

// Set process DPI awareness using SetProcessDpiAwarenessContext (if available) or SetProcessDPIAware
void EnableDPIAwareness();

// Use NtSetTimerResolution (if available) or timeBeginPeriod to increase process timer resolution
BOOL IncreaseTimerResolution(ULONG maxRequestedResolution);

// Set power throttling states using SetProcessInformation
BOOL SetPowerThrottlingState(ULONG feature, BOOL enable);
