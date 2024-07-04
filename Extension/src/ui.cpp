#include <windows.h>
#include "hooking.h"
#include "shared.h"
#include "cl.h"
#include "gl/gl.h"
#include "gl/glu.h"

#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_impl_opengl2.h"
#include "../vendor/imgui/imgui_impl_win32.h"
#pragma comment(lib, "opengl32")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "vendor/detours/detours.lib")
#include "../vendor/detours/detours.h"



DWORD ui_mp;

#ifdef PATCH_1_1
#define UI_FILE_OFF(x) (ui_mp + (x - 0x40000000)) //vmMain in IDA offset
void UI_DisplayDownloadInfo(const char downloadName, float centerPoint, float yStart, float scale)
{
	void(*DisplayText)(const char, float, float, float);
	*(int*)&DisplayText = UI_FILE_OFF(0x4000DEA0);
	DisplayText(downloadName, centerPoint, yStart, 0.25);
}
#endif

void UI_Init(DWORD base)
{
	ui_mp = base;
#ifdef PATCH_1_1
	__call(UI_FILE_OFF(0x4000E895), (int)UI_DisplayDownloadInfo); // Smaller download text (UDP only)
#endif
}








extern bool displayMenu;
extern bool menuIsDisplayed;
bool imgui_init_called = false;
bool imgui_needs_restore = false;

bool sensitivityAimMultiply_enabled = false;
float sensitivityAimMultiply_value = 0.0f;
bool hideConnectionInterrupted = false;
bool hideWeaponSelection = false;
bool hideMiddleMessages = false;

#ifdef PATCH_1_1
bool zoomFovMultiply_enabled = false;
float zoomFovMultiply_value = 0.0f;
#endif

extern cvar_t* cl_sensitivityAimMultiply_enabled;
extern cvar_t* cl_sensitivityAimMultiply;
#ifdef PATCH_1_1
extern cvar_t* cg_drawConnectionInterrupted;
extern cvar_t* cg_drawWeaponSelection;
extern cvar_t* cg_drawMessagesMiddle;

extern cvar_t* cg_zoomFovMultiply_enabled;
extern cvar_t* cg_zoomFovMultiply;
#endif

BOOL(WINAPI* oSwapBuffers)(HDC);
HGLRC wglContext;
HWND hWnd_during_imgui_init;

extern const char* imguiConfigPath;
void initImgui(HDC hdc)
{
	imgui_init_called = true;

	hWnd_during_imgui_init = *gameWindow;
	wglContext = wglCreateContext(hdc);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Verdana.ttf", 18.0f);

	if (imguiConfigPath != nullptr)
		io.IniFilename = imguiConfigPath;

	ImGui::StyleColorsDark();
	ImGui_ImplWin32_InitForOpenGL(*gameWindow);
	ImGui_ImplOpenGL2_Init();
}

BOOL __stdcall hSwapBuffers(HDC hdc)
{
	if (!imgui_init_called)
	{
		initImgui(hdc);
	}
	else
	{
		if (WindowFromDC(hdc) != hWnd_during_imgui_init) //Caused by Alt+Enter / vid_restart
		{
			ImGui_ImplOpenGL2_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();

			initImgui(hdc);
			imgui_needs_restore = true;
		}
	}
	if (!displayMenu)
	{
		menuIsDisplayed = false;
		return oSwapBuffers(hdc);
	}

	HGLRC o_WglContext = wglGetCurrentContext();
	wglMakeCurrent(hdc, wglContext);

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::SetNextWindowSize(ImVec2(0, 0));
	ImGui::SetNextWindowPos(ImVec2(50, 150), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowFocus();
	ImGui::Begin("c1cx", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

#ifdef PATCH_1_1
	ImGui::SeparatorText("User Interface");

	/*Connection Interrupted */
	hideConnectionInterrupted = cg_drawConnectionInterrupted->integer ? false : true;
	ImGui::Checkbox("Hide \"Connection Interrupted\"", &hideConnectionInterrupted);
	Cvar_Set(cg_drawConnectionInterrupted->name, hideConnectionInterrupted ? "0" : "1");
	/**/

	/*Middle messages*/
	hideMiddleMessages = cg_drawMessagesMiddle->integer ? false : true;
	ImGui::Checkbox("Hide middle messages", &hideMiddleMessages);
	Cvar_Set(cg_drawMessagesMiddle->name, hideMiddleMessages ? "0" : "1");
	/**/

	/*Weapon selection */
	hideWeaponSelection = cg_drawWeaponSelection->integer ? false : true;
	ImGui::Checkbox("Hide weapon selection", &hideWeaponSelection);
	Cvar_Set(cg_drawWeaponSelection->name, hideWeaponSelection ? "0" : "1");
	/**/
#endif

	ImGui::SeparatorText("Aim Down Sight");

	/*Sensitivity aim multiplier*/
	sensitivityAimMultiply_enabled = cl_sensitivityAimMultiply_enabled->integer;
	sensitivityAimMultiply_value = cl_sensitivityAimMultiply->value;

	ImGui::Checkbox("Sensitivity scale", &sensitivityAimMultiply_enabled);
	Cvar_Set(cl_sensitivityAimMultiply_enabled->name, sensitivityAimMultiply_enabled ? "1" : "0");

	if (!sensitivityAimMultiply_enabled)
		ImGui::BeginDisabled();

	ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - (ImGui::GetStyle().WindowPadding.x * 2));
	ImGui::SliderFloat("##sensiads", &sensitivityAimMultiply_value, 0.25f, 1.25f, "%.2f", ImGuiSliderFlags_NoInput);
	Cvar_Set(cl_sensitivityAimMultiply->name, va("%f", (float)sensitivityAimMultiply_value));

	if (!sensitivityAimMultiply_enabled)
		ImGui::EndDisabled();
	/**/
#ifdef PATCH_1_1

	/*Zoom fov multiplier*/
	zoomFovMultiply_enabled = cg_zoomFovMultiply_enabled->integer;
	zoomFovMultiply_value = cg_zoomFovMultiply->value;

	ImGui::Checkbox("FOV scale", &zoomFovMultiply_enabled);
	Cvar_Set(cg_zoomFovMultiply_enabled->name, zoomFovMultiply_enabled ? "1" : "0");

	if (!zoomFovMultiply_enabled)
		ImGui::BeginDisabled();

	ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - (ImGui::GetStyle().WindowPadding.x * 2));
	ImGui::SliderFloat("##zoomfov", &zoomFovMultiply_value, 0.80f, 1.20f, "%.2f", ImGuiSliderFlags_NoInput);
	Cvar_Set(cg_zoomFovMultiply->name, va("%f", (float)zoomFovMultiply_value));

	if (!zoomFovMultiply_enabled)
		ImGui::EndDisabled();
	/**/
#endif

	ImGui::End();
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	wglMakeCurrent(hdc, o_WglContext);
	menuIsDisplayed = true;
	return oSwapBuffers(hdc);
}
FARPROC get_gl_func_ptr(const char* name)
{
	HMODULE hOpenGL = GetModuleHandleA("opengl32.dll");
	if (!hOpenGL)
	{
		MessageBoxA(NULL, "!hOpenGL", "c1cx", MB_OK | MB_ICONERROR);
		return 0;
	}
	FARPROC bindtex = GetProcAddress(hOpenGL, name);
	if (!bindtex)
	{
		MessageBoxA(NULL, "!bindtex", "c1cx", MB_OK | MB_ICONERROR);
		return 0;
	}
	return bindtex;
}
void patch_opcode_glbindtexture(void)
{
	oSwapBuffers = (BOOL(__stdcall*)(HDC)) \
		DetourFunction((LPBYTE)get_gl_func_ptr("wglSwapBuffers"), (LPBYTE)hSwapBuffers);
}