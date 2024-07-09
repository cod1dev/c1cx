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


static int(__stdcall* entryPoint)(HINSTANCE, HINSTANCE, LPSTR, int) = (int(__stdcall*)(HINSTANCE, HINSTANCE, LPSTR, int))0x4640B0;
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

const char* imguiConfigPath = nullptr;
bool movedWindow = false;
LRESULT CALLBACK h_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg != WM_KEYDOWN || wParam != VK_HOME) // Not to send toggle key to imgui
	{
		if (menuIsDisplayed && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		{
			return true;
		}
	}

	switch (uMsg)
	{
	
	// To use Injector directoy for imgui.ini
	case WM_COPYDATA:
	{
		PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;
		char* pData = (char*)pcds->lpData;
		if (strstr(pData, "imgui.ini"))
			imguiConfigPath = _strdup(pData);
	}
	break;
	// end

	// Prevent mouse capture after moving window
	case WM_EXITSIZEMOVE:
		movedWindow = true;
		break;
	case WM_LBUTTONDOWN:
		if (movedWindow)
			movedWindow = false;
		break;
	// end

	// Maximize button support
	case WM_SYSCOMMAND:
		if (wParam == SC_MAXIMIZE)
		{
			movedWindow = false;

			Cvar_Set("r_fullscreen", "1");
			void(*Cbuf_ExecuteText)(int exec_when, const char* text);
			*(int*)&Cbuf_ExecuteText = 0x00428290;
			Cbuf_ExecuteText(EXEC_APPEND, "vid_restart\n");
			return 0;
		}
		break;
	case WM_NCLBUTTONDBLCLK:
		return 0;
		break;
	// end

	case WM_CREATE:
		SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
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
					((void(*)())0x4616b0)(); //IN_DeactivateMouse //TODO: check if can do similar for keyboard
					*mouseActive = 0;
					*mouseInitialized = 0;
				}
				else
				{
					ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename); //Not to wait IniSavingRate
					displayMenu = false;
					*mouseInitialized = 1;
					((void(*)())0x461730)(); //IN_ActivateMouse
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

	* (int*)&o_WndProc = 0x466BE0;

	return o_WndProc(hWnd, uMsg, wParam, lParam);
}