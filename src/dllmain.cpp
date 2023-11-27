#include "stdafx.h"

void codextended();

HMODULE hModule;
#ifdef DEBUG
extern HANDLE hLogFile = INVALID_HANDLE_VALUE;
#endif

// PROCESS_DETACH is not called so don't make global declarations which have deconstructors which have to be called.
static BYTE originalCode[5];
static PBYTE originalEP = 0;
void Main_UnprotectModule(HMODULE hModule);

void Main_DoInit()
{
	// unprotect our entire PE image
	HMODULE hModule;
	if (SUCCEEDED(GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)Main_DoInit, &hModule)))
	{
		Main_UnprotectModule(hModule);
	}
	void patch_opcode_loadlibrary(void);
	// return to the original EP
	memcpy(originalEP, &originalCode, sizeof(originalCode));
	__asm jmp originalEP
}

void Main_SetSafeInit()
{
	// find the entry point for the executable process, set page access, and replace the EP
	HMODULE hModule = GetModuleHandle(NULL); // passing NULL should be safe even with the loader lock being held (according to ReactOS ldr.c)

	if (hModule)
	{
		PIMAGE_DOS_HEADER header = (PIMAGE_DOS_HEADER)hModule;
		PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((DWORD)hModule + header->e_lfanew);
		Main_UnprotectModule(hModule);
		// back up original code
		PBYTE ep = (PBYTE)((DWORD)hModule + ntHeader->OptionalHeader.AddressOfEntryPoint);
		memcpy(originalCode, ep, sizeof(originalCode));
		// patch to call our EP
		int newEP = (int)Main_DoInit - ((int)ep + 5);
		ep[0] = 0xE9; // for some reason this doesn't work properly when run under the debugger
		memcpy(&ep[1], &newEP, 4);
		originalEP = ep;
	}
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			char szModuleName[MAX_PATH + 1];

			GetModuleFileNameA(NULL, szModuleName, MAX_PATH);
			void MSS32_Hook();
			MSS32_Hook();

			extern bool mss32_original_loaded;
			if (!mss32_original_loaded)
				return FALSE;
			Main_SetSafeInit();

#ifdef DEBUG //Prevents SP from running
			if (hLogFile == INVALID_HANDLE_VALUE)
			{
				hLogFile = CreateFile("./memlog.txt",
					GENERIC_WRITE,
					FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL, NULL);
				_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
				_CrtSetReportFile(_CRT_WARN, hLogFile);
			}
#endif
			codextended();
		break;
		case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}