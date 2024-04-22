#include <windows.h>
#include <shlwapi.h>
#include <d3d9.h>

#include "selftest.h"
#include "macros.h"

static BOOL IsUsingBundledDXVK(LPCWSTR pModulePath) {
	LPWSTR pD3D9Path = malloc(MAX_PATH * sizeof(WCHAR));
	PathCombine(pD3D9Path, pModulePath, L"d3d9.dll");
	BOOL result = GetFileAttributes(pD3D9Path) != INVALID_FILE_ATTRIBUTES;
	free(pD3D9Path);
	return result;
}

VF_SELFTEST_RESULT RunSelfTest(LPCWSTR pModulePath) {
	// Check if self-test should run before spawning a new process
	if(!IsUsingBundledDXVK(pModulePath)) {
		return VF_SELFTEST_FAIL_REASON_SYSTEM_D3D9;
	}

	LPWSTR pVfExePath = malloc(MAX_PATH * sizeof(WCHAR));
	PathCombine(pVfExePath, pModulePath, L"VanillaFixes.exe");
	PathQuoteSpaces(pVfExePath);

	STARTUPINFO startupInfo = {0};
	PROCESS_INFORMATION processInfo = {0};
	DWORD flags = CREATE_UNICODE_ENVIRONMENT;

	// Prepare command line arguments for VanillaFixes
	int nCmdLine = MAX_PATH + 1 + wcslen(VF_SELFTEST_ARG) + 1;
	LPWSTR pCmdLine = malloc(nCmdLine * sizeof(WCHAR));
	swprintf(pCmdLine, nCmdLine, "%ls " VF_SELFTEST_ARG, pVfExePath);

	BOOL processCreated =
		CreateProcess(NULL, pCmdLine, NULL, NULL, FALSE, flags, NULL, NULL, &startupInfo, &processInfo);
	free(pCmdLine);

	if(!processCreated) {
		free(pVfExePath);
		return VF_SELFTEST_FAIL_REASON_CREATEPROCESS;
	}

	// Wait for the created process
	WaitForSingleObject(processInfo.hProcess, 10000);

	DWORD processResult = 0;
	// Retrieve the termination status of the process
	GetExitCodeProcess(processInfo.hProcess, &processResult);

	// If the self-test takes too long, terminate it
	if(processResult == STILL_ACTIVE) {
		TerminateProcess(processInfo.hProcess, VF_SELFTEST_FAIL_REASON_STILL_ACTIVE);
		processResult = (DWORD)VF_SELFTEST_FAIL_REASON_STILL_ACTIVE;
	}

	// Close process handles once they're no longer needed
	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);

	free(pVfExePath);

	return (VF_SELFTEST_RESULT)processResult;
}

int WINAPI SelfTestMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	LPWSTR pWowDirectory = malloc(MAX_PATH * sizeof(WCHAR));
	GetModuleFileName(NULL, pWowDirectory, MAX_PATH);
	// Check if the buffer was large enough
	AssertMessageBoxF(GetLastError() != ERROR_INSUFFICIENT_BUFFER, L"GetLastError() == ERROR_INSUFFICIENT_BUFFER");
	// Remove the file name from the directory path
	PathRemoveFileSpec(pWowDirectory);

	VF_SELFTEST_RESULT result = TestDXVK(pWowDirectory);
	DebugOutputF(L"Self-test result: %ls", TestResultToStr(result));
	return result;
}

typedef LPDIRECT3D9 (WINAPI *PDIRECT3D_CREATE_9)(UINT);

VF_SELFTEST_RESULT TestDXVK(LPCWSTR pModulePath) {
	LPWSTR pD3D9Path = malloc(MAX_PATH * sizeof(WCHAR));
	PathCombine(pD3D9Path, pModulePath, L"d3d9.dll");

	HMODULE hD3D9 = LoadLibrary(pD3D9Path);
	// d3d9.dll exists but cannot be loaded, probably because of missing vulkan-1.dll
	if(!hD3D9) {
		return VF_SELFTEST_FAIL_REASON_D3D9_NOT_LOADED;
	}

	PDIRECT3D_CREATE_9 pDirect3DCreate9 =
		(PDIRECT3D_CREATE_9)GetProcAddress(hD3D9, "Direct3DCreate9");
	if(!pDirect3DCreate9) {
		return VF_SELFTEST_FAIL_REASON_D3DCREATE9_NOT_FOUND;
	}

	LPDIRECT3D9 pD3D9 = pDirect3DCreate9(D3D_SDK_VERSION);
	// Direct3D init failed, probably because of missing Vulkan 1.3
	if(!pD3D9) {
		return VF_SELFTEST_FAIL_REASON_D3DCREATE9_FAILED;
	}

	UINT adapters = IDirect3D9_GetAdapterCount(pD3D9);

	IDirect3D9_Release(pD3D9);
	FreeLibrary(hD3D9);

	// DXVK is functional if one or more adapters were found
	return adapters > 0 ? VF_SELFTEST_SUCCESS : VF_SELFTEST_FAIL_REASON_NO_ADAPTERS;
}

LPCWSTR TestResultToStr(VF_SELFTEST_RESULT result) {
	switch(result) {
		case VF_SELFTEST_SUCCESS: return L"Success";

		case VF_SELFTEST_FAIL_REASON_CREATEPROCESS: return L"Could not create process";
		case VF_SELFTEST_FAIL_REASON_STILL_ACTIVE: return L"Process still active";

		case VF_SELFTEST_FAIL_REASON_SYSTEM_D3D9: return L"System D3D9 is in use";
		case VF_SELFTEST_FAIL_REASON_D3D9_NOT_LOADED: return L"d3d9.dll could not be loaded";
		case VF_SELFTEST_FAIL_REASON_D3DCREATE9_NOT_FOUND: return L"Direct3DCreate9 was not found in d3d9.dll";
		case VF_SELFTEST_FAIL_REASON_D3DCREATE9_FAILED: return L"Direct3DCreate9 failed";
		case VF_SELFTEST_FAIL_REASON_NO_ADAPTERS: return L"No adapters";
	}

	// If we have an unknown result, the process was terminated by an external exception
	return L"Exception caught";
}
