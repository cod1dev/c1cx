#include <windows.h>
#include "com.h"

Com_DPrintf_t Com_DPrintf = (Com_DPrintf_t)0x435820;
Com_Printf_t Com_Printf = (Com_Printf_t)0x4357B0;
Com_Error_t Com_Error = (Com_Error_t)0x435AD0;

Cmd_AddCommand_t Cmd_AddCommand = (Cmd_AddCommand_t)0x428840;

#include <stdio.h>
char* va(const char* format, ...)
{
	va_list argptr;
#define MAX_VA_STRING 32000
	static char temp_buffer[MAX_VA_STRING];
	static char string[MAX_VA_STRING]; // in case va is called by nested functions
	static int index = 0;
	char* buf;
	int len;

	va_start(argptr, format);
	vsprintf_s(temp_buffer, sizeof(temp_buffer), format, argptr);
	va_end(argptr);

	if ((len = strlen(temp_buffer)) >= MAX_VA_STRING)
	{
		Com_Error(ERR_DROP, "Attempted to overrun string in call to va()\n");
	}
	if (len + index >= MAX_VA_STRING - 1)
	{
		index = 0;
	}
	buf = &string[index];
	memcpy(buf, temp_buffer, len + 1);
	index += len + 1;

	return buf;
}