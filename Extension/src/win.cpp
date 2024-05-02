#include <ctime>
#include "hooking.h"
#include "shared.h"
#include "cl.h"

#include "Commctrl.h"
#include "ShlObj.h"
#include "Shlwapi.h"
#include "Shellapi.h"

#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_impl_opengl2.h"
#include "../vendor/imgui/imgui_impl_win32.h"


#ifdef PATCH_1_1
static int(__stdcall* entryPoint)(HINSTANCE, HINSTANCE, LPSTR, int) = (int(__stdcall*)(HINSTANCE, HINSTANCE, LPSTR, int))0x4640B0;
#elif PATCH_1_5
static int(__stdcall* entryPoint)(HINSTANCE, HINSTANCE, LPSTR, int) = (int(__stdcall*)(HINSTANCE, HINSTANCE, LPSTR, int))0x004694b0;
#endif
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
	if (uMsg != WM_KEYDOWN || wParam != VK_HOME) //Not to send toggle key to imgui
	{
		if (menuIsDisplayed && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		{
			return true;
		}
	}

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
#ifdef PATCH_1_1
					((void(*)())0x4616b0)(); //IN_DeactivateMouse //TODO: check can do similar for keyboard
#elif PATCH_1_5
					((void(*)())0x004669d0)(); //IN_DeactivateMouse //TODO: check can do similar for keyboard
#endif
					* mouseActive = 0;
					*mouseInitialized = 0;
				}
				else
				{
					ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename); //Not to wait IniSavingRate
					displayMenu = false;
					*mouseInitialized = 1;
#ifdef PATCH_1_1
					((void(*)())0x461730)(); //IN_ActivateMouse
#elif PATCH_1_5
					((void(*)())0x00466a50)(); //IN_ActivateMouse
#endif
				}
			}
			waitForMenuKeyReleasing = true;
			break;
		default:
			if (menuIsDisplayed)
				return 0; //To prevent player move
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
	case WM_MOUSEWHEEL:
		if (menuIsDisplayed)
			return 0;
		break;
	case WM_MENUCHAR:
		return MNC_CLOSE << 16; //To prevent Alt+Enter beep sound
	}

	LRESULT(CALLBACK * o_WndProc)(HWND, UINT, WPARAM, LPARAM);


#ifdef PATCH_1_1
	* (int*)&o_WndProc = 0x466BE0;
#elif PATCH_1_5
	* (int*)&o_WndProc = 0x0046c160;
#endif

	return o_WndProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return entryPoint(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}