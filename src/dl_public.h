#pragma once

extern bool clc_bWWWDl;

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

int DL_BeginDownload(const char *localName, const char *remoteName, int debug);
dlStatus_t DL_DownloadLoop();
void DL_Shutdown();