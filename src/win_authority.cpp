#include "shared.h"
#include "Commctrl.h"
#include "ShlObj.h"
#include "Shellapi.h"

bool verifyCodVersion()
{
#ifdef PATCH_1_1
	int addressMP = 0x566C18;
	int addressSP = 0x555494;
	char* versionMP = "1.1";
	char* versionSP = "1.0";
#elif PATCH_1_5
	int addressMP = 0x005a60d0;
	int addressSP = 0x005565ac;
	char* versionMP = "1.5";
	char* versionSP = "1.3";
#endif

	if (is_addr_safe(addressMP))
	{
		char* patchVersion = (char*)addressMP;
		if (patchVersion && !strcmp(patchVersion, versionMP))
		{
#ifdef PATCH_1_1
			codversion = COD1_1_1_MP;
#elif PATCH_1_5
			codversion = COD1_1_5_MP;
#endif
			return true;
		}
	}

	if (is_addr_safe(addressSP))
	{
		char* patchVersion = (char*)addressSP;
		if (patchVersion && !strcmp(patchVersion, versionSP))
		{
#ifdef PATCH_1_1
			codversion = COD1_1_1_SP;
#elif PATCH_1_5
			codversion = COD1_1_5_SP;
#endif
			return true;
		}
	}

	return false;
}