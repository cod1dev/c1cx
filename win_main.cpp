#include "shared.h"
#include "Commctrl.h"
#include "ShlObj.h"
#include "Shlwapi.h"
#include "Shellapi.h"

static int(__stdcall *main)(HINSTANCE, HINSTANCE, LPSTR, int) = (int(__stdcall*)(HINSTANCE, HINSTANCE, LPSTR, int))0x4640B0;

char sys_cmdline[MAX_STRING_CHARS];
char szAppData[MAX_PATH + 1];

std::vector<threadInfo_t> threadsinfo;
bool thrIsExit = false;

extern HMODULE hModule;
HINSTANCE hInst;

extern "C" bool bClosing = false;

#ifdef DEBUG
HMODULE hLogFile;
#endif
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

	extern bool miles32_loaded;
	if (!miles32_loaded)
		return 0;

#ifndef DEBUG
	void CL_DiscordInitialize();
	CL_DiscordInitialize();
#endif

	return main(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}