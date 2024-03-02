#include "shared.h"
#include "stdafx.h"

bool applyHooks();
bool verifyCodVersion();

int codversion;
void codextended()
{
	srand(time(NULL));

	if (!verifyCodVersion())
	{
		MsgBox("CoD version verification failed");
		return;
	}
	else if ((codversion == COD1_1_1_MP || codversion == COD1_1_5_MP) && !applyHooks())
	{
		MsgBox("Hooking failed");
		Com_Quit_f();
	}
}