#include "stdafx.h"
#include "shared.h"
#include "client.h"

//0046319B E8 10 B1 FA FF call sub_40E2B0
void sub_40E2B0()
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

bool apply_hooks()
{
	HMODULE hModule;
	if (SUCCEEDED(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)apply_hooks, &hModule)))
	{
		Main_UnprotectModule(hModule);
	}

	void patch_opcode_loadlibrary();
	patch_opcode_loadlibrary();
	
	if (codversion != COD_1)
		return true;

	__call(0x46319B, (int)sub_40E2B0); //cleanup exit

	int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
	__call(0x528948, (int)WinMain);

	void CL_Connect_f();
	XUNLOCK((void*)0x41269B, 5);
	*(UINT32*)(0x41269B + 1) = (int)CL_Connect_f;

	void RB_ExecuteRenderCommands();

	unlock_client_structure(); // make some client cls_ structure members writeable etc

	//DOWNLOAD FUNCTIONS
	void _CL_InitDownloads();
	__call(0x41627b, (int)_CL_InitDownloads);

	void _CL_NextDownload();
	__call(0x410316, (int)_CL_NextDownload);
	__call(0x410376, (int)_CL_NextDownload);
	__call(0x41656C, (int)_CL_NextDownload);
	//DOWNLOAD FUNCTIONS END

	void CL_Frame(int msec);
	__call(0x43822C, (int)CL_Frame);

	/*
	LRESULT CALLBACK ConsoleWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	XUNLOCK((void*)0x466414, 4);
	*(int*)0x466414 = (int)ConsoleWndProc;
	*/

	void Field_CharEvent_IgnoreTilde();
	__jmp(0x40CB1E, (int)Field_CharEvent_IgnoreTilde);

	void CL_Init();
	__call(0x437B4B, (int)CL_Init);
	__call(0x438178, (int)CL_Init);

	void* ri_Hunk_AllocAlign(int size);
	XUNLOCK((void*)0x4FD6AF, 6);
	*(BYTE*)0x4FD6AF = 0xe8;
	__call(0x4FD6AF, (int)ri_Hunk_AllocAlign); //cannot call since it's 6 opcodes
	*(BYTE*)(0x4FD6AF + 5) = 0x90;

	return true;
}