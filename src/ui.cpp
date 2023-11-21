#include "shared.h"
#include "menudef.h"
#include "render.h"
#include "client.h"

DWORD ui_mp;

#define UI_FILE_OFF(x) (ui_mp + (x - 0x40000000)) //vmMain in IDA offset

typedef struct rectDef_s
{
	float x;    // horiz position
	float y;    // vert position
	float w;    // width
	float h;    // height;
} rectDef_t;
typedef rectDef_t qRectangle;
typedef struct
{
	qRectangle rect;                 // client coord rectangle
	qRectangle rectClient;           // screen coord rectangle
	const char *name;
	char _stuff[0x24];
	int flags;
} menuDef_t;

qboolean String_Parse(char **p, const char **out) /* out must be freed */
{ 
	char *token;
	char *COM_ParseExt(char **data_p, qboolean allowLineBreaks);
	token = COM_ParseExt(p, qfalse);
	if (token && token[0] != 0)
	{
		*(out) = strdup(token);
		return qtrue;
	}
	return qfalse;
}

void Q_strcat(char *dest, int size, const char *src);
int Q_stricmp(const char *s1, const char *s2);

static bool isUIRunning = false;

void UI_DisplayDownloadInfo(const char downloadName, float centerPoint, float yStart, float scale)
{
	void(*DisplayText)(const char, float, float, float);
	*(int*)&DisplayText = UI_FILE_OFF(0x4000DEA0);
	DisplayText(downloadName, centerPoint, yStart, 0.25);
}

void UI_Init(DWORD base)
{
	ui_mp = base;
	isUIRunning = true;

	__call(UI_FILE_OFF(0x4000E895), (int)UI_DisplayDownloadInfo); // Smaller download text (UDP only).
}