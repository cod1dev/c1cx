#include "shared.h"
#include <WinSock2.h>

bool apply_hooks();
bool determine_cod_version();
bool find_cod_version();

#define ENABLE_CODX

void codextended()
{
	srand(time(NULL));

	if (!determine_cod_version())
	{
		if (!find_cod_version())
		{
			MsgBox("Failed to find Call of Duty version");
			return;
		}
	}
	if (!apply_hooks())
	{
		MsgBox("Failed to initialize CoDExtended");
		Com_Quit_f();
	}
}

void codextended_unload() {}