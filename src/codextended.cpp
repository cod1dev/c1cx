#include "shared.h"
#include <WinSock2.h>

bool applyHooks();
bool determine_cod_version();
bool find_cod_version();

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
	if (!applyHooks())
	{
		MsgBox("Failed to initialize CoDExtended");
		Com_Quit_f();
	}
}
void codextended_unload(){}