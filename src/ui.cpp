#include "shared.h"
#include "render.h"
#include "client.h"

DWORD ui_mp;

#ifdef PATCH_1_1
#define UI_FILE_OFF(x) (ui_mp + (x - 0x40000000)) //vmMain in IDA offset
void UI_DisplayDownloadInfo(const char downloadName, float centerPoint, float yStart, float scale)
{
	void(*DisplayText)(const char, float, float, float);
	*(int*)&DisplayText = UI_FILE_OFF(0x4000DEA0);
	DisplayText(downloadName, centerPoint, yStart, 0.25);
}
#endif

void UI_Init(DWORD base)
{
	ui_mp = base;
#ifdef PATCH_1_1
	__call(UI_FILE_OFF(0x4000E895), (int)UI_DisplayDownloadInfo); // Smaller download text (UDP only)
#endif
}