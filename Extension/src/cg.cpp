#include <windows.h>
#include "hooking.h"
#include "shared.h"
#include "cl.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")



DWORD cgame_mp;

typedef void(*CG_ServerCommand_t)();
CG_ServerCommand_t CG_ServerCommand;

const char* writeProtectedCvars[] =
{
	"cl_allowdownload",
	"cl_avidemo",
	"cg_norender",
	"r_showimages",
	"sensitivity",
	"name",
	NULL
};

extern cvar_t* cg_drawMessagesMiddle;
char* (*CG_Argv)(int) = nullptr;
void _CG_ServerCommand(void)
{
	int argc = Cmd_Argc();
#ifdef DEBUG
	Com_Printf("^2CG_ServerCommand: ");
	for (int i = 0; i < argc; i++)
		Com_Printf("%s ", Cmd_Argv(i));
	Com_Printf("\n");
#endif

	if (argc > 0)
	{
		const char* cmd = Cmd_Argv(0);
		if (strlen(cmd) > 0)
		{
			if (*cmd == 'v')
			{
				if (argc > 1)
				{
					const char* var = Cmd_Argv(1);
					for (int i = 0; writeProtectedCvars[i]; i++)
						if (!strcmp(writeProtectedCvars[i], var))
							return;
				}
			}
			else if (*cmd == 'g')
			{
				if (!cg_drawMessagesMiddle->integer)
				{
					Com_Printf("cg_drawMessagesMiddle hid: %s \n", Cmd_Argv(1));
					return;
				}
			}
		}
	}
	CG_ServerCommand();
}

extern cvar_t* cg_drawConnectionInterrupted;
void CG_DrawDisconnect()
{
	if (cg_drawConnectionInterrupted->integer)
	{
		void(*call)();
		*(int*)&call = CGAME_OFF(0x30015450);
		call();
	}
}

#define	FPS_FRAMES 4
extern cvar_t* cg_drawFPS;
extern cvar_t* cg_drawFPS_x;
extern cvar_t* cg_drawFPS_y;
void _CG_DrawFPS(float y)
{
	if (cg_drawFPS->integer && cg_drawFPS->integer == 3)
	{
		//TODO: do like original code instead

		static int previousTimes[FPS_FRAMES];
		static int index;
		int	i, total;
		int	fps;
		static int previous;
		int	t, frameTime;

		t = timeGetTime();
		frameTime = t - previous;
		previous = t;
		previousTimes[index % FPS_FRAMES] = frameTime;
		index++;

		if (index > FPS_FRAMES)
		{
			total = 0;
			for (i = 0; i < FPS_FRAMES; i++)
			{
				total += previousTimes[i];
			}
			if (!total)
			{
				total = 1;
			}
			fps = 1000 * FPS_FRAMES / total;

			const char* s = va("%ifps", fps);

			void(*CG_DrawBigString)(float x, float y, const char* s, float alpha);
			*(int*)&CG_DrawBigString = CGAME_OFF(0x30019710);
			CG_DrawBigString(cg_drawFPS_x->integer, cg_drawFPS_y->integer, s, 1.0F);
		}
	}
	else
	{
		void(*CG_DrawFPS)(float);
		*(int*)&CG_DrawFPS = CGAME_OFF(0x30014A00);
		CG_DrawFPS(y);
	}
}

extern cvar_t* cg_drawWeaponSelection;
void _CG_DrawWeaponSelect()
{
	if (cg_drawWeaponSelection->integer)
	{
		void(*CG_DrawWeaponSelect)(void);
		*(int*)&CG_DrawWeaponSelect = CGAME_OFF(0x30037790);
		CG_DrawWeaponSelect();
	}
}

extern cvar_t* cl_sensitivityAimMultiply_enabled;
extern cvar_t* cl_sensitivityAimMultiply;
float stockCgZoomSensitivity()
{
	float* fov_visible_percentage = (float*)CGAME_OFF(0x3020958c); //Visible percentage of cg_fov value
	float* cg_fov_value = (float*)CGAME_OFF(0x30298c68);

	return (*fov_visible_percentage / *cg_fov_value); //See instruction 30032fe8
}
float multipliedCgZoomSensitivity()
{
	return stockCgZoomSensitivity() * cl_sensitivityAimMultiply->value;
}
void sensitivityAimMultiply()
{
	float* cg_zoomSensitivity = (float*)CGAME_OFF(0x3020b5f4); //zoomSensitivity var of cg_t struct
	float* ads_anim_progress = (float*)CGAME_OFF(0x30207214); //From 0 to 1

	//See FUN_30032e20
	if (*ads_anim_progress == 1) //ADS animation completed
	{
		//ADS
		if (cl_sensitivityAimMultiply_enabled->integer)
			*cg_zoomSensitivity = multipliedCgZoomSensitivity();
		else
			*cg_zoomSensitivity = stockCgZoomSensitivity();
	}
	else if (*ads_anim_progress != 0) //ADS animation in progress
	{
		bool* ads = (bool*)CGAME_OFF(0x30209458);
		if (*ads)
		{
			//ADS
			if (cl_sensitivityAimMultiply_enabled->integer)
				*cg_zoomSensitivity = multipliedCgZoomSensitivity();
			else
				*cg_zoomSensitivity = stockCgZoomSensitivity();
		}
		else
		{
			//NOT ADS
			*cg_zoomSensitivity = stockCgZoomSensitivity();
		}
	}
	else if (*ads_anim_progress == 0)
	{
		//NOT ADS
		*cg_zoomSensitivity = stockCgZoomSensitivity();
	}

	__asm
	{
		fstp st(0)
		retn
	}
}

extern cvar_t* cg_zoomFovMultiply_enabled;
extern cvar_t* cg_zoomFovMultiply;
float multipliedzoomFov(float adsZoomFov)
{
	if (cg_zoomFovMultiply->value >= 0.80f && cg_zoomFovMultiply->value <= 1.20f)
		return adsZoomFov * cg_zoomFovMultiply->value;
	return adsZoomFov * 1;
}
void zoomFovMultiply_zooming(float adsZoomFov)
{
	if (cg_zoomFovMultiply_enabled->integer)
		adsZoomFov = multipliedzoomFov(adsZoomFov);
	__asm
	{
		fsub dword ptr[adsZoomFov]
	}
}
uintptr_t resume_addr_zoomFovMultiply_zooming;
__declspec(naked) void zoomFovMultiply_zooming_Naked()
{
	__asm
	{
		push[ecx + 0x218];
		call zoomFovMultiply_zooming;
		pop ecx
			jmp resume_addr_zoomFovMultiply_zooming;
	}
}

void zoomFovMultiply_zoomed(float adsZoomFov)
{
	if (cg_zoomFovMultiply_enabled->integer)
		adsZoomFov = multipliedzoomFov(adsZoomFov);
	__asm
	{
		fld dword ptr[adsZoomFov]
	}
}
uintptr_t resume_addr_zoomFovMultiply_zoomed;
__declspec(naked) void zoomFovMultiply_zoomed_Naked()
{
	__asm
	{
		push[ecx + 0x218];
		call zoomFovMultiply_zoomed;
		pop ecx
			jmp resume_addr_zoomFovMultiply_zoomed;
	}
}

void CG_Init(DWORD base)
{
	cgame_mp = base;

	CG_Argv = (char* (*)(int))CGAME_OFF(0x30020960);
	CG_ServerCommand = (CG_ServerCommand_t)(cgame_mp + 0x2E0D0);

	__call(CGAME_OFF(0x3002E5A6), (int)_CG_ServerCommand);
	__call(CGAME_OFF(0x3001509E), (int)_CG_DrawFPS);
	__call(CGAME_OFF(0x300159CC), (int)CG_DrawDisconnect);
	__call(CGAME_OFF(0x300159D4), (int)CG_DrawDisconnect);

	__jmp(CGAME_OFF(0x30032fe8), (int)sensitivityAimMultiply);

	__jmp(CGAME_OFF(0x30032f1e), (int)zoomFovMultiply_zooming_Naked);
	resume_addr_zoomFovMultiply_zooming = CGAME_OFF(0x30032f24);

	__jmp(CGAME_OFF(0x30032ea5), (int)zoomFovMultiply_zoomed_Naked);
	resume_addr_zoomFovMultiply_zoomed = CGAME_OFF(0x30032eab);

	__call(CGAME_OFF(0x30018896), (int)_CG_DrawWeaponSelect);
}