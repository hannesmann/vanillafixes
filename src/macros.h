#pragma once

#include <windows.h>
#include <stdio.h>

#define MSGBUF_CHARS 512
// Scratch buffer for debug/error messages
static WCHAR g_msgBuffer[MSGBUF_CHARS] = {0};

#define MessageBoxF(...) \
	swprintf(g_msgBuffer, MSGBUF_CHARS, __VA_ARGS__); \
	MessageBox(NULL, g_msgBuffer, L"VanillaFixes", MB_OK | MB_ICONERROR);

#define AssertMessageBoxF(condition, ...) \
	if(!(condition)) { \
		swprintf(g_msgBuffer, MSGBUF_CHARS, __VA_ARGS__); \
		return MessageBox(NULL, g_msgBuffer, L"VanillaFixes", MB_OK | MB_ICONERROR); \
	}

#define DebugOutputF(...) \
	swprintf(g_msgBuffer, MSGBUF_CHARS, L"VanillaFixes: " __VA_ARGS__); \
	OutputDebugString(g_msgBuffer);
