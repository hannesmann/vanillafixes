
// Global shared data variable and remote data pointer
static VF_SHARED_DATA g_sharedData = { 0 };
static PVF_SHARED_DATA g_pRemoteData = NULL;

// Function to synchronize shared data with the target process
BOOL RemoteSyncData(HANDLE hTargetProcess)
{
	// If the remote data pointer has not been allocated yet
	if (!g_pRemoteData)
	{
		// Allocate memory in the target process for the shared data structure
		g_pRemoteData = VirtualAllocEx(
			hTargetProcess,
			NULL,
			sizeof(VF_SHARED_DATA),
			MEM_RESERVE | MEM_COMMIT,
			PAGE_READWRITE
		);

		// Check if VirtualAllocEx was successful
		if (g_pRemoteData == NULL)
		{
			return FALSE;
		}

		// Write the address of the shared data structure to the target process's memory
		if (!WriteProcessMemory(hTargetProcess, g_pSharedData, &g_pRemoteData, sizeof(PVF_SHARED_DATA*), NULL))
		{
			return FALSE;
		}
	}

	// Write the shared data to the target process's memory
	if (!WriteProcessMemory(hTargetProcess, g_pRemoteData, &g_sharedData, sizeof(VF_SHARED_DATA), NULL))
	{
		return FALSE;
	}

	return TRUE;
}

// Function to load a DLL into the target process
int RemoteLoadLibrary(LPWSTR pDllPath, HANDLE hTargetProcess)
{
	// Copy the DLL path to the shared data structure
	StrCpy(g_sharedData.pDllPath, pDllPath);

	// Assert that the shared data was successfully written to the target process's memory
	AssertMessageBox(RemoteSyncData(hTargetProcess), L"Failed to write process memory (pDllPath)");

	// Create a new thread in the target process that will load the DLL
	HANDLE hThread =
		CreateRemoteThread(hTargetProcess, NULL, 0, (LPTHREAD_START_ROUTINE)&LoadLibraryW, &g_pRemoteData->pDllPath, 0, NULL);

	// Assert that the thread was successfully created
	AssertMessageBox(hThread, L"Failed to create remote thread");

	// Wait for the created thread to terminate
	WaitForSingleObject(hThread, 10000);

	DWORD threadResult = 0;
	// Retrieve the termination status of the thread
	GetExitCodeThread(hThread, &threadResult);

	// Close the thread handle
	CloseHandle(hThread);

	// Assert that the thread terminated successfully and the DLL was loaded
	AssertMessageBox(threadResult && threadResult != STILL_ACTIVE,
		L"DllMain failed after injection.\r\n\r\n"
		L"Make sure you have a compatible game client."
	);

	return 0;
}
