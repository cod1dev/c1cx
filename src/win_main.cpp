#include "shared.h"
#include "Commctrl.h"
#include "ShlObj.h"
#include "Shlwapi.h"
#include "Shellapi.h"

static int(__stdcall *entryPoint)(HINSTANCE, HINSTANCE, LPSTR, int) = (int(__stdcall*)(HINSTANCE, HINSTANCE, LPSTR, int))0x4640B0;

char sys_cmdline[MAX_STRING_CHARS];
char szAppData[MAX_PATH + 1];

bool thrIsExit = false;
extern "C" bool bClosing = false;

extern HMODULE hModule;
HINSTANCE hInst;
#ifdef DEBUG
HMODULE hLogFile;
#endif

std::vector<threadInfo_t> threadsinfo;

void Sys_Unload()
{
	bClosing = true;
	static bool unloaded = false;

	if (unloaded)
		return;
	unloaded = true;

#ifndef DEBUG
	void CL_DiscordShutdown();
	CL_DiscordShutdown();
#endif

#ifdef DEBUG
	_CrtDumpMemoryLeaks();
	CloseHandle(hLogFile);
#endif
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	hInst = hInstance;
	strncpy(sys_cmdline, lpCmdLine, sizeof(sys_cmdline) - 1);

	if (Sys_GetAppDataFolder(szAppData, MAX_PATH, true) == NULL)
	{
		MsgBox("Failed to create data folder.");
		return 0;
	}

	void MSS32_Hook();
	MSS32_Hook();

	extern bool mss32_original_loaded;
	if (!mss32_original_loaded)
		return 0;

#ifndef DEBUG
	void CL_DiscordInitialize();
	CL_DiscordInitialize();
#endif

	return entryPoint(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}