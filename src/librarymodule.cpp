#include "shared.h"
#pragma comment(lib, "libs/detours/detours.lib")
#include "libs/detours/detours.h"

HMODULE(WINAPI *orig_LoadLibraryA)(LPCSTR lpFileName);
HMODULE WINAPI hLoadLibraryA(LPSTR lpFileName)
{
	HMODULE hModule = orig_LoadLibraryA(lpFileName);
	DWORD pBase = (DWORD)GetModuleHandle(lpFileName);

	if (!pBase)
		return hModule;

	void Main_UnprotectModule(HMODULE hModule);
		Main_UnprotectModule(hModule);

	if (strstr(lpFileName, "ui_mp"))
	{
		if (codversion != COD1_1_1_MP && codversion != COD1_1_5_MP)
			return hModule;

		void UI_Init(DWORD);
		UI_Init(pBase);
	}
	else if (strstr(lpFileName, "cgame_mp"))
	{
		if (codversion != COD1_1_1_MP && codversion != COD1_1_5_MP)
			return hModule;

		void CG_Init(DWORD);
		CG_Init(pBase);
	}
	else if (strstr(lpFileName, "game_mp"))
	{
		void G_Init(DWORD);
		G_Init(pBase);
	}

#ifdef DEBUG
	Com_Printf("hLoadLibraryA: ^2lpFileName = %s\n", lpFileName);
#endif
	return hModule;
}

extern DWORD cgame_mp;
BOOL(WINAPI* orig_FreeLibrary)(HMODULE hModule);
BOOL WINAPI hFreeLibrary(HMODULE hModule)
{
	CHAR szFileName[MAX_PATH];
	if (GetModuleFileName(hModule, szFileName, sizeof(szFileName)))
	{
		if (strstr(szFileName, "cgame_mp"))
			cgame_mp = 0;
#ifdef DEBUG
		Com_Printf("hFreeLibrary: ^szFileName = %s\n", szFileName);
#endif
	}
	return orig_FreeLibrary(hModule);
}

void patch_opcode_loadlibrary(void)
{
	orig_LoadLibraryA = (struct HINSTANCE__* (__stdcall*)(const char*)) \
		DetourFunction((LPBYTE)LoadLibraryA, (LPBYTE)hLoadLibraryA);
}

void patch_opcode_freelibrary(void)
{
	orig_FreeLibrary = (BOOL(WINAPI*)(HMODULE)) \
		DetourFunction((LPBYTE)FreeLibrary, (LPBYTE)hFreeLibrary);
}