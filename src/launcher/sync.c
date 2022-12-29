static VF_SHARED_DATA g_sharedData = {0};
static PVF_SHARED_DATA g_pRemoteData = NULL;

BOOL RemoteSyncData(HANDLE hTargetProcess) {
	if(!g_pRemoteData) {
		g_pRemoteData = VirtualAllocEx(
			hTargetProcess,
			NULL,
			sizeof(VF_SHARED_DATA),
			MEM_RESERVE | MEM_COMMIT,
			PAGE_READWRITE
		);

		WriteProcessMemory(hTargetProcess, g_pSharedPtr, &g_pRemoteData, sizeof(PVF_SHARED_DATA*), NULL);
	}

	return WriteProcessMemory(hTargetProcess, g_pRemoteData, &g_sharedData, sizeof(VF_SHARED_DATA), NULL);
}

int RemoteLoadLibrary(LPWSTR pDllPath, HANDLE hTargetProcess) {
	StrCpy(g_sharedData.pDllPath, pDllPath);
	AssertMessageBox(RemoteSyncData(hTargetProcess), L"Failed to write process memory (pDllPath)");

	HANDLE hThread =
		CreateRemoteThread(hTargetProcess, NULL, 0, (LPTHREAD_START_ROUTINE)&LoadLibraryW, &g_pRemoteData->pDllPath, 0, NULL);
	AssertMessageBox(hThread, L"Failed to create remote thread");

	WaitForSingleObject(hThread, 10000);

	DWORD threadResult = 0;
	GetExitCodeThread(hThread, &threadResult);

	CloseHandle(hThread);

	AssertMessageBox(threadResult && threadResult != STILL_ACTIVE,
		L"DllMain failed after injection.\n\n"
		L"Make sure you have a compatible game client."
	);

	return 0;
}