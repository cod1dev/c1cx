#include "shared.h"
#include "menudef.h"
#include "render.h"
#include "client.h"

DWORD ui_mp;

#define UI_FILE_OFF(x) (ui_mp + (x - 0x40000000)) //vmMain in IDA offset

void UI_DisplayDownloadInfo(const char downloadName, float centerPoint, float yStart, float scale)
{
	void(*DisplayText)(const char, float, float, float);
	*(int*)&DisplayText = UI_FILE_OFF(0x4000DEA0);
	DisplayText(downloadName, centerPoint, yStart, 0.25);
}

void UI_Init(DWORD base)
{
	ui_mp = base;
	__call(UI_FILE_OFF(0x4000E895), (int)UI_DisplayDownloadInfo); // Smaller download text (UDP only)
}