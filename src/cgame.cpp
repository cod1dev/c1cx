#include "shared.h"
#include "client.h"
#include "render.h"
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
char *(*CG_Argv)(int) = nullptr;
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
		char* cmd = Cmd_Argv(0);
		if (strlen(cmd) > 0)
		{
			if (*cmd == 'v')
			{
				if (argc > 1)
				{
					char* var = Cmd_Argv(1);
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

/* //TODO: improve so it works like CoD2
void pm_aimflag() // To aim in the air
{
	int *pm = (int*)(cgame_mp + 0x19D570);
	int *ps = (int*)*pm;
	int *gclient = (int*)*ps;
	int *v4 = (int *)(ps + 12);
	int val = *(int*)(gclient + 21); //336? 84*4=336 /84/4=21??
	if (val == 1023)
	{
		*v4 |= 0x20;
		return;
	}
	void(*call)();
	*(int*)&call = CGAME_OFF(0x3000FB80);
	call();
}
*/

extern cvar_t* cl_sensitivityAimMultiply;
float stockCgZoomSensitivity()
{
	float* fov_visible_percentage = (float*)CGAME_OFF(0x3020958c); //Visible percentage of cg_fov value
	float* cg_fov_value = (float*)CGAME_OFF(0x30298c68);
	return (*fov_visible_percentage / *cg_fov_value); //See instruction 30032fe8
}
void sensitivityAimMultiply()
{
	float* cg_zoomSensitivity = (float*)CGAME_OFF(0x3020b5f4); //zoomSensitivity var of cg_t struct
	float* ads_anim_progress = (float*)CGAME_OFF(0x30207214); //From 0 to 1
	//See FUN_30032e20
	if (*ads_anim_progress == 1) //ADS animation completed
	{
		//ADS
		*cg_zoomSensitivity = (stockCgZoomSensitivity() * cl_sensitivityAimMultiply->value);
	}
	else if (*ads_anim_progress != 0) //ADS animation in progress
	{
		bool* ads = (bool*)CGAME_OFF(0x30209458);
		if (*ads)
		{
			//ADS
			*cg_zoomSensitivity = (stockCgZoomSensitivity() * cl_sensitivityAimMultiply->value);
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

#define M_DrawShadowString(x,y,font,fontscale,color,text,a,b,c) \
	RE_SetColor(vColorBlack); \
	SCR_DrawString(x + 1,y + 1,font,fontscale,vColorBlack,text,a,b,c); \
	RE_SetColor(color); \
	SCR_DrawString(x,y,font,fontscale,color,text,a,b,c); \
	RE_SetColor(NULL);
#define	FPS_FRAMES 4
extern cvar_t* xui_fps;
extern cvar_t* xui_fps_x;
extern cvar_t* xui_fps_y;
void CG_DrawFPS(float y)
{
	if (xui_fps->integer)
	{
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

			M_DrawShadowString(xui_fps_x->integer, xui_fps_y->integer, 1, .20, vColorWhite, va("FPS: %d", fps), NULL, NULL, NULL);
		}
	}
	else
	{
		void(*call)(float);
		*(int*)&call = CGAME_OFF(0x30014A00);
		call(y);
	}
}

/*by xoxor4d*/
void PM_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out)
{
	float backoff;
	float change;
	int i;
	float overbounce = 1.001f;

	backoff = DotProduct(in, normal);
	if (backoff < 0)
	{
		backoff *= overbounce;
	}
	else
	{
		backoff /= overbounce;
	}

	for (i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;
	}
}
void PM_ProjectVelocity(vec3_t in, vec3_t normal, vec3_t out)
{
	float speedXY, DotNormalXY, normalisedNormalXY, projectionZ, projectionXYZ;
	speedXY = in[1] * in[1] + in[0] * in[0];
	if ((normal[2]) < 0.001f || (speedXY == 0.0f))
	{
		VectorCopy(in, out);
	}
	else
	{
		DotNormalXY = normal[1] * in[1] + in[0] * normal[0];
		normalisedNormalXY = -DotNormalXY / normal[2];
		projectionZ = in[2] * in[2] + speedXY;
		projectionXYZ = sqrtf((projectionZ / (speedXY + normalisedNormalXY * normalisedNormalXY)));

		if (projectionXYZ < 1.0f || normalisedNormalXY < 0.0f || in[2] > 0.0f)
		{
			out[0] = projectionXYZ * in[0];
			out[1] = projectionXYZ * in[1];
			out[2] = projectionXYZ * normalisedNormalXY;
		}
	}
}
uint32_t PM_Bounce(vec3_t in, vec3_t normal, vec3_t out)
{
	int x_cl_bounce = atoi(Info_ValueForKey(cs1, "x_cl_bounce"));
	if (x_cl_bounce)
	{
		PM_ProjectVelocity(in, normal, out);
	}
	else
	{
		PM_ClipVelocity(in, normal, out);
	}
	return CGAME_OFF(0x3000D830);
}
__declspec(naked) void PM_Bounce_Stub()
{
	__asm
	{
		push esi; // out
		push ecx; // normal
		push edx; // in
		call PM_Bounce;
		add esp, 12;
		push eax
			retn;
	}
}
/**/

void CG_Init(DWORD base)
{
	cgame_mp = base;

	CG_Argv = (char* (*)(int))CGAME_OFF(0x30020960);
	CG_ServerCommand = (CG_ServerCommand_t)(cgame_mp + 0x2E0D0);

	__call(CGAME_OFF(0x3002E5A6), (int)_CG_ServerCommand);

	__call(CGAME_OFF(0x3001509E), (int)CG_DrawFPS);
	__call(CGAME_OFF(0x300159CC), (int)CG_DrawDisconnect);
	__call(CGAME_OFF(0x300159D4), (int)CG_DrawDisconnect);

	/*
	__call(CGAME_OFF(0x3000C799), (int)pm_aimflag);
	__call(CGAME_OFF(0x3000C7B8), (int)pm_aimflag);
	__call(CGAME_OFF(0x3000C7D2), (int)pm_aimflag);
	__call(CGAME_OFF(0x3000C7FF), (int)pm_aimflag);
	__call(CGAME_OFF(0x3000C858), (int)pm_aimflag);
	__call(CGAME_OFF(0x3000C893), (int)pm_aimflag);
	*/

	__jmp(CGAME_OFF(0x3000D82B), (int)PM_Bounce_Stub);

	__jmp(CGAME_OFF(0x30032fe8), (int)sensitivityAimMultiply);
}