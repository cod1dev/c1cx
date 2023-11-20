#include "shared.h"
#include "client.h"
#include "render.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

typedef void(*CG_ServerCommand_t)();
CG_ServerCommand_t CG_ServerCommand;

const char* disliked_vars[] =
{
	"r_showimages",
	"name",
	"cl_allowdownload",
	"version",
	"cg_norender",
	"cl_avidemo",
	NULL
};

DWORD cgame_mp;

char *(*CG_Argv)(int) = nullptr;
void myCG_ServerCommand(void)
{
	int argc = Cmd_Argc();
	#if 0
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
			if ((*cmd == 'h' || *cmd == 'i'))
			{

			}
			else if (*cmd == 'b')
			{
				Com_DPrintf("[CG_ParseScores] b ");
				for (size_t i = 0; i < argc; i++) {
					Com_DPrintf("%s ", Cmd_Argv(i));
				}
				Com_DPrintf("\n");
			}
			else if (*cmd == 'v')
			{
				if (argc > 1)
				{
					char* var = Cmd_Argv(1);
					for (int i = 0; disliked_vars[i]; i++)
					{
						if (!strcmp(disliked_vars[i], var))
							return;
					}
				}
			}
		}
	}
	CG_ServerCommand();
}

void pm_aimflag() //Used to aim in the air
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

extern cvar_t* cl_sensitivityAimMultiply;
float stockCgZoomSensitivity()
{
	float* fov_visible_percentage = (float*)CGAME_OFF(0x3020958c);	//Visible percentage of cg_fov value
	float* cg_fov_value = (float*)CGAME_OFF(0x30298c68);
	return (*fov_visible_percentage / *cg_fov_value);				//See instruction 30032fe8
}
void sensitivityAimMultiply()
{
	float* cg_zoomSensitivity = (float*)CGAME_OFF(0x3020b5f4);		//zoomSensitivity var of cg_t struct
	float* ads_anim_progress = (float*)CGAME_OFF(0x30207214);		//From 0 to 1
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

typedef enum
{
	TEAM_FREE,
	TEAM_AXIS,
	TEAM_ALLIES,
	TEAM_SPECTATOR,
	TEAM_NUM_TEAMS
} team_t;
typedef struct
{
	int snapsFlags;
	int ping;
	int serverTime;
	//rest
} snapshot_t;
typedef struct
{
	int unk;
	int infoValid;
	int clientNum;
	char name[32];
	team_t team;
	//down here model names and attachments and rest of client info
} clientInfo_t;

#define M_DrawShadowString(x,y,font,fontscale,color,text,a,b,c) \
	RE_SetColor(vColorBlack); \
	SCR_DrawString(x + 1,y + 1,font,fontscale,vColorBlack,text,a,b,c); \
	RE_SetColor(color); \
	SCR_DrawString(x,y,font,fontscale,color,text,a,b,c); \
	RE_SetColor(NULL);

void CG_DrawDisconnect()
{
	cvar_t* xui_interrupted = Cvar_Get("cg_xui_interrupted", "0", CVAR_ARCHIVE);
	if (xui_interrupted->integer)
	{
		void(*call)();
		*(int*)&call = CGAME_OFF(0x30015450);
		call();
	}
}

void CG_DrawFPS(float y)
{
	cvar_t* xui_fps = Cvar_Get("cg_xui_fps", "1", CVAR_ARCHIVE);

	if (xui_fps->integer) {
		cvar_t* x = Cvar_Get("cg_xui_fps_x", "597", CVAR_ARCHIVE);
		cvar_t* y = Cvar_Get("cg_xui_fps_y", "8", CVAR_ARCHIVE);

		#define	FPS_FRAMES 4
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

		if (index > FPS_FRAMES) {
			total = 0;
			for (i = 0; i < FPS_FRAMES; i++) {
				total += previousTimes[i];
			}
			if (!total) {
				total = 1;
			}
			fps = 1000 * FPS_FRAMES / total;

			M_DrawShadowString(x->integer, y->integer, 1, .20, vColorWhite, va("FPS: %d", fps), NULL, NULL, NULL);
		}
	} else {
		void(*call)(float);
		*(int*)&call = CGAME_OFF(0x30014A00);
		call(y);
	}
}

typedef enum
{
	TR_STATIONARY,
	TR_INTERPOLATE,             // non-parametric, but interpolate between snapshots
	TR_LINEAR,
	TR_LINEAR_STOP,
	TR_LINEAR_STOP_BACK,        //----(SA)	added.  so reverse movement can be different than forward
	TR_SINE,                    // value = base + sin( time / duration ) * delta
	TR_GRAVITY,
	// Ridah
	TR_GRAVITY_LOW,
	TR_GRAVITY_FLOAT,           // super low grav with no gravity acceleration (floating feathers/fabric/leaves/...)
	TR_GRAVITY_PAUSED,          //----(SA)	has stopped, but will still do a short trace to see if it should be switched back to TR_GRAVITY
	TR_ACCELERATE,
	TR_DECCELERATE,
	// Gordon
	TR_SPLINE,
	TR_LINEAR_PATH
} trType_t;
typedef struct
{
	trType_t trType;
	int trTime;
	int trDuration;             // if non 0, trTime + trDuration = stop time
//----(SA)	removed
	vec3_t trBase;
	vec3_t trDelta;             // velocity, etc
//----(SA)	removed
} trajectory_t;
typedef struct entityState_s
{
	int number;
	entityTypes eType; //4
	int eFlags; //8
	trajectory_t pos; //12
	trajectory_t apos; //48
	int unk; //84 //time??
	int unk2; //88 //time2??
	vec3_t origin2; //92
	vec3_t angles2; //104 (guessed name)
	int otherEntityNum; //116
	int otherEntityNum2; //120
	int groundEntityNum; //124
	int constantLight; //128
	int loopSound; //132
	int surfaceFlags; //136
	int modelindex; //140
	int clientNum; //144
	char ___cba[0x34];
	/*
	gentity_t *teammaster; //152
	int eventParm; //160
	int eventSequence; //164
	int events[4]; //168
	int eventParms[4]; //184
	*/
	int weapon; //200
	int legsAnim; //204
	int torsoAnim; //208
	float leanf; //212
	int loopfxid; //216
	int hintstring; //220
	int animMovetype; //224
} entityState_t;

void PM_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out)
{
	float	backoff;
	float	change;
	int		i;
	float   overbounce = 1.001f;

	backoff = DotProduct(in, normal);

	if (backoff < 0) {
		backoff *= overbounce;
	}
	else {
		backoff /= overbounce;
	}

	for (i = 0; i < 3; i++) {
		change = normal[i] * backoff;
		out[i] = in[i] - change;
	}
}

// by xoxor4d
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

	if (x_cl_bounce) {
		PM_ProjectVelocity(in, normal, out);
	}
	else {
		PM_ClipVelocity(in, normal, out);
	}

	return CGAME_OFF(0x3000D830);
}
__declspec(naked) void PM_Bounce_Stub()
{
	__asm
	{
		push    esi; // out
		push    ecx; // normal
		push    edx; // in
		call    PM_Bounce;
		add     esp, 12;

		push eax
			retn;
	}
}

static void (*PM_CheckForChangeWeapon)();
static void (*PM_BeginWeaponChange)(int, int);
static void (*PM_FinishWeaponChange)();
void _PM_CheckForChangeWeapon()
{
	int* pm = (int*)(cgame_mp + 0x19D570);
	pmove_t* xm = *(pmove_t**)(int)pm;

	if ((xm->ps->pm_flags & 0x20000))
	{
		int* weapon = (int*)((int)xm->ps + 176);
		if (*weapon)
		{
			PM_BeginWeaponChange(*weapon, 0);
		}
		return;
	}
	PM_CheckForChangeWeapon();
}

void _PM_FinishWeaponChange()
{
	int* pm = (int*)(cgame_mp + 0x19D570);
	pmove_t* xm = *(pmove_t**)(int)pm;

	if ((xm->ps->pm_flags & 0x20000))
	{
		int* weapon = (int*)((int)xm->ps + 176);
		if (*weapon)
		{
			*weapon = 0;
		}
		return;
	}
	PM_FinishWeaponChange();
}

void CG_Init(DWORD base)
{
	cgame_mp = base;
	CG_ServerCommand = (CG_ServerCommand_t)(cgame_mp + 0x2E0D0);
	CG_Argv = (char*(*)(int))CGAME_OFF(0x30020960);

	__call(CGAME_OFF(0x3002E5A6), (int)myCG_ServerCommand);

	__call(CGAME_OFF(0x3000C799), (int)pm_aimflag);
	__call(CGAME_OFF(0x3000C7B8), (int)pm_aimflag);
	__call(CGAME_OFF(0x3000C7D2), (int)pm_aimflag);
	__call(CGAME_OFF(0x3000C7FF), (int)pm_aimflag);
	__call(CGAME_OFF(0x3000C858), (int)pm_aimflag);
	__call(CGAME_OFF(0x3000C893), (int)pm_aimflag);

	__jmp(CGAME_OFF(0x30032fe8), (int)sensitivityAimMultiply);

	__call(CGAME_OFF(0x300159CC), (int)CG_DrawDisconnect);
	__call(CGAME_OFF(0x300159D4), (int)CG_DrawDisconnect);
	__call(CGAME_OFF(0x3001509E), (int)CG_DrawFPS);

	*(UINT32*)CGAME_OFF(0x300749EC) = 1; // Unlock cg_fov

	__jmp(CGAME_OFF(0x3000D82B), (int)PM_Bounce_Stub);

	__call(CGAME_OFF(0x30011C25), (int)_PM_CheckForChangeWeapon);
	__call(CGAME_OFF(0x30011CB4), (int)_PM_FinishWeaponChange);
	PM_CheckForChangeWeapon = (void(*)())CGAME_OFF(0x300112E0);
	PM_BeginWeaponChange = (void(*)(int, int))CGAME_OFF(0x30010570);
	PM_FinishWeaponChange = (void(*)())CGAME_OFF(0x300107c0);
}