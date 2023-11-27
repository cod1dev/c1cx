#include "shared.h"
#include "client.h"
#include "render.h"
#include "dl_public.h"
#pragma comment(lib, "psapi.lib")
#include "Psapi.h"
#include "Shlwapi.h"

cvar_t * com_cl_running;
cvar_t *cl_wwwDownload;
cvar_t *cl_allowDownload;
cvar_t* cl_sensitivityAimMultiply;

#include <sstream>
#include <cstdint>

void Cmd_Minimize()
{
	ShowWindow(*gameWindow, SW_MINIMIZE);
}

char* __cdecl CL_SetServerInfo_HostnameStrncpy(char* a1, char* a2, size_t a3)
{
	return strncpy(a1, Com_CleanHostname(a2, true), a3);
}

void Need_Paks()
{
	return;
}

void DL_Name(const char* localName, char* remoteName)
{
	Cvar_Set("cl_downloadName", va("        %s", remoteName)); // Spaces to render name fully.
}
void _CL_DownloadsComplete()
{
	void(*CL_DownloadsComplete)();
	*(int*)(&CL_DownloadsComplete) = 0x40FFB0;
	CL_DownloadsComplete();
}
int dl_files_count = 0;
static int use_regular_dl = 0;
void WWW_BeginDownload()
{
	if (clc_bWWWDl)
		return;

	char localTempName[MAX_PATH];
	char remoteTempName[MAX_PATH];
	char* s;
	char* remoteName, * localName;
	char* info = clc_stringData + clc_stringOffsets[1];
	char* url = Info_ValueForKey(info, "sv_wwwBaseURL");

	if (*clc_downloadList)
	{
		s = clc_downloadList;

		dl_files_count = 0;
		int i;
		for (i = 0; i < strlen(clc_downloadList); i++)
			if (clc_downloadList[i] == '@')
				dl_files_count++;

		// @remotename@localname@remotename@localname, etc.
		if (*s == '@')
		{
			s++;
		}
		remoteName = s;
		if ((s = strchr(s, '@')) == NULL)
		{
			_CL_DownloadsComplete();
			return;
		}
		*s++ = 0;
		localName = s;
		if ((s = strchr(s, '@')) != NULL)
		{
			*s++ = 0;
		}
		else
		{
			s = localName + strlen(localName); // point at the nul byte
		}

		int tmp = use_regular_dl;
		use_regular_dl = 0;
		if (cl_wwwDownload->integer && url && *url && !tmp)
		{
			Com_Printf("***** WWW_BeginDownload *****\n"
				"Localname: %s\n"
				"Remotename: %s\n"
				"****************************\n", localName, remoteName);

			Cvar_Set("cl_downloadSize", "0");
			Cvar_Set("cl_downloadCount", "0");
			Cvar_Set("cl_downloadTime", va("%i", *cls_realtime));

			Q_strncpyz(localTempName, FS_BuildOSPath(Cvar_VariableString("fs_homepath"), localName, ""), sizeof(localTempName));
			localTempName[strlen(localTempName) - 1] = '\0';
			Q_strncpyz(remoteTempName, FS_BuildOSPath(url, remoteName, ""), sizeof(remoteTempName));
			remoteTempName[strlen(remoteTempName) - 1] = '\0';

			if (!DL_BeginDownload(localTempName, remoteTempName, 1))
			{
				clc_bWWWDl = false;
				const char* error = va("Download failure while getting '%s'\n", remoteTempName); // get the msg before clearing structs
				Com_Error(ERR_DROP, error);
				return;
			}

			clc_bWWWDl = true;
		}
		*cls_downloadRestart = qtrue;
		memmove(clc_downloadList, s, strlen(s) + 1);
		return;
	}
	_CL_DownloadsComplete();
}
void CL_WWWDownload()
{
	dlStatus_t ret = DL_DownloadLoop();
	if (ret == DL_CONTINUE)
		return;
	if (ret == DL_DONE)
	{
		Cvar_Set("cl_downloadName", "");
		clc_bWWWDl = false;
		*cls_downloadRestart = 1;
		_CL_DownloadsComplete();
	}
	else if (ret == DL_FAILED)
	{
		// Perhaps actually check the response? Invalid URL, forbidden, etc?
		char* error = va("Download failure while getting %s.\nURL might be invalid.", Cvar_VariableString("dlname_error"));
		Com_Error(ERR_DROP, error);
		return;
	}
}
//DL STUCK FIX
int* cl_serverId = (int*)0x143a9ac;
int last_cl_serverId = 0;
void _CL_InitDownloads()
{
	bool preventCall = false;

	if (cl_allowDownload->integer
		&& FS_ComparePaks(clc_downloadList, 1024, qfalse)
		&& *clc_downloadList)
	{
		//CLIENT WILL DOWNLOAD
		if ((*cl_serverId && last_cl_serverId)
			&& (*cl_serverId > last_cl_serverId))
		{
			preventCall = true;
		}
	}
	else
	{
		//CLIENT WILL NOT DOWNLOAD
		if ((*cl_serverId && last_cl_serverId)
			&& (*cl_serverId > last_cl_serverId))
		{
			//Uncomment = prevents double map load but prevents future map changes (disconnect error: CL_SetCGameTime: !cl.snap.valid)
			//preventCall = true;
		}
	}

	if (*cl_serverId)
	{
		last_cl_serverId = *cl_serverId;
	}
	if (preventCall)
	{
		return;
	}
	void(*CL_InitDownloads)();
	*(int*)(&CL_InitDownloads) = 0x410240;
	CL_InitDownloads();
}
void _CL_NextDownload()
{
	char* info = clc_stringData + clc_stringOffsets[1];
	char* url = Info_ValueForKey(info, "sv_wwwBaseURL");
	int argc = Cmd_Argc();
	if (argc > 1)
	{
		const char* arg1 = Info_ValueForKey(info, "sv_referencedPakNames");
		if (strstr(arg1, ".pk3") != NULL)
		{
			Com_Error(ERR_DROP, "Unauthorized download attempted");
			return;
		}
	}

	if (cl_wwwDownload->integer && *url)
	{
		WWW_BeginDownload();
	}
	else
	{
		void(*CL_NextDownload)();
		*(int*)(&CL_NextDownload) = 0x410190;
		CL_NextDownload();
	}
}

void CL_Connect_f()
{
	void(*o)() = (void(*)())0x40F6A0;
	o();

	/*
	if (*cls_state == CA_CONNECTING || *cls_state == CA_CHALLENGING)
	{
		Cvar_Set("cl_allowDownload", "0");
	}
	*/

	/*
	char* info = clc_stringData + clc_stringOffsets[1];
	char *fs_game = Info_ValueForKey(info, "fs_game"); // Reset fs_game if loaded and server doesn't use it.
	if (fs_game == "")
	{
		Cvar_Set("fs_game", "");
	}
	*/
}
void Disconnect_IfEsc()
{
	if (*cls_state == CA_CONNECTING || *cls_state == CA_CHALLENGING || *cls_state == CA_CONNECTED)
	{
		if (GetFocus() && GetKeyState(VK_ESCAPE) & 0x8000)
		{
			((void(*)())0x40F5F0)(); //CL_Disconnnect 
		}
	}
}

char* MAX_PACKET_USERCMDS()
{
	return false;
}

void CL_Frame(int msec)
{
	void(*call)(int);
	*(DWORD*)&call = 0x411280;

	if (!com_cl_running->integer)
		return;

	if (clc_bWWWDl)
		CL_WWWDownload();

#ifndef DEBUG
	void CL_DiscordFrame();
	CL_DiscordFrame();
#endif

	Disconnect_IfEsc();

	call(msec);
}

void CL_Init(void)
{
	bool fix_bugs();
	if (!fix_bugs())
	{
		MsgBox("Failed to fix bugs");
		Com_Quit_f();
	}

	void(*oCL_Init)();
	*(int*)(&oCL_Init) = 0x411E60;
	oCL_Init();

	com_cl_running = Cvar_Get("cl_running", "0", CVAR_ROM);
	cl_wwwDownload = Cvar_Get("cl_wwwDownload", "1", CVAR_ARCHIVE);
	cl_allowDownload = Cvar_Get("cl_allowDownload", "0", CVAR_ARCHIVE);
	cl_sensitivityAimMultiply = Cvar_Get("sensitivityAimMultiply", "1.0", CVAR_ARCHIVE);

	Cmd_AddCommand("minimize", Cmd_Minimize);
}