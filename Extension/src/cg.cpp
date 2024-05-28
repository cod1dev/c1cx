#include <windows.h>
#include "hooking.h"
#include "shared.h"
#include "cl.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")



DWORD cgame_mp;

#ifdef PATCH_1_1
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
#endif

#ifdef PATCH_1_1
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
#endif

#ifdef PATCH_1_1
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
#endif

extern cvar_t* cl_sensitivityAimMultiply_enabled;
extern cvar_t* cl_sensitivityAimMultiply;
float stockCgZoomSensitivity()
{
#ifdef PATCH_1_1
	float* fov_visible_percentage = (float*)CGAME_OFF(0x3020958c); //Visible percentage of cg_fov value
	float* cg_fov_value = (float*)CGAME_OFF(0x30298c68);
#elif PATCH_1_5
	float* fov_visible_percentage = (float*)CGAME_OFF(0x3020d340); //Visible percentage of cg_fov value
	float* cg_fov_value = (float*)CGAME_OFF(0x3029ca28);
#endif
	return (*fov_visible_percentage / *cg_fov_value); //See instruction 30032fe8 || 30034688
}
float multipliedCgZoomSensitivity()
{
	return stockCgZoomSensitivity() * cl_sensitivityAimMultiply->value;
}
void sensitivityAimMultiply()
{
#ifdef PATCH_1_1
	float* cg_zoomSensitivity = (float*)CGAME_OFF(0x3020b5f4); //zoomSensitivity var of cg_t struct
	float* ads_anim_progress = (float*)CGAME_OFF(0x30207214); //From 0 to 1
#elif PATCH_1_5
	float* cg_zoomSensitivity = (float*)CGAME_OFF(0x3020f3a8); //zoomSensitivity var of cg_t struct
	float* ads_anim_progress = (float*)CGAME_OFF(0x3020afcc); //From 0 to 1
#endif

	//See FUN_30032e20 || FUN_300344c0
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
#ifdef PATCH_1_1
		bool* ads = (bool*)CGAME_OFF(0x30209458);
#elif PATCH_1_5
		bool* ads = (bool*)CGAME_OFF(0x3020d20c);
#endif
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

#ifdef PATCH_1_1
extern cvar_t* cg_zoomFovMultiply_enabled;
extern cvar_t* cg_zoomFovMultiply;

float multipliedzoomFov(float adsZoomFov)
{
	if (cg_zoomFovMultiply->value >= 0.80f && cg_zoomFovMultiply->value <= 1.20f)
		return adsZoomFov *= cg_zoomFovMultiply->value;
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
#endif

#ifdef PATCH_1_5
#define PMF_JUMPING 0x2000
#define JUMP_LAND_SLOWDOWN_TIME 1800
#define VectorScale( v, s, o )      ( ( o )[0] = ( v )[0] * ( s ),( o )[1] = ( v )[1] * ( s ),( o )[2] = ( v )[2] * ( s ) )

void Jump_ApplySlowdown()
{
	int* pm = (int*)(cgame_mp + 0x1a0ed0);
	playerState_t* ps = ((pmove_t*)*((int*)pm))->ps;

	if (ps->pm_flags & PMF_JUMPING)
	{
		float scale = 1.0;

		if (ps->pm_time <= JUMP_LAND_SLOWDOWN_TIME)
		{
			if (!ps->pm_time)
			{
				if ((float)(ps->jumpOriginZ + 18.0) <= ps->origin[2])
				{
					ps->pm_time = 1200;
					scale = 0.5;
				}
				else
				{
					ps->pm_time = JUMP_LAND_SLOWDOWN_TIME;
					scale = 0.64999998;
				}
			}
		}
		else
		{
			// Clear jump state
			ps->pm_flags &= ~PMF_JUMPING;
			ps->jumpOriginZ = 0.0;
			scale = 0.64999998;
		}

		char* jump_slowdownEnable = Info_ValueForKey(cs1, "jump_slowdownEnable");
		if (*jump_slowdownEnable && atoi(jump_slowdownEnable) == 0)
			return;
		VectorScale(ps->velocity, scale, ps->velocity);
	}
}
uintptr_t resume_addr_PM_WalkMove;
__declspec(naked) void hook_PM_WalkMove_Naked()
{
	__asm
	{
		pushad;
		call Jump_ApplySlowdown;
		popad;
		jmp resume_addr_PM_WalkMove;
	}
}

void hook_PM_SlideMove(float primal_velocity_0, float primal_velocity_1, float primal_velocity_2)
{
	char* jump_slowdownEnable = Info_ValueForKey(cs1, "jump_slowdownEnable");
	if (*jump_slowdownEnable && atoi(jump_slowdownEnable) == 0)
		return;

	int* pm = (int*)(cgame_mp + 0x1a0ed0);
	playerState_t* ps = ((pmove_t*)*((int*)pm))->ps;
	if (ps->pm_time)
	{
		ps->velocity[0] = primal_velocity_0;
		ps->velocity[1] = primal_velocity_1;
		ps->velocity[2] = primal_velocity_2;
	}
}
uintptr_t resume_addr_PM_SlideMove;
__declspec(naked) void hook_PM_SlideMove_Naked()
{
	__asm
	{
		mov eax, dword ptr[esp + 0x110 - 0xA8]
		mov ecx, dword ptr[esp + 0x110 - 0xAC]
		mov edx, dword ptr[esp + 0x110 - 0xB0]

		push eax
		push ecx
		push edx

		call hook_PM_SlideMove
		add esp, 12

		jmp resume_addr_PM_SlideMove
	}
}

void Jump_GetLandFactor()
{
	int* pm = (int*)(cgame_mp + 0x1a0ed0);
	playerState_t* ps = ((pmove_t*)*((int*)pm))->ps;

	double factor;

	char* jump_slowdownEnable = Info_ValueForKey(cs1, "jump_slowdownEnable");
	if (*jump_slowdownEnable && atoi(jump_slowdownEnable) == 0)
		factor = 1.0;
	else if (ps->pm_time < 1700)
		factor = (double)ps->pm_time * 0.00088235294 + 1.0;
	else
		factor = 2.5;

	__asm fld factor
}
uintptr_t resume_addr_Jump_Start;
__declspec(naked) void hook_Jump_Start_Naked()
{
	__asm
	{
		pushad;
		call Jump_GetLandFactor;
		popad;
		jmp resume_addr_Jump_Start;
	}
}

void custom_PM_GetReducedFriction()
{
	double friction;

	char* jump_slowdownEnable = Info_ValueForKey(cs1, "jump_slowdownEnable");
	if (*jump_slowdownEnable && atoi(jump_slowdownEnable) == 0)
	{
		friction = 1.0;
	}
	else
	{
		int* pm = (int*)(cgame_mp + 0x1a0ed0);
		playerState_t* ps = ((pmove_t*)*((int*)pm))->ps;

		if (ps->pm_time < 1700)
			friction = (double)ps->pm_time * 0.00088235294 + 1.0;
		else
			friction = 2.5;
	}

	__asm fld friction
}
__declspec(naked) void custom_PM_GetReducedFriction_Naked()
{
	__asm
	{
		pushad
		call custom_PM_GetReducedFriction
		popad
		ret
	}
}
#endif

#ifdef PATCH_1_1
#include <stdint.h>
#include <math.h>
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
#endif

void CG_Init(DWORD base)
{
	cgame_mp = base;

#ifdef PATCH_1_1
	CG_Argv = (char* (*)(int))CGAME_OFF(0x30020960);
	CG_ServerCommand = (CG_ServerCommand_t)(cgame_mp + 0x2E0D0);

	__call(CGAME_OFF(0x3002E5A6), (int)_CG_ServerCommand);
	__call(CGAME_OFF(0x3001509E), (int)_CG_DrawFPS);
	__call(CGAME_OFF(0x300159CC), (int)CG_DrawDisconnect);
	__call(CGAME_OFF(0x300159D4), (int)CG_DrawDisconnect);

	__jmp(CGAME_OFF(0x3000D82B), (int)PM_Bounce_Stub);
#endif

#ifdef PATCH_1_1
	__jmp(CGAME_OFF(0x30032fe8), (int)sensitivityAimMultiply);
#elif PATCH_1_5
	__jmp(CGAME_OFF(0x30034688), (int)sensitivityAimMultiply);
#endif

#ifdef PATCH_1_5
	__jmp(CGAME_OFF(0x30008822), (int)hook_PM_WalkMove_Naked);
	resume_addr_PM_WalkMove = CGAME_OFF(0x300088be);

	__jmp(CGAME_OFF(0x3000e171), (int)hook_PM_SlideMove_Naked);
	resume_addr_PM_SlideMove = CGAME_OFF(0x3000e18e);

	__jmp(CGAME_OFF(0x30008320), (int)hook_Jump_Start_Naked);
	resume_addr_Jump_Start = CGAME_OFF(0x3000833a);

	__jmp(CGAME_OFF(0x30007ae0), (int)custom_PM_GetReducedFriction_Naked);
#endif

#ifdef PATCH_1_1
	__jmp(CGAME_OFF(0x30032f1e), (int)zoomFovMultiply_zooming_Naked);
	resume_addr_zoomFovMultiply_zooming = CGAME_OFF(0x30032f24);

	__jmp(CGAME_OFF(0x30032ea5), (int)zoomFovMultiply_zoomed_Naked);
	resume_addr_zoomFovMultiply_zoomed = CGAME_OFF(0x30032eab);
#endif

#ifdef PATCH_1_1
	__call(CGAME_OFF(0x30018896), (int)_CG_DrawWeaponSelect);
#endif
}