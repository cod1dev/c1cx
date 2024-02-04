#include "shared.h"
#include "Commctrl.h"
#include "ShlObj.h"
#include "Shlwapi.h"
#include "Shellapi.h"
#include "libs/imgui/imgui.h"
#include "libs/imgui/imgui_impl_opengl2.h"
#include "libs/imgui/imgui_impl_win32.h"
#include "client.h"

static int(__stdcall *entryPoint)(HINSTANCE, HINSTANCE, LPSTR, int) = (int(__stdcall*)(HINSTANCE, HINSTANCE, LPSTR, int))0x4640B0;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern "C" bool bClosing = false;

#ifdef DEBUG
HMODULE hLogFile;
#endif
extern HMODULE hModule;

bool waitForMenuKeyReleasing = false;
bool menuIsDisplayed = false;
bool displayMenu = false;

void Sys_Unload()
{
	bClosing = true;
	static bool unloaded = false;
	if (unloaded)
		return;
	unloaded = true;
#ifdef DEBUG
	_CrtDumpMemoryLeaks();
	CloseHandle(hLogFile);
#endif
}

LRESULT CALLBACK h_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	switch (uMsg)
	{
	case WM_CREATE:
		SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) | WS_MINIMIZEBOX);
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_HOME:
			if (!waitForMenuKeyReleasing)
			{
				if (!menuIsDisplayed)
				{
					displayMenu = true;
					Cvar_Set("in_mouse", "0");
					((void(*)())0x461910)(); //IN_Shutdown //To give mouse to menu
					*mouseInitialized = 0;
				}
				else
				{
					displayMenu = false;
					Cvar_Set("in_mouse", "1");
					*mouseInitialized = 1;
				}
			}
			waitForMenuKeyReleasing = true;
			break;
		default:
			if (menuIsDisplayed)
				return 0; //To prevent player moving
			break;
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_HOME:
			if (waitForMenuKeyReleasing)
				waitForMenuKeyReleasing = false;
			break;
		default:
			break;
		}
		break;
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_MOUSEMOVE: //To prevent click+move = shoot
		if (menuIsDisplayed)
			return 0;
		break;
	case WM_MENUCHAR:
		return MNC_CLOSE << 16; //To prevent Alt+Enter beep sound
	}
	
	LRESULT(CALLBACK * o_WndProc)(HWND, UINT, WPARAM, LPARAM);
	*(int*)&o_WndProc = 0x466BE0;
	return o_WndProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	void MSS32_Hook();
	MSS32_Hook();

	extern bool mss32_original_loaded;
	if (!mss32_original_loaded)
		return 0;

	return entryPoint(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}