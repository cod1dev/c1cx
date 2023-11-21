#include "shared.h"
#include "Commctrl.h"
#include "ShlObj.h"
#include "Shellapi.h"

extern char sys_cmdline[MAX_STRING_CHARS];

#define ERROR_MSG(x) do \
	MessageBox(0, va("Call of Duty caught an error whilst trying to do something.\nFile: %s, Line: %d\nErrorcode: %d (0x%x)\nError: %s" \
				"Please report this error at the forums.",__FILE__,__LINE__,x,x,GetLastErrorAsString().c_str()), __TITLE, MB_ICONERROR | MB_OK); \
								while(0)

typedef struct
{
	int ver;
	char *md5;
} cod_v_pair;

cod_v_pair cod_v_info[] =
{
	{ COD_1, "753fbcabd0fdda7f7dad3dbb29c3c008" },
	{ COD_1_SP, "9fa83933bbf659050a2f213c217b624c" },
	//{ COD_1, "766345d1ceaf79caf7fe88a214b2f3ec" }, //from spect (different codmp entry points RVA import)
	{ CODUO_51, "928dd08dc169bd85fdd12d2db28def70" },
	{ COD_5_SP_STEAM, "acdf185fe7767d711fc99cb57df46044" },
	{ COD_5_STEAM, "4f4596b1cdb21f9eb62e6683ecf48dc6" },
	{ COD_5, "4bdf293d8e6fb32208d1b0942a1ba6bc" },
	{ COD_UNKNOWN, NULL }
};

bool determine_cod_version()
{
	FILE *fp = NULL;

	char szFileName[MAX_PATH + 1];
	GetModuleFileName(NULL, szFileName, MAX_PATH);

	if (GetLastError() != ERROR_SUCCESS)
	{
		MessageBoxA(NULL, "Too long filepath/name", "", 0);
		return false;
	}

	char *fn;
	Sys_GetModulePathInfo(NULL, NULL, &fn, NULL);
	fp = fopen(fn, "rb");
	if (!fp)
	{
		MessageBoxA(NULL, va("Error reading '%s'\nRun Call of Duty as Administrator.", fn), __TITLE, MB_OK);
		return false;
	}

	BYTE *buf = NULL;
	size_t fs = 0;
	fseek(fp, 0, SEEK_END);
	fs = ftell(fp);
	rewind(fp);
	buf = new BYTE[fs];

	if (fread(buf, 1, fs, fp) != fs)
	{
		MessageBox(NULL, va("Reading error '%s'\n", fn), __TITLE, MB_OK | MB_ICONERROR);
		delete[] buf;
		fclose(fp);
		return false;
	}

	std::string hash = GetHashText(buf, fs, HashMd5);

	delete[] buf;
	fclose(fp);
	fp = NULL;

	for (int i = 0; cod_v_info[i].md5 != NULL; i++)
	{
		if (!strncmp(cod_v_info[i].md5, hash.c_str(), 32))
		{
			codversion = cod_v_info[i].ver;
			break;
		}
	}

	return (codversion != COD_UNKNOWN);
}

bool find_cod_version()
{
	if (!is_addr_safe(0x566C18))
		return false;

	char *l_1 = (char*)0x566C18;
	if (!l_1)
		return false;

	if (!strcmp(l_1, "1.1"))
	{
		codversion = COD_1;
		return true;
	}
	return false;
}