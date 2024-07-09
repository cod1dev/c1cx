#include <windows.h>
#include <ctime>
#include "hooking.h"
#include "cl.h"
#include "shared.h"
#pragma comment(lib, "vendor/detours/detours.lib")
#include "../vendor/detours/detours.h"

#ifdef DEBUG
extern HANDLE hLogFile = INVALID_HANDLE_VALUE;
#endif
int codversion;
typedef enum
{
	COD1_1_1_MP,
	COD1_1_1_SP
} cod_v;

void Main_UnprotectModule(HMODULE hModule)
{
	PIMAGE_DOS_HEADER header = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((DWORD)hModule + header->e_lfanew);

	SIZE_T size = ntHeader->OptionalHeader.SizeOfImage;
	DWORD oldProtect;
	VirtualProtect((LPVOID)hModule, size, PAGE_EXECUTE_READWRITE, &oldProtect);
}

HMODULE(WINAPI* orig_LoadLibraryA)(LPCSTR lpFileName);
HMODULE WINAPI hLoadLibraryA(LPSTR lpFileName)
{
	HMODULE hModule = orig_LoadLibraryA(lpFileName);
	DWORD pBase = (DWORD)GetModuleHandleA(lpFileName);

	if (!pBase)
		return hModule;
	
	if (strstr(lpFileName, "ui_mp"))
	{
		if (codversion != COD1_1_1_MP)
			return hModule;

		void UI_Init(DWORD);
		UI_Init(pBase);
	}
	else if (strstr(lpFileName, "cgame_mp"))
	{
		if (codversion != COD1_1_1_MP)
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

void patch_opcode_loadlibrary(void)
{
	orig_LoadLibraryA = (struct HINSTANCE__* (__stdcall*)(const char*)) \
		DetourFunction((LPBYTE)LoadLibraryA, (LPBYTE)hLoadLibraryA);
}

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
bool verifyCodVersion()
{
	int addressMP = 0x566C18;
	int addressSP = 0x555494;
	const char* versionMP = "1.1";
	const char* versionSP = "1.0";

	if (is_addr_safe(addressMP))
	{
		char* patchVersion = (char*)addressMP;
		if (patchVersion && !strcmp(patchVersion, versionMP))
		{
			codversion = COD1_1_1_MP;
			return true;
		}
	}

	if (is_addr_safe(addressSP))
	{
		char* patchVersion = (char*)addressSP;
		if (patchVersion && !strcmp(patchVersion, versionSP))
		{
			codversion = COD1_1_1_SP;
			return true;
		}
	}

	return false;
}

void cleanupExit()
{
	void(*o)();
	* (UINT32*)&o = 0x40E2B0;
	o();

	void Sys_Unload();
	Sys_Unload();
}

static void(*Com_Quit_f)() = (void(*)())0x435D80;

cHook* hook_sv_shutdown;
void custom_SV_Shutdown(char* finalmsg)
{
	hook_sv_shutdown->unhook();
	void (*SV_Shutdown)(char* finalmsg);
	*(int*)&SV_Shutdown = hook_sv_shutdown->from;
	SV_Shutdown(finalmsg);
	hook_sv_shutdown->hook();

	LONG style = GetWindowLong(*gameWindow, GWL_STYLE);
	style |= WS_MAXIMIZEBOX;
	SetWindowLong(*gameWindow, GWL_STYLE, style);
	SetWindowPos(*gameWindow, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
}

cHook* hook_sv_startup;
void custom_SV_Startup(void)
{
	LONG style = GetWindowLong(*gameWindow, GWL_STYLE);
	style &= ~WS_MAXIMIZEBOX;
	SetWindowLong(*gameWindow, GWL_STYLE, style);
	SetWindowPos(*gameWindow, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

	hook_sv_startup->unhook();
	void (*SV_Startup)(void);
	*(int*)&SV_Startup = hook_sv_startup->from;
	SV_Startup();
	hook_sv_startup->hook();
}

extern bool movedWindow;
cHook* hook_in_frame;
void custom_IN_Frame(void)
{
	if (movedWindow)
		return;

	hook_in_frame->unhook();
	void (*IN_Frame)(void);
	*(int*)&IN_Frame = hook_in_frame->from;
	IN_Frame();
	hook_in_frame->hook();
}

bool applyHooks()
{
	unlock_client_structure(); // make some client cls structure members writeable

	// by lstolcman
	// allow alt tab - set dwExStyle from WS_EX_TOPMOST to WS_EX_LEFT (default), which allows minimizing
	memset((void*)0x5083b1, 0x00, 1);
	//

	patch_opcode_loadlibrary();
	void patch_opcode_glbindtexture(void);
	patch_opcode_glbindtexture();

	LRESULT CALLBACK h_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	* (int*)(0x4639b9 + 1) = (int)h_WndProc;

	void _CL_Init();
	__call(0x437B4B, (int)_CL_Init);
	__call(0x438178, (int)_CL_Init);

	void CL_Frame(int msec);
	__call(0x43822C, (int)CL_Frame);

	char* __cdecl CL_SetServerInfo_HostnameStrncpy(char*, char*, size_t);
	__call(0x412A2C, (int)CL_SetServerInfo_HostnameStrncpy);

	void _CL_Connect_f();
	XUNLOCK((void*)0x41269B, 5);
	*(UINT32*)(0x41269B + 1) = (int)_CL_Connect_f;

	void _CL_NextDownload();
	__call(0x410316, (int)_CL_NextDownload);
	__call(0x410376, (int)_CL_NextDownload);
	__call(0x41656C, (int)_CL_NextDownload);

	void Field_CharEvent_IgnoreTilde();
	__jmp(0x40CB1E, (int)Field_CharEvent_IgnoreTilde);

	__call(0x46319B, (int)cleanupExit);

	hook_in_frame = new cHook(0x00461a80, (int)custom_IN_Frame);
	hook_in_frame->hook();

	hook_sv_startup = new cHook(0x00458160, (int)custom_SV_Startup);
	hook_sv_startup->hook();

	hook_sv_shutdown = new cHook(0x00459600, (int)custom_SV_Shutdown);
	hook_sv_shutdown->hook();

	return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		HMODULE hModule = GetModuleHandle(NULL); // codmp.exe
		if (hModule)
			Main_UnprotectModule(hModule);

#if 0
#ifdef DEBUG
		if (hLogFile == INVALID_HANDLE_VALUE)
		{
			hLogFile = CreateFile(L"./memlog.txt",
				GENERIC_WRITE,
				FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL, NULL);
			_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
			_CrtSetReportFile(_CRT_WARN, hLogFile);
		}
#endif
#endif

		if (!verifyCodVersion())
		{
			MessageBoxA(NULL, "CoD version verification failed", "c1cx", MB_OK | MB_ICONERROR);
			return FALSE;
		}
		else if ((codversion == COD1_1_1_MP) && !applyHooks())
		{
			MessageBoxA(NULL, "Hooking failed", "c1cx", MB_OK | MB_ICONERROR);
			Com_Quit_f();
		}
	}
	break;

	case DLL_PROCESS_DETACH:
	{

	}
	break;

	}
	return TRUE;
}