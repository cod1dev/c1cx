#ifdef PATCH_1_1

#include "shared.h"
#include "dl_public.h"
#include "client.h"
#pragma comment(lib, "libs/libcurl/lib/libcurl_a.lib")
#include "libs/libcurl/include/curl/curl.h"
#include <iostream>

#define APP_NAME "ID_DOWNLOAD"
#define APP_VERSION "2.0"

// initialize once
static int dl_initialized = 0;
static CURLM* dl_multi = NULL;
static CURL* dl_request = NULL;
FILE* dl_file = NULL;
bool clc_bWWWDl = false;

//Write to file
static size_t DL_cb_FWriteFile(void *ptr, size_t size, size_t nmemb, void *stream)
{
	FILE *file = (FILE*)stream;
	return fwrite(ptr, size, nmemb, file);
}
// Print progress
static int DL_cb_Progress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
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

#define LOCAL_DL_PATH "dl.tmp"
char localDownloadName[MAX_PATH];
int DL_BeginDownload(const char *localName, const char *remoteName, int debug)
{
	char referer[MAX_STRING_CHARS + 5 /*"ET://"*/];
	if (dl_request) {
		Com_Printf("ERROR: DL_BeginDownload called with a download request already active\n"); \
			return 0;
	}

	if (!localName || !remoteName) {
		Com_DPrintf("Empty download URL or empty local file name\n");
		return 0;
	}

	_snprintf(localDownloadName, MAX_PATH - 1, "%s", localName);

	FS_CreatePath(LOCAL_DL_PATH);
	dl_file = fopen(LOCAL_DL_PATH, "wb+");
	if (!dl_file) {
		Com_Printf("ERROR: DL_BeginDownload unable to open '%s' for writing\n", LOCAL_DL_PATH);
		return 0;
	}

	DL_InitDownload();

	/* ET://ip:port */
	/*
	strcpy( referer, "ET://" );
	Q_strncpyz( referer + 5, Cvar_VariableString( "cl_currentServerIP" ), MAX_STRING_CHARS );
	*/

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

#endif