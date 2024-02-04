#include "shared.h"
#include "client.h"

void cleanupExit()
{
	void(*o)();
	*(UINT32*)&o = 0x40E2B0;
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






extern bool displayMenu;



BOOL __stdcall hSetCursorPos(int x, int y)
{
	if (displayMenu)
		return TRUE;
	return SetCursorPos(x, y);
}
POINT cachedPt;
BOOL __stdcall hGetCursorPos(LPPOINT pt)
{
	if (displayMenu)
	{
		pt->x = cachedPt.x;
		pt->y = cachedPt.y;
		return TRUE;
	}
	auto result = GetCursorPos(pt);
	cachedPt.x = pt->x;
	cachedPt.y = pt->y;
	return result;
}







bool applyHooks()
{
	HMODULE hModule;
	if (SUCCEEDED(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)applyHooks, &hModule)))
	{
		Main_UnprotectModule(hModule);
	}

	unlock_client_structure(); // make some client cls structure members writeable

	/*by lstolcman*/
	// allow alt tab - set dwExStyle from WS_EX_TOPMOST to WS_EX_LEFT (default), which allows minimizing
	XUNLOCK((void*)0x5083b1, 1);
	memset((void*)0x5083b1, 0x00, 1);
	/**/

	void patch_opcode_loadlibrary();
	patch_opcode_loadlibrary();

	void patch_opcode_glbindtexture(void);
	patch_opcode_glbindtexture();

	int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
	__call(0x528948, (int)WinMain);

	LRESULT CALLBACK h_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	*(int*)(0x4639b9 + 1) = (int)h_WndProc;

	__ffcall(0x461858, (int)hGetCursorPos);
	__ffcall(0x461664, (int)hSetCursorPos);
	__ffcall(0x46186c, (int)hSetCursorPos);

	void CL_Init();
	__call(0x437B4B, (int)CL_Init);
	__call(0x438178, (int)CL_Init);

	void CL_Frame(int msec);
	__call(0x43822C, (int)CL_Frame);

	char* __cdecl CL_SetServerInfo_HostnameStrncpy(char*, char*, size_t);
	__call(0x412A2C, (int)CL_SetServerInfo_HostnameStrncpy);

	void CL_Connect_f();
	XUNLOCK((void*)0x41269B, 5);
	*(UINT32*)(0x41269B + 1) = (int)CL_Connect_f;

	//void _CL_InitDownloads();
	//__call(0x41627b, (int)_CL_InitDownloads);
	void _CL_NextDownload();
	__call(0x410316, (int)_CL_NextDownload);
	__call(0x410376, (int)_CL_NextDownload);
	__call(0x41656C, (int)_CL_NextDownload);

	void Field_CharEvent_IgnoreTilde();
	__jmp(0x40CB1E, (int)Field_CharEvent_IgnoreTilde);

	__call(0x46319B, (int)cleanupExit);

	return true;
}