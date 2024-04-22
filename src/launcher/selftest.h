#pragma once

#include <windows.h>

#define VF_SELFTEST_ARG L"-selftest"

typedef enum {
	VF_SELFTEST_SUCCESS = 0,

	VF_SELFTEST_FAIL_REASON_CREATEPROCESS = 0x1000,
	VF_SELFTEST_FAIL_REASON_STILL_ACTIVE = 0x1001,

	VF_SELFTEST_FAIL_REASON_SYSTEM_D3D9 = 0x1002,
	VF_SELFTEST_FAIL_REASON_D3D9_NOT_LOADED = 0x1003,
	VF_SELFTEST_FAIL_REASON_D3DCREATE9_NOT_FOUND = 0x1004,
	VF_SELFTEST_FAIL_REASON_D3DCREATE9_FAILED = 0x1005,
	VF_SELFTEST_FAIL_REASON_NO_ADAPTERS = 0x1006
} VF_SELFTEST_RESULT, *PVF_SELFTEST_RESULT;

// Run the launcher as a self-test executable to see if DXVK is working
VF_SELFTEST_RESULT RunSelfTest(LPCWSTR pModulePath);

// Main function if VanillaFixes has been launched with -selftest
int WINAPI SelfTestMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow);

// Run a test to see if DXVK is functional
VF_SELFTEST_RESULT TestDXVK(LPCWSTR pModulePath);

// Translate VF_SELFTEST_RESULT to a descriptive string
LPCWSTR TestResultToStr(VF_SELFTEST_RESULT result);
