#include "shared.h"
#include "render.h"
#include "client.h"
#include "libs/imgui/imgui.h"
#include "libs/imgui/imgui_impl_opengl2.h"
#include "libs/imgui/imgui_impl_win32.h"
#pragma comment(lib, "libs/detours/detours.lib")
#include "libs/detours/detours.h"

bool initImguiCalled = false;

extern bool menuIsDisplayed;
extern bool displayMenu;

BOOL(WINAPI* oSwapBuffers)(HDC);
HGLRC wglContext;
SCR_DrawString_t SCR_DrawString = (SCR_DrawString_t)0x4DF570;
RE_SetColor_t RE_SetColor = (RE_SetColor_t)0x4DDCF0;

void initImgui(HDC hdc)
{
	initImguiCalled = true;
	
	wglContext = wglCreateContext(hdc);
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_InitForOpenGL(*gameWindow);
	ImGui_ImplOpenGL2_Init();
	ImGui::SetNextWindowPos(ImVec2(50, 100)/*, ImGuiCond_FirstUseEver*/);
	ImGui::SetNextWindowSize(ImVec2(*glc_vidWidth / 2, *glc_vidHeight / 3));
	//ImGui::SetNextWindowContentSize(ImVec2(100, 100));
}

BOOL __stdcall hSwapBuffers(HDC hdc)
{
	if (!initImguiCalled)
		initImgui(hdc);

	if (!displayMenu)
	{
		if (menuIsDisplayed)
			menuIsDisplayed = false;
		return oSwapBuffers(hdc);
	}

	HGLRC o_WglContext = wglGetCurrentContext();
	wglMakeCurrent(hdc, wglContext);

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("menu");
	ImGui::End();
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	wglMakeCurrent(hdc, o_WglContext);

	if (!menuIsDisplayed)
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