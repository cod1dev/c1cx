#include "shared.h"
#include "Commctrl.h"
#include "ShlObj.h"
#include "Shellapi.h"

bool verifyCodVersion()
{
	if (is_addr_safe(0x566C18))
	{
		char* patchVersion = (char*)0x566C18;
		if (patchVersion && !strcmp(patchVersion, "1.1"))
		{
			codversion = COD1_1_1;
			return true;
		}
	}
	if (is_addr_safe(0x555494))
	{
		char* patchVersion = (char*)0x555494;
		if (patchVersion && !strcmp(patchVersion, "1.0"))
		{
			codversion = COD1_SP;
			return true;
		}
	}
	return false;
}