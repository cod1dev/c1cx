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

bool sensitivityAimMultiply_enabled = false;
float sensitivityAimMultiply_value = 0.0f;
bool hideConnectionInterrupted = false;
bool hideMiddleMessages = false;

extern cvar_t* cl_sensitivityAimMultiply_enabled;
extern cvar_t* cl_sensitivityAimMultiply;
#ifdef PATCH_1_1
extern cvar_t* cg_drawConnectionInterrupted;
extern cvar_t* cg_drawMessagesMiddle;
#endif

BOOL(WINAPI* oSwapBuffers)(HDC);
HGLRC wglContext;
HWND hWnd_during_imgui_init;

#ifdef PATCH_1_1
SCR_DrawString_t SCR_DrawString = (SCR_DrawString_t)0x4DF570;
RE_SetColor_t RE_SetColor = (RE_SetColor_t)0x4DDCF0;
#endif

void initImgui(HDC hdc)
{
	imgui_init_called = true;

	hWnd_during_imgui_init = *gameWindow;
	wglContext = wglCreateContext(hdc);
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();	
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Verdana.ttf", 18.0f);
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
	ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(50, 150), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowContentSize(ImVec2(100, 100));
	ImGui::SetNextWindowFocus();
	ImGui::Begin("codxt", NULL, ImGuiWindowFlags_NoCollapse);

#ifdef PATCH_1_1
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

	ImGui::Separator(); //TODO: full window width
#endif

	/*Sensitivity aim multiplier*/
	sensitivityAimMultiply_enabled = cl_sensitivityAimMultiply_enabled->integer;
	sensitivityAimMultiply_value = cl_sensitivityAimMultiply->value;

	ImGui::Checkbox("Sensitivity aim multiplier", &sensitivityAimMultiply_enabled);
	Cvar_Set(cl_sensitivityAimMultiply_enabled->name, sensitivityAimMultiply_enabled ? "1" : "0");

	if (!sensitivityAimMultiply_enabled)
		ImGui::BeginDisabled();

	ImGui::SliderFloat("##", &sensitivityAimMultiply_value, 0.25f, 1.25f, "%.2f", ImGuiSliderFlags_NoInput);
	Cvar_Set(cl_sensitivityAimMultiply->name, va("%f", (float)sensitivityAimMultiply_value));

	if (!sensitivityAimMultiply_enabled)
		ImGui::EndDisabled();
	/**/

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