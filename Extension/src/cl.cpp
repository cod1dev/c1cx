#include "hooking.h"
#include "shared.h"

#define CURL_STATICLIB
#pragma comment(lib, "vendor/libcurl/lib/libcurl_a.lib")
#include "../vendor/libcurl/include/curl/curl.h"

#include "cl.h"


cvar_t* com_cl_running;
cvar_t* jump_bounceEnable;
cvar_t* cl_wwwDownload;
cvar_t* cl_allowDownload;
cvar_t* cl_sensitivityAimMultiply_enabled;
cvar_t* cl_sensitivityAimMultiply;
cvar_t* cl_sensitivityAimMultiplySniper_enabled;
cvar_t* cl_sensitivityAimMultiplySniper;
cvar_t* cg_drawConnectionInterrupted;
cvar_t* cg_drawMessagesMiddle;
cvar_t* cg_drawFPS;
cvar_t* cg_drawFPS_x;
cvar_t* cg_drawFPS_y;
cvar_t* cg_drawWeaponSelection;

#define PITCH 0
#define YAW 1
void Cmd_LookBack()
{
	float* cl_viewangles = (float*)0x0143a9a0;
	cl_viewangles[YAW]+= 180;
}

void Cmd_Minimize()
{
	ShowWindow(*gameWindow, SW_MINIMIZE);
}

char* __cdecl CL_SetServerInfo_HostnameStrncpy(char* dest, const char* src, int destsize)
{
	errno_t err = strncpy_s(dest, destsize + 1, cleanHostname(src, true), _TRUNCATE);
	if (err)
		return NULL;
	return dest;
}

#define APP_NAME "ID_DOWNLOAD"
#define APP_VERSION "2.0"
#define LOCAL_DL_PATH "dl.tmp"

// initialize once
static int dl_initialized = 0;
static CURLM* dl_multi = NULL;
static CURL* dl_request = NULL;
FILE* dl_file = NULL;
bool clc_bWWWDl = false;

//Write to file
static size_t DL_cb_FWriteFile(void* ptr, size_t size, size_t nmemb, void* stream)
{
	FILE* file = (FILE*)stream;
	return fwrite(ptr, size, nmemb, file);
}
// Print progress
static int DL_cb_Progress(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	/* cl_downloadSize and cl_downloadTime are set by the Q3 protocol...
	and it would probably be expensive to verify them here.   -zinx */

	Cvar_Set("cl_downloadSize", va("%f", (float)dltotal));
	Cvar_Set("cl_downloadCount", va("%f", (float)dlnow));
	return 0;
}
void DL_InitDownload()
{
	if (dl_initialized)
		return;
	//Make sure curl has initialized, so the cleanup doesn't get confused
	curl_global_init(CURL_GLOBAL_ALL);
	dl_multi = curl_multi_init();
	Com_Printf("Client download subsystem initialized\n");
	dl_initialized = 1;
}
void DL_Shutdown()
{
	if (!dl_initialized)
		return;
	curl_multi_cleanup(dl_multi);
	dl_multi = NULL;
	curl_global_cleanup();
	dl_initialized = 0;
}


char localDownloadName[MAX_PATH];
int DL_BeginDownload(const char* localName, const char* remoteName, int debug)
{
	//char referer[MAX_STRING_CHARS + 5 /*"ET://"*/];
	if (dl_request) {
		Com_Printf("ERROR: DL_BeginDownload called with a download request already active\n"); \
			return 0;
	}

	if (!localName || !remoteName) {
		Com_DPrintf("Empty download URL or empty local file name\n");
		return 0;
	}

	_snprintf_s(localDownloadName, MAX_PATH - 1, "%s", localName);

	FS_CreatePath(LOCAL_DL_PATH);

	errno_t err = fopen_s(&dl_file, LOCAL_DL_PATH, "wb+");
	if (err != 0) {
		Com_Printf("ERROR: DL_BeginDownload unable to open '%s' for writing\n", LOCAL_DL_PATH);
		return 0;
	}

	DL_InitDownload();

	dl_request = curl_easy_init();
	curl_easy_setopt(dl_request, CURLOPT_USERAGENT, va("%s %s", APP_NAME "/" APP_VERSION, curl_version()));
	//curl_easy_setopt( dl_request, CURLOPT_REFERER, referer );
	curl_easy_setopt(dl_request, CURLOPT_URL, remoteName);
	curl_easy_setopt(dl_request, CURLOPT_WRITEFUNCTION, DL_cb_FWriteFile);
	curl_easy_setopt(dl_request, CURLOPT_WRITEDATA, (void*)dl_file);
	curl_easy_setopt(dl_request, CURLOPT_PROGRESSFUNCTION, DL_cb_Progress);
	curl_easy_setopt(dl_request, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(dl_request, CURLOPT_FAILONERROR, 1);

	curl_multi_add_handle(dl_multi, dl_request);

	Cvar_Set("cl_downloadName", va("        %s", (char*)remoteName)); //spaces to make link fully displayed
	Cvar_Set("dlname_error", (char*)remoteName);
	return 1;
}

typedef enum
{
	DL_CONTINUE,
	DL_DONE,
	DL_FAILED,
	DL_DISCONNECTED,
} dlStatus_t;
// bitmask
typedef enum
{
	DL_FLAG_DISCON = 0,
	DL_FLAG_URL
} dlFlags_t;


// (maybe this should be CL_DL_DownloadLoop)
dlStatus_t DL_DownloadLoop()
{
	CURLMcode status;
	CURLMsg* msg;
	int dls = 0;
	const char* err = NULL;

	if (!dl_request)
	{
		Com_DPrintf("DL_DownloadLoop: unexpected call with dl_request == NULL\n");
		return DL_DONE;
	}

	if (*cls_state != CA_CONNECTING && *cls_state != CA_CHALLENGING && *cls_state != CA_CONNECTED)
	{
		curl_multi_remove_handle(dl_multi, dl_request);
		curl_easy_cleanup(dl_request);
		fclose(dl_file);
		dl_file = NULL;
		dl_request = NULL;
		Cvar_Set("ui_dl_running", "0");
		return DL_DISCONNECTED;
	}

	if ((status = curl_multi_perform(dl_multi, &dls)) == CURLM_CALL_MULTI_PERFORM && dls)
	{
		return DL_CONTINUE;
	}

	while (true)
	{
		msg = curl_multi_info_read(dl_multi, &dls);
		if (msg && msg->easy_handle != dl_request)
		{
			continue;
		}
		break;
	}

	if (!msg || msg->msg != CURLMSG_DONE)
	{
		return DL_CONTINUE;
	}
	if (msg->data.result != CURLE_OK)
	{
		err = curl_easy_strerror(msg->data.result);
	}
	else
	{
		err = NULL;
	}

	curl_multi_remove_handle(dl_multi, dl_request);
	curl_easy_cleanup(dl_request);
	fclose(dl_file);
	dl_file = NULL;
	dl_request = NULL;
	Cvar_Set("ui_dl_running", "0");

	if (err)
	{
		Com_DPrintf("DL_DownloadLoop: request terminated with failure status '%s'\n", err);
		return DL_FAILED;
	}
	if (!MoveFileA(LOCAL_DL_PATH, localDownloadName))
	{
		Com_Printf("DL_DownloadFinished: failed to move temporary storage file to '%s'\n", localDownloadName);
	}
	*localDownloadName = '\0';

	return DL_DONE;
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
	const char* url = Info_ValueForKey(info, "sv_wwwBaseURL");

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
	const char* url = Info_ValueForKey(info, "sv_wwwBaseURL");
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
	*(int*)(&CL_Connect_f) = 0x40F6A0;
	CL_Connect_f();
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

void _CL_Init(void)
{
	void DL_Name(const char* localName, char* remoteName); //fix for not full download name on slow dl
	__call(0x41011C, (int)DL_Name);

	// NOP out the calls to CL_Motd (crash upon startup net not loaded and socket being sent or smth)
	__nop(0x40F6DA, 0x40F6DA + 5);
	__nop(0x4117B6, 0x4117B6 + 5);

	__nop(0x411815, 1);

	__nop(0x42D122, 5); //call Com_AppendCDKey (fixes the invalid cdkey with fs_game)
	__nop(0x40BC18, 5); //fixes spam with "MAX_PACKET_USERCMDS" if you have 1000 fps
	__nop(0x43BA04, 5); //Remove the non-used second "Need Paks:" message

	void(*CL_Init)();
	* (int*)(&CL_Init) = 0x411E60;
	CL_Init();

	Cvar_Get("c1cx", "1", CVAR_USERINFO | CVAR_ROM);

	Cmd_AddCommand("minimize", Cmd_Minimize);

	com_cl_running = Cvar_Get("cl_running", "0", CVAR_ROM);
	cl_wwwDownload = Cvar_Get("cl_wwwDownload", "1", CVAR_ARCHIVE);
	cl_allowDownload = Cvar_Get("cl_allowDownload", "0", CVAR_ARCHIVE);

	cl_sensitivityAimMultiply_enabled = Cvar_Get("sensitivityAimMultiply_enabled", "0", CVAR_ARCHIVE);
	cl_sensitivityAimMultiply = Cvar_Get("sensitivityAimMultiply", "0.5", CVAR_ARCHIVE);
	cl_sensitivityAimMultiplySniper_enabled = Cvar_Get("sensitivityAimMultiplySniper_enabled", "0", CVAR_ARCHIVE);
	cl_sensitivityAimMultiplySniper = Cvar_Get("sensitivityAimMultiplySniper", "0.5", CVAR_ARCHIVE);

	jump_bounceEnable = Cvar_Get("jump_bounceEnable", "0", CVAR_ARCHIVE);
	cg_drawConnectionInterrupted = Cvar_Get("cg_drawConnectionInterrupted", "1", CVAR_ARCHIVE);
	cg_drawFPS = Cvar_Get("cg_drawFPS", "0", CVAR_ARCHIVE);
	cg_drawFPS_x = Cvar_Get("cg_drawFPS_x", "523", CVAR_ARCHIVE);
	cg_drawFPS_y = Cvar_Get("cg_drawFPS_y", "2", CVAR_ARCHIVE);
	cg_drawWeaponSelection = Cvar_Get("cg_drawWeaponSelection", "1", CVAR_ARCHIVE);
	cg_drawMessagesMiddle = Cvar_Get("cg_drawMessagesMiddle", "1", CVAR_ARCHIVE);

	Cmd_AddCommand("lookback", Cmd_LookBack);
}

UINT_PTR pfield_charevent_return = (UINT_PTR)0x40CB77;
UINT_PTR pfield_charevent_continue = (UINT_PTR)0x40CB23;
__declspec(naked) void Field_CharEvent_IgnoreTilde()
{
	__asm
	{
		cmp ebx, 20h
		jge check
		jmp pfield_charevent_return

		check :
		cmp ebx, 126
			jl checked
			jmp pfield_charevent_return

			checked :
		jmp pfield_charevent_continue
	}
}
// cmp ebx, 20h is 3 bytes, we need 5 for a jmp...
// jl ... is 2 bytes 7c54 (assuming when subtracing the addresses)
// so it works out
// - Richard