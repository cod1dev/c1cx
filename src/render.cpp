#include "shared.h"
#include "render.h"
#include "client.h"
#include "libs/imgui/imgui.h"
#include "libs/imgui/imgui_impl_opengl2.h"
#include "libs/imgui/imgui_impl_win32.h"
#pragma comment(lib, "libs/detours/detours.lib")
#include "libs/detours/detours.h"

extern bool displayMenu;
extern bool menuIsDisplayed;
bool imgui_init_called = false;
bool imgui_needs_restore = false;
bool imgui_size_position_set = false;
bool imgui_focus_set = false;

BOOL(WINAPI* oSwapBuffers)(HDC);
HGLRC wglContext;
HWND hWnd;

SCR_DrawString_t SCR_DrawString = (SCR_DrawString_t)0x4DF570;
RE_SetColor_t RE_SetColor = (RE_SetColor_t)0x4DDCF0;

void initImgui(HDC hdc)
{
	imgui_init_called = true;

	hWnd = WindowFromDC(hdc);
	wglContext = wglCreateContext(hdc);
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();	
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_InitForOpenGL(hWnd);
	ImGui_ImplOpenGL2_Init();
}

BOOL __stdcall hSwapBuffers(HDC hdc)
{
	if (!imgui_init_called)
		initImgui(hdc);
	else
	{
		if (WindowFromDC(hdc) != hWnd) //Caused by Alt+Enter / vid_restart
		{
			ImGui_ImplOpenGL2_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();

			initImgui(hdc);
			imgui_focus_set = false;
			imgui_needs_restore = true;
		}
	}
	if (!displayMenu)
	{
		if (menuIsDisplayed)
			imgui_focus_set = false;
		menuIsDisplayed = false;
		return oSwapBuffers(hdc);
	}

	HGLRC o_WglContext = wglGetCurrentContext();
	wglMakeCurrent(hdc, wglContext);

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	if (!imgui_size_position_set)
	{
		ImGui::SetNextWindowSize(ImVec2(500, 300));
		ImGui::SetNextWindowPos(ImVec2(50, 150));
		//ImGui::SetNextWindowContentSize(ImVec2(100, 100));
		imgui_size_position_set = true;
	}
	if (!imgui_focus_set)
	{
		ImGui::SetNextWindowFocus();
		imgui_focus_set = true;
	}
	ImGui::Begin("menu");
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
		MsgBox("no opengl");
		return 0;
	}
	FARPROC bindtex = GetProcAddress(hOpenGL, name);
	if (!bindtex)
	{
		MsgBox("no bindtex");
		return 0;
	}
	return bindtex;
}
void patch_opcode_glbindtexture(void)
{
	oSwapBuffers = (BOOL(__stdcall*)(HDC)) \
		DetourFunction((LPBYTE)get_gl_func_ptr("wglSwapBuffers"), (LPBYTE)hSwapBuffers);
}