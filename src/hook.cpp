#include "shared.h"
#include "client.h"

void cleanupExit()
{
	void(*o)();
#ifdef PATCH_1_1
	* (UINT32*)&o = 0x40E2B0;
#elif PATCH_1_5
	* (UINT32*)&o = 0x0040ef70;
#endif
	o();

	void Sys_Unload();
	Sys_Unload();
}

void Main_UnprotectModule(HMODULE hModule)
{
	PIMAGE_DOS_HEADER header = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((DWORD)hModule + header->e_lfanew);

	// unprotect the entire PE image
	SIZE_T size = ntHeader->OptionalHeader.SizeOfImage;
	DWORD oldProtect;
	VirtualProtect((LPVOID)hModule, size, PAGE_EXECUTE_READWRITE, &oldProtect);
}

bool applyHooks()
{
	HMODULE hModule;
	if (SUCCEEDED(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)applyHooks, &hModule)))
	{
		Main_UnprotectModule(hModule);
	}

#ifdef PATCH_1_1
	unlock_client_structure(); // make some client cls structure members writeable
#endif

#ifdef PATCH_1_1
	/*by lstolcman*/
	// allow alt tab - set dwExStyle from WS_EX_TOPMOST to WS_EX_LEFT (default), which allows minimizing
	XUNLOCK((void*)0x5083b1, 1);
	memset((void*)0x5083b1, 0x00, 1);
	/**/
#endif

	void patch_opcode_loadlibrary();
	patch_opcode_loadlibrary();

	void patch_opcode_glbindtexture(void);
	patch_opcode_glbindtexture();

	int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
#ifdef PATCH_1_1
	__call(0x528948, (int)WinMain);
#elif PATCH_1_5
	__call(0x00560f99, (int)WinMain);
#endif

	LRESULT CALLBACK h_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#ifdef PATCH_1_1
	* (int*)(0x4639b9 + 1) = (int)h_WndProc;
#elif PATCH_1_5
	* (int*)(0x00468db9 + 1) = (int)h_WndProc;
#endif

	void _CL_Init();
#ifdef PATCH_1_1
	__call(0x437B4B, (int)_CL_Init);
	__call(0x438178, (int)_CL_Init);
#elif PATCH_1_5
	__call(0x00439fca, (int)_CL_Init);
	__call(0x0043a617, (int)_CL_Init);
#endif

#ifdef PATCH_1_5
	void _CL_InitCGame(void);
	__call(0x004109c4, (int)_CL_InitCGame);
	__call(0x00410d2b, (int)_CL_InitCGame);
#endif

#ifdef PATCH_1_5
	void _CL_SystemInfoChanged(void);
	__call(0x004015fc, (int)_CL_SystemInfoChanged);
	__call(0x00417a78, (int)_CL_SystemInfoChanged);
#endif

#ifdef PATCH_1_1
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
#endif

#ifdef PATCH_1_1
	__call(0x46319B, (int)cleanupExit);
#elif PATCH_1_5
	__call(0x004684c5, (int)cleanupExit);
#endif
	return true;
}