#include <windows.h>
#include <psapi.h>

#include "memory.h"
#include "macros.h"

// Signatures contributed by Daribon
// Example for 1.12.1:
// TimeKeeperThreadProc at 0x0042b9c0
// GetCPUFrequency at 0x0042c060
// UseTSC at 0x00884c80
// TimerTicksPerSecond at 0x008332c0
// TimerToMilliseconds at 0x008332c8
// TimerOffset at 0x00884c88

static BYTE g_sigTimeKeeperThreadProc[] = { 0x8B, 0x0D, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x56, 0x8D, 0x64, 0x24 };
static ULONG g_maskTimeKeeperThreadProc = 0b110000100001111;

static BYTE g_sigGetCPUFrequency[] = { 0xE8, 0x00, 0x00, 0x00, 0x00, 0x85, 0xC0, 0x74, 0x00, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x15, 0x00, 0x00, 0x00, 0x00, 0xC3 };
static ULONG g_maskGetCPUFrequency = 0b100001110100001100001;

static BYTE g_sigUseTSC[] = { 0xA1, 0x00, 0x00, 0x00, 0x00, 0x85, 0xC0, 0x89, 0x45, 0x00, 0x75 };
static ULONG g_maskUseTSC = 0b10000111101;
static int g_xrefOffsetUseTSC = 1;

static BYTE g_sigTimerTicksPerSecond[] = { 0x8B, 0x15, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x89, 0x45, 0x00, 0x89, 0x75 };
static ULONG g_maskTimerTicksPerSecond = 0b110000110000011011;
static int g_xrefOffsetTimerTicksPerSecond = 2;

static BYTE g_sigTimerToMilliseconds[] = { 0xDC, 0x0D, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x05, 0x00, 0x00, 0x00, 0x00, 0xDD, 0x45 };
static ULONG g_maskTimerToMilliseconds = 0b11000011000011;
static int g_xrefOffsetTimerToMilliseconds = 2;

static BYTE g_sigTimerOffset[] = { 0xDC, 0x05, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x8B, 0xE5 };
static ULONG g_maskTimerOffset = 0b1100001000011;
static int g_xrefOffsetTimerOffset = 2;

typedef struct {
	ULONG startAddress;
	ULONG endAddress;

	PBYTE pPattern;
	int patternLength;
	ULONG mask;
} VF_SIGNATURE, *PVF_SIGNATURE;

// Search for a signature in memory from startAddress to endAddress
BOOL SignatureSearch(PVF_SIGNATURE pSignature, PVOID* pResult) {
	PBYTE pStart = (PBYTE)pSignature->startAddress;
	PBYTE pEnd = (PBYTE)(pSignature->endAddress - pSignature->patternLength);

	for(PBYTE pCurrent = pStart; pCurrent != pEnd; pCurrent++) {
		for(int offset = 0; offset < pSignature->patternLength; offset++) {
			int marker = 1 << (pSignature->patternLength - offset - 1);

			if(pSignature->mask & marker) {
				if(pCurrent[offset] != pSignature->pPattern[offset]) {
					break;
				}
			}

			if(offset == pSignature->patternLength - 1) {
				*pResult = pCurrent;
				return TRUE;
			}
		}
	}

	return FALSE;
}

PVOID ResolveXRef(PDWORD pXrefAddress, int offset) {
	PDWORD pAdjustedXRef = (PDWORD)((PBYTE)pXrefAddress + offset);
	return (PVOID)*pAdjustedXRef;
}

// Scan memory for addresses required by VanillaFixes
BOOL ScanMemory(PVF_ADDRESSES pResult) {
	// Get module information for the game executable
	MODULEINFO gameExecutableInfo;
	if(!GetModuleInformation(GetCurrentProcess(), GetModuleHandle(NULL), &gameExecutableInfo, sizeof(MODULEINFO))) {
		return FALSE;
	}

	VF_SIGNATURE sig;
	sig.startAddress = (ULONG)gameExecutableInfo.lpBaseOfDll;
	sig.endAddress = sig.startAddress + gameExecutableInfo.SizeOfImage;

	// Search for TimeKeeperThreadProc
	sig.pPattern = g_sigTimeKeeperThreadProc;
	sig.patternLength = sizeof(g_sigTimeKeeperThreadProc);
	sig.mask = g_maskTimeKeeperThreadProc;

	if(!SignatureSearch(&sig, (PVOID*)&pResult->pTimeKeeperThreadProc)) {
		DebugOutputF(L"Signature search failed: Could not find pTimeKeeperThreadProc");
		return FALSE;
	}

	// Search for GetCPUFrequency
	sig.pPattern = g_sigGetCPUFrequency;
	sig.patternLength = sizeof(g_sigGetCPUFrequency);
	sig.mask = g_maskGetCPUFrequency;

	if(!SignatureSearch(&sig, (PVOID*)&pResult->pGetCPUFrequency)) {
		DebugOutputF(L"Signature search failed: Could not find pGetCPUFrequency");
		return FALSE;
	}

	// Search for UseTSC reference
	PDWORD pUseTSCXRef;
	sig.pPattern = g_sigUseTSC;
	sig.patternLength = sizeof(g_sigUseTSC);
	sig.mask = g_maskUseTSC;

	if(!SignatureSearch(&sig, (PVOID*)&pUseTSCXRef)) {
		DebugOutputF(L"Signature search failed: Could not find UseTSC reference");
		return FALSE;
	}

	// Resolve UseTSC reference
	pResult->pUseTSC = ResolveXRef(pUseTSCXRef, g_xrefOffsetUseTSC);

	// Search for TimerTicksPerSecond reference
	PDWORD pTimerTicksPerSecondXRef;
	sig.pPattern = g_sigTimerTicksPerSecond;
	sig.patternLength = sizeof(g_sigTimerTicksPerSecond);
	sig.mask = g_maskTimerTicksPerSecond;

	if(!SignatureSearch(&sig, (PVOID*)&pTimerTicksPerSecondXRef)) {
		DebugOutputF(L"Signature search failed: Could not find TimerTicksPerSecond reference");
		return FALSE;
	}

	// Resolve TimerTicksPerSecond reference
	pResult->pTimerTicksPerSecond = ResolveXRef(pTimerTicksPerSecondXRef, g_xrefOffsetTimerTicksPerSecond);

	// Search for TimerToMilliseconds reference
	PDWORD pTimerToMillisecondsXRef;
	sig.pPattern = g_sigTimerToMilliseconds;
	sig.patternLength = sizeof(g_sigTimerToMilliseconds);
	sig.mask = g_maskTimerToMilliseconds;

	if(!SignatureSearch(&sig, (PVOID*)&pTimerToMillisecondsXRef)) {
		DebugOutputF(L"Signature search failed: Could not find TimerToMilliseconds reference");
		return FALSE;
	}

	// Resolve TimerToMilliseconds reference
	pResult->pTimerToMilliseconds = ResolveXRef(pTimerToMillisecondsXRef, g_xrefOffsetTimerToMilliseconds);

	// Search for TimerOffset reference
	PDWORD pTimerOffsetXRef;
	sig.pPattern = g_sigTimerOffset;
	sig.patternLength = sizeof(g_sigTimerOffset);
	sig.mask = g_maskTimerOffset;

	if(!SignatureSearch(&sig, (PVOID*)&pTimerOffsetXRef)) {
		DebugOutputF(L"Signature search failed: Could not find TimerOffset reference");
		return FALSE;
	}

	// Resolve TimerOffset reference
	pResult->pTimerOffset = ResolveXRef(pTimerOffsetXRef, g_xrefOffsetTimerOffset);
	return TRUE;
}
