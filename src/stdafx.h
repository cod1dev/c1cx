#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#define _MB_TITLE "CoDExtended"

#ifdef PATCH_1_1
#define CURL_STATICLIB
#endif

#include <windows.h>
#include <ctime>
#include <map>
#include "mem_util.h"

#define QDECL __cdecl
#define DLL_EXPORT __declspec(dllexport)
#define DLL_IMPORT __declspec(dllimport)
typedef unsigned char byte;

extern int codversion;
typedef enum
{
	COD1_1_1_MP,
	COD1_1_1_SP,
	COD1_1_5_MP,
	COD1_1_5_SP
} cod_v;

#ifdef PATCH_1_1
static void(*Com_Quit_f)() = (void(*)())0x435D80;
#elif PATCH_1_5
static void(*Com_Quit_f)() = (void(*)())0x00438220;
#endif

static void MsgBox(const char* msg)
{
	MessageBoxA(NULL, msg, _MB_TITLE, MB_OK | MB_ICONINFORMATION);
}

/*
static bool CopyToClipboard(const char *s)
{
	if (OpenClipboard(NULL))
	{
		HGLOBAL clipbuffer;
		char* buffer;
		EmptyClipboard();
		clipbuffer = GlobalAlloc(GMEM_DDESHARE, strlen(s) + 1);
		buffer = (char*)GlobalLock(clipbuffer);
		strcpy(buffer, s);
		GlobalUnlock(clipbuffer);
		SetClipboardData(CF_TEXT, clipbuffer);
		CloseClipboard();
	}
	else
		return false;
	return true;
}*/

static bool is_addr_safe(size_t addr)
{
	__try
	{
		*(unsigned char*)(addr);
	}
	__except (1)
	{
		return false;
	}
	return true;
}

template <typename T, typename ... Ts> T call(size_t addr, Ts ... ts);
template <typename T, typename ... Ts>
T call(size_t addr, Ts ... ts) {
	T(*f)(...);
	*(T*)&f = (T)addr;
	return f(ts...);
}