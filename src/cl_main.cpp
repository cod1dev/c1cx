#include "shared.h"
#include "client.h"
#include "render.h"
#include "dl_public.h"

cvar_t* com_cl_running;
cvar_t* g_bounce;
cvar_t* cl_wwwDownload;
cvar_t* cl_allowDownload;
cvar_t* cl_sensitivityAimMultiply_enabled;
cvar_t* cl_sensitivityAimMultiply;
cvar_t* cg_drawConnectionInterrupted;
cvar_t* cg_drawMessagesMiddle;
cvar_t* xui_fps;
cvar_t* xui_fps_x;
cvar_t* xui_fps_y;

#ifdef PATCH_1_1
void Cmd_Minimize()
{
	ShowWindow(*gameWindow, SW_MINIMIZE);
}

char* __cdecl CL_SetServerInfo_HostnameStrncpy(char* a1, char* a2, size_t a3)
{
	return strncpy(a1, Com_CleanHostname(a2, true), a3);
}

void DL_Name(const char* localName, char* remoteName)
{
	Cvar_Set("cl_downloadName", va("        %s", remoteName)); // Spaces to render name fully
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
		char* error = va("Failed downloading %s.", Cvar_VariableString("dlname_error"));
		Com_Error(ERR_DROP, error);
		return;
	}
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
			Com_Error(ERR_DROP, "Potentially dangerous download blocked");
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
	Cvar_Set("fs_game", "");

	void(*o)() = (void(*)())0x40F6A0;
	o();

	/*
	if (*cls_state == CA_CONNECTING || *cls_state == CA_CHALLENGING) //TODO: check the purpose
	{
		Cvar_Set("cl_allowDownload", "0");
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

void CL_Frame(int msec)
{
	if (!com_cl_running->integer)
		return;

	if (clc_bWWWDl)
		CL_WWWDownload();

	Disconnect_IfEsc();

	void(*call)(int);
	*(DWORD*)&call = 0x411280;
	call(msec);
}
#endif



void CL_Init(void)
{
#ifdef PATCH_1_1
	bool fixBugs();
	if (!fixBugs())
	{
		MsgBox("Bug fixes failed");
		Com_Quit_f();
	}
#endif

	void(*oCL_Init)();
#ifdef PATCH_1_1
	*(int*)(&oCL_Init) = 0x411E60;
#elif PATCH_1_5
	* (int*)(&oCL_Init) = 0x00413380;
#endif
	oCL_Init();

#ifdef PATCH_1_1
	Cmd_AddCommand("minimize", Cmd_Minimize);

	Cvar_Get("cl_supportHttpDownload", "1", CVAR_USERINFO | CVAR_ROM);

	com_cl_running = Cvar_Get("cl_running", "0", CVAR_ROM);
	cl_wwwDownload = Cvar_Get("cl_wwwDownload", "1", CVAR_ARCHIVE);
	cl_allowDownload = Cvar_Get("cl_allowDownload", "0", CVAR_ARCHIVE);
#endif

	cl_sensitivityAimMultiply_enabled = Cvar_Get("sensitivityAimMultiply_enabled", "0", CVAR_ARCHIVE);
	cl_sensitivityAimMultiply = Cvar_Get("sensitivityAimMultiply", "0.5", CVAR_ARCHIVE);

#ifdef PATCH_1_1
	g_bounce = Cvar_Get("g_bounce", "0", CVAR_ARCHIVE);
	cg_drawConnectionInterrupted = Cvar_Get("cg_drawConnectionInterrupted", "1", CVAR_ARCHIVE);
	cg_drawMessagesMiddle = Cvar_Get("cg_drawMessagesMiddle", "1", CVAR_ARCHIVE);
	xui_fps = Cvar_Get("cg_xui_fps", "1", CVAR_ARCHIVE);
	xui_fps_x = Cvar_Get("cg_xui_fps_x", "597", CVAR_ARCHIVE);
	xui_fps_y = Cvar_Get("cg_xui_fps_y", "12", CVAR_ARCHIVE);
#endif
}