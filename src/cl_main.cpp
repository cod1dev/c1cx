#include "shared.h"
#include "client.h"
#include "dl_public.h"

cvar_t* com_cl_running;
cvar_t* g_bounce;
cvar_t* cl_wwwDownload;
cvar_t* cl_allowDownload;
cvar_t* cl_sensitivityAimMultiply_enabled;
cvar_t* cl_sensitivityAimMultiply;
cvar_t* cg_drawConnectionInterrupted;
cvar_t* cg_drawMessagesMiddle;
cvar_t* cg_drawFPS;
cvar_t* cg_drawFPS_x;
cvar_t* cg_drawFPS_y;
cvar_t* cg_zoomFovMultiply_enabled;
cvar_t* cg_zoomFovMultiply;

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

void _CL_Connect_f()
{
	Cvar_Set("fs_game", "");

	void(*CL_Connect_f)();
	* (int*)(&CL_Connect_f) = 0x40F6A0;
	CL_Connect_f();

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

void _CL_Init(void)
{
#ifdef PATCH_1_1
	bool fixBugs();
	if (!fixBugs())
	{
		MsgBox("Bug fixes failed");
		Com_Quit_f();
	}
#endif

	void(*CL_Init)();
#ifdef PATCH_1_1
	*(int*)(&CL_Init) = 0x411E60;
#elif PATCH_1_5
	* (int*)(&CL_Init) = 0x00413380;
#endif
	CL_Init();

	Cvar_Get("codxt", "1", CVAR_USERINFO | CVAR_ROM);

#ifdef PATCH_1_1
	Cmd_AddCommand("minimize", Cmd_Minimize);

	com_cl_running = Cvar_Get("cl_running", "0", CVAR_ROM);
	cl_wwwDownload = Cvar_Get("cl_wwwDownload", "1", CVAR_ARCHIVE);
	cl_allowDownload = Cvar_Get("cl_allowDownload", "0", CVAR_ARCHIVE);
#endif

	cl_sensitivityAimMultiply_enabled = Cvar_Get("sensitivityAimMultiply_enabled", "0", CVAR_ARCHIVE);
	cl_sensitivityAimMultiply = Cvar_Get("sensitivityAimMultiply", "0.5", CVAR_ARCHIVE);

#ifdef PATCH_1_1
	g_bounce = Cvar_Get("g_bounce", "0", CVAR_ARCHIVE);
	cg_drawConnectionInterrupted = Cvar_Get("cg_drawConnectionInterrupted", "1", CVAR_ARCHIVE);
	cg_drawFPS = Cvar_Get("cg_drawFPS", "0", CVAR_ARCHIVE);
	cg_drawFPS_x = Cvar_Get("cg_drawFPS_x", "523", CVAR_ARCHIVE);
	cg_drawFPS_y = Cvar_Get("cg_drawFPS_y", "2", CVAR_ARCHIVE);
	cg_drawMessagesMiddle = Cvar_Get("cg_drawMessagesMiddle", "1", CVAR_ARCHIVE);
	cg_zoomFovMultiply_enabled = Cvar_Get("cg_zoomFovMultiply_enabled", "0", CVAR_ARCHIVE);
	cg_zoomFovMultiply = Cvar_Get("cg_zoomFovMultiply", "1", CVAR_ARCHIVE);
#endif
}

#ifdef PATCH_1_5
bool g_legacyStyle_client;
std::map<std::string, std::map<std::string, WeaponProperties>> weapons_properties;

int32_t BG_GetWeaponIndexForName(const char* name)
{
	_asm
	{
		mov esi, name
		mov eax, cgame_mp
		add eax, 0x10040
		call eax
	}
}

WeaponDef_t* BG_GetWeaponDef(int32_t index)
{
	WeaponDef_t* weaponinfo = nullptr;
	//TODO: check with BG_IsWeaponIndexValid before, or use original function
	uintptr_t ptr = *(uintptr_t*)(cgame_mp + 0xF0FB4);
	weaponinfo = *(WeaponDef_t**)(ptr + index * 4);
	return weaponinfo;
}

void toggleLegacyStyle()
{
	char* g_legacyStyle = Info_ValueForKey(cs1, "g_legacyStyle");
	if (!*g_legacyStyle)
		return;

	bool g_legacyStyle_server = (atoi(g_legacyStyle) == 1) ? true : false;
	if (g_legacyStyle_client == g_legacyStyle_server)
		return;

	int id_kar98k_sniper = BG_GetWeaponIndexForName("kar98k_sniper_mp");
	WeaponDef_t* weapon_kar98k_sniper = BG_GetWeaponDef(id_kar98k_sniper);
	int id_springfield = BG_GetWeaponIndexForName("springfield_mp");
	WeaponDef_t* weapon_springfield = BG_GetWeaponDef(id_springfield);
	int id_mosin_nagant_sniper = BG_GetWeaponIndexForName("mosin_nagant_sniper_mp");
	WeaponDef_t* weapon_mosin_nagant_sniper = BG_GetWeaponDef(id_mosin_nagant_sniper);

	if (weapon_kar98k_sniper)
	{
		const WeaponProperties* properties_kar98k_sniper = nullptr;
		if (g_legacyStyle_server)
			properties_kar98k_sniper = &weapons_properties[weapon_kar98k_sniper->name]["legacy"];
		else
			properties_kar98k_sniper = &weapons_properties[weapon_kar98k_sniper->name]["default"];
		weapon_kar98k_sniper->adsTransInTime = properties_kar98k_sniper->adsTransInTime;
		weapon_kar98k_sniper->OOPosAnimLength[0] = 1.0 / (float)weapon_kar98k_sniper->adsTransInTime;
		weapon_kar98k_sniper->adsZoomInFrac = properties_kar98k_sniper->adsZoomInFrac;
		weapon_kar98k_sniper->idleCrouchFactor = properties_kar98k_sniper->idleCrouchFactor;
		weapon_kar98k_sniper->idleProneFactor = properties_kar98k_sniper->idleProneFactor;
		weapon_kar98k_sniper->rechamberWhileAds = properties_kar98k_sniper->rechamberWhileAds;
		weapon_kar98k_sniper->adsViewErrorMin = properties_kar98k_sniper->adsViewErrorMin;
		weapon_kar98k_sniper->adsViewErrorMax = properties_kar98k_sniper->adsViewErrorMax;
	}

	if (weapon_mosin_nagant_sniper)
	{
		const WeaponProperties* properties_mosin_nagant_sniper = nullptr;
		if (g_legacyStyle_server)
			properties_mosin_nagant_sniper = &weapons_properties[weapon_mosin_nagant_sniper->name]["legacy"];
		else
			properties_mosin_nagant_sniper = &weapons_properties[weapon_mosin_nagant_sniper->name]["default"];
		weapon_mosin_nagant_sniper->reloadAddTime = properties_mosin_nagant_sniper->reloadAddTime;
		weapon_mosin_nagant_sniper->adsTransInTime = properties_mosin_nagant_sniper->adsTransInTime;
		weapon_mosin_nagant_sniper->OOPosAnimLength[0] = 1.0 / (float)weapon_mosin_nagant_sniper->adsTransInTime;
		weapon_mosin_nagant_sniper->adsZoomInFrac = properties_mosin_nagant_sniper->adsZoomInFrac;
		weapon_mosin_nagant_sniper->idleCrouchFactor = properties_mosin_nagant_sniper->idleCrouchFactor;
		weapon_mosin_nagant_sniper->idleProneFactor = properties_mosin_nagant_sniper->idleProneFactor;
		weapon_mosin_nagant_sniper->rechamberWhileAds = properties_mosin_nagant_sniper->rechamberWhileAds;
		weapon_mosin_nagant_sniper->adsViewErrorMin = properties_mosin_nagant_sniper->adsViewErrorMin;
		weapon_mosin_nagant_sniper->adsViewErrorMax = properties_mosin_nagant_sniper->adsViewErrorMax;
	}

	if (weapon_springfield)
	{
		const WeaponProperties* properties_springfield = nullptr;
		if (g_legacyStyle_server)
			properties_springfield = &weapons_properties[weapon_springfield->name]["legacy"];
		else
			properties_springfield = &weapons_properties[weapon_springfield->name]["default"];
		weapon_springfield->adsTransInTime = properties_springfield->adsTransInTime;
		weapon_springfield->OOPosAnimLength[0] = 1.0 / (float)weapon_springfield->adsTransInTime;
		weapon_springfield->adsZoomInFrac = properties_springfield->adsZoomInFrac;
		weapon_springfield->idleCrouchFactor = properties_springfield->idleCrouchFactor;
		weapon_springfield->idleProneFactor = properties_springfield->idleProneFactor;
		weapon_springfield->rechamberWhileAds = properties_springfield->rechamberWhileAds;
		weapon_springfield->adsViewErrorMin = properties_springfield->adsViewErrorMin;
		weapon_springfield->adsViewErrorMax = properties_springfield->adsViewErrorMax;
	}

	g_legacyStyle_client = g_legacyStyle_server;
}

void _CL_SystemInfoChanged(void)
{
	void(*CL_SystemInfoChanged)();
	*(int*)(&CL_SystemInfoChanged) = 0x004176f0;
	CL_SystemInfoChanged();

	if (cgame_mp)
		toggleLegacyStyle();
}

void _CL_InitCGame(void)
{
	void(*CL_InitCGame)();
	*(int*)(&CL_InitCGame) = 0x00404ab0;
	CL_InitCGame();

	if (weapons_properties.empty())
	{
		weapons_properties["kar98k_sniper_mp"]["default"] = { 199, 449, 0.1, 0.6, 0.2, 0, 1.2, 1.4 };
		weapons_properties["kar98k_sniper_mp"]["legacy"] = { 199, 299, 0.42, 0.2, 0.085, 1, 0, 0 };

		weapons_properties["mosin_nagant_sniper_mp"]["default"] = { 1339, 449, 0.1, 0.6, 0.2, 0, 1.2, 1.4 };
		weapons_properties["mosin_nagant_sniper_mp"]["legacy"] = { 339, 299, 0.42, 0.2, 0.085, 1, 0, 0 };

		weapons_properties["springfield_mp"]["default"] = { 199, 449, 0.1, 0.6, 0.2, 0, 1.2, 1.4 };
		weapons_properties["springfield_mp"]["legacy"] = { 199, 299, 0.5, 0.2, 0.085, 1, 0, 0 };
		/*
		springfield_mp adsZoomInFrac in 1.1 patch weapon file = 0.05.
		There must be an error somewhere. Now replacing by 0.5 to fix slowness.
		*/
	}

	g_legacyStyle_client = false;
	toggleLegacyStyle();
}
#endif