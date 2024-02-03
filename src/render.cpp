#include "shared.h"
#include "render.h"
#include "client.h"
#include "libs/imgui/imgui.h"
#include "libs/imgui/imgui_impl_opengl2.h"
#include "libs/imgui/imgui_impl_win32.h"

#pragma comment(lib, "libs/detours/detours.lib")
#include "libs/detours/detours.h"

bool initImguiCalled = false;
BOOL(WINAPI* oSwapBuffers)(HDC);
SCR_DrawString_t SCR_DrawString = (SCR_DrawString_t)0x4DF570;
RE_SetColor_t RE_SetColor = (RE_SetColor_t)0x4DDCF0;

void initImgui()
{
	initImguiCalled = true;
	//OutputDebugString("##### initImgui \n");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_InitForOpenGL(*gameWindow);
	ImGui_ImplOpenGL2_Init();
}

BOOL __stdcall hSwapBuffers(HDC hdc)
{
	//OutputDebugString("##### hSwapBuffers \n");

	if (!initImguiCalled)
		initImgui();

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	bool show_demo_window = true;
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	ImGui::Render();
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

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
	//OutputDebugString("##### patch_opcode_glbindtexture \n");

	oSwapBuffers = (BOOL(__stdcall*)(HDC)) \
		DetourFunction((LPBYTE)get_gl_func_ptr("wglSwapBuffers"), (LPBYTE)hSwapBuffers);
}