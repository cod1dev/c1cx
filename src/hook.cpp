#include "stdafx.h"
#include "shared.h"
#include "client.h"

#include <sstream>

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
#if 0 // Failed attempts
LRESULT CALLBACK MyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
	switch (uMsg)
	{
		case WM_CREATE:
		{
			SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) | WS_MINIMIZEBOX);
			break;

			/*
			LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
			style |= WS_MINIMIZEBOX;
			SetWindowLongPtr(hwnd, GWL_STYLE, style);
			SetWindowPos(hwnd, NULL, 0, 0, *glc_vidWidth, *glc_vidHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
			return 0;
			*/
		}

		/*
		default:
		{
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
		*/
	}

	LRESULT(CALLBACK * o)(HWND, UINT, WPARAM, LPARAM);
	*(int*)&o = 0x466be0;
	return o(hwnd, uMsg, wParam, lParam);
}
#endif

bool applyHooks()
{
	HMODULE hModule;
	if (SUCCEEDED(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)applyHooks, &hModule)))
	{
		Main_UnprotectModule(hModule);
	}

	// Allow alt+tab (- )set dwExStyle from WS_EX_TOPMOST to WS_EX_LEFT (default))
	XUNLOCK((void*)0x5083b1, 1);
	memset((void*)0x5083b1, 0x00, 1);
#if 0 //See https://github.com/xtnded/codextended-client/pull/1
	// fix bad bahavior on 4k monitors - avoid redundant ChangeDisplaySettingsA
	XUNLOCK((void*)0x508821, 2);
	memset((void*)0x508821, 0x90, 1);
	memset((void*)0x508822, 0x90, 1);
#endif

	void patch_opcode_loadlibrary();
	patch_opcode_loadlibrary();
	
	if (codversion != COD_1)
		return true;

	unlock_client_structure(); // make some client cls_ structure members writeable etc
	
	int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
	__call(0x528948, (int)WinMain);

#if 0 // Failed attempts
	XUNLOCK((void*)0x508308, 25);
	*(int*)(0x508308) = (int)MyWndProc;
#endif

	void CL_Init();
	__call(0x437B4B, (int)CL_Init);
	__call(0x438178, (int)CL_Init);

	void CL_Frame(int msec);
	__call(0x43822C, (int)CL_Frame);

	void Field_CharEvent_IgnoreTilde();
	__jmp(0x40CB1E, (int)Field_CharEvent_IgnoreTilde);

	char* __cdecl CL_SetServerInfo_HostnameStrncpy(char*, char*, size_t);
	__call(0x412A2C, (int)CL_SetServerInfo_HostnameStrncpy);

	void CL_Connect_f();
	XUNLOCK((void*)0x41269B, 5);
	*(UINT32*)(0x41269B + 1) = (int)CL_Connect_f;

	void _CL_InitDownloads();
	__call(0x41627b, (int)_CL_InitDownloads);
	void _CL_NextDownload();
	__call(0x410316, (int)_CL_NextDownload);
	__call(0x410376, (int)_CL_NextDownload);
	__call(0x41656C, (int)_CL_NextDownload);

	__call(0x46319B, (int)cleanupExit);

	return true;
}