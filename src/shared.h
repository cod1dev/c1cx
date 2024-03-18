#include "common.h"

#define CVAR_ARCHIVE        1
#define CVAR_USERINFO       2
#define CVAR_ROM            64
#define CVAR_CHEAT          512

#ifdef PATCH_1_1
#define MAX_STRING_CHARS    1024 // max length of a string passed to Cmd_TokenizeString
#define MAX_STRING_TOKENS   256 // max tokens resulting from Cmd_TokenizeString
#define MAX_RELIABLE_COMMANDS 64
#define MAX_INFO_STRING     1024
#define MAX_INFO_KEY        1024
#define MAX_INFO_VALUE      1024
#endif
#define BIG_INFO_STRING     8192 // used for system info key only
#define BIG_INFO_KEY        8192
#define BIG_INFO_VALUE      8192
#ifdef PATCH_1_1
#define MAX_QPATH 64
#define MAX_OSPATH 256
#endif

typedef enum
{
	qfalse,
	qtrue
} qboolean;

typedef struct cvar_s
{
	char* name;
	char* string;
	char* resetString; // cvar_restart will reset to this value
	char* latchedString; // for CVAR_LATCH vars
	int flags;
	qboolean modified; // set each time the cvar is changed
	int modificationCount; // incremented each time the cvar is changed
	float value; // atof( string )
	int integer; // atoi( string )
	struct cvar_s* next;
	struct cvar_s* hashNext;
} cvar_t;

#ifdef PATCH_1_1
typedef enum
{
	CA_UNINITIALIZED,
	CA_DISCONNECTED = 0,
	CA_CONNECTING,
	CA_CHALLENGING,
	CA_CONNECTED,
} connstate_t;

typedef int fileHandle_t;
#endif

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];

#ifdef PATCH_1_1
#define Q_COLOR_ESCAPE  '^'
#define Q_IsColorString( p )  ( p && *( p ) == Q_COLOR_ESCAPE && *( ( p ) + 1 ) && *( ( p ) + 1 ) != Q_COLOR_ESCAPE )

#define DotProduct( x,y )         ( ( x )[0] * ( y )[0] + ( x )[1] * ( y )[1] + ( x )[2] * ( y )[2] )
#define VectorCopy( a,b )         ( ( b )[0] = ( a )[0],( b )[1] = ( a )[1],( b )[2] = ( a )[2] )

typedef struct
{
	netsrc_t	sock;
	int			dropped;			// between last packet and previous
	netadr_t	remoteAddress;
	int			qport;				// qport value to write when transmitting
	int			incomingSequence;
	int			outgoingSequence;
	// incoming fragment assembly buffer
	int			fragmentSequence;
	int			fragmentLength;
	byte		fragmentBuffer[MAX_MSGLEN];
	// outgoing fragment buffer
	// we need to space out the sending of large fragmented messages
	qboolean	unsentFragments;
	int			unsentFragmentStart;
	int			unsentLength;
	byte		unsentBuffer[MAX_MSGLEN];
} netchan_t;

typedef struct
{
	int			clientNum;
	int			lastPacketSentTime;			// for retransmits during connection
	int			lastPacketTime;				// for timeouts
	netadr_t	serverAddress;
	int			connectTime;				// for connection retransmits
	int			connectPacketCount;			// for display on connection dialog
	char		serverMessage[MAX_STRING_TOKENS];	// for display on connection dialog
	int			challenge;					// from the server to use for connecting
	int			checksumFeed;				// from the server for checksum calculations
	int			reliableSequence;
	int			reliableAcknowledge;		// the last one the server has executed
	char		reliableCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];
	// server message (unreliable) and command (reliable) sequence
	// numbers are NOT cleared at level changes, but continue to
	// increase as long as the connection is valid
	// message sequence is used by both the network layer and the
	// delta compression layer
	int			serverMessageSequence;
	// reliable messages received from server
	int			serverCommandSequence;
	int			lastExecutedServerCommand;		// last server command grabbed or executed with CL_GetServerCommand
	char		serverCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];
	// file transfer from server
	fileHandle_t download;
	char		downloadTempName[MAX_OSPATH];
	char		downloadName[MAX_OSPATH];
	int			downloadNumber;
	int			downloadBlock;	// block we are waiting for
	int			downloadCount;	// how many bytes we got
	int			downloadSize;	// how many bytes we got
	char		downloadList[MAX_INFO_STRING]; // list of paks we need to download
	qboolean	downloadRestart;	// if true, we need to do another FS_Restart because we downloaded a pak
	char		demoName[MAX_QPATH];
	qboolean	spDemoRecording;
	qboolean	demorecording;
	qboolean	demoplaying;
	qboolean	demowaiting;	// don't record until a non-delta message is received
	qboolean	firstDemoFrameSkipped;
	fileHandle_t	demofile;
	int			timeDemoFrames;		// counter of rendered frames
	int			timeDemoStart;		// cls.realtime before first frame
	int			timeDemoBaseTime;	// each frame will be at this time + frameNum * 50
	netchan_t	netchan;
} clientConnection_t;
#endif

typedef enum
{
	PM_NORMAL = 0x0,
	PM_NORMAL_LINKED = 0x1,
	PM_NOCLIP = 0x2,
	PM_UFO = 0x3,
	PM_SPECTATOR = 0x4,
	PM_INTERMISSION = 0x5,
	PM_DEAD = 0x6,
	PM_DEAD_LINKED = 0x7,
} pmtype_t;

typedef enum
{
	WEAPON_READY = 0x0,
	WEAPON_RAISING = 0x1,
	WEAPON_DROPPING = 0x2,
	WEAPON_FIRING = 0x3,
	WEAPON_RECHAMBERING = 0x4,
	WEAPON_RELOADING = 0x5,
	WEAPON_RELOADING_INTERUPT = 0x6,
	WEAPON_RELOAD_START = 0x7,
	WEAPON_RELOAD_START_INTERUPT = 0x8,
	WEAPON_RELOAD_END = 0x9,
	WEAPON_MELEE_INIT = 0xA,
	WEAPON_MELEE_FIRE = 0xB,
	WEAPONSTATES_NUM = 0xC,
} weaponstate_t;

#ifdef PATCH_1_5
typedef struct WeaponDef_t
{
	int number;
	char* name;
	char* displayName;
	byte pad[0x1E4];
	int reloadAddTime;
	byte pad2[0x20];
	float moveSpeedScale;
	float adsZoomFov;
	float adsZoomInFrac;
	float adsZoomOutFrac;
	byte pad3[0x44];
	int adsTransInTime;
	int adsTransOutTime;
	byte pad4[0x8];
	float idleCrouchFactor;
	float idleProneFactor;
	byte pad5[0x50];
	int rechamberWhileAds;
	float adsViewErrorMin;
	float adsViewErrorMax;
	byte pad6[0x14C];
	float OOPosAnimLength[2];
	//...
} WeaponDef_t;

struct WeaponProperties // Custom struct for g_legacyStyle
{
	int reloadAddTime;
	int adsTransInTime;
	float adsZoomInFrac;
	float idleCrouchFactor;
	float idleProneFactor;
	int rechamberWhileAds;
	float adsViewErrorMin;
	float adsViewErrorMax;
};
#endif

typedef struct playerState_s
{
	int commandTime;
	pmtype_t pm_type;
	int bobCycle;
	int pm_flags;
	int pm_time;
	vec3_t origin;
	vec3_t velocity;
	vec2_t oldVelocity;
	int weaponTime;
	int weaponDelay;
	int gravity;
	float leanf;
	int speed;
	vec3_t delta_angles;
	int groundEntityNum;
	vec3_t vLadderVec;
	int jumpTime;
	float jumpOriginZ;
	int legsTimer;
	int legsAnim;
	int torsoTimer;
	int torsoAnim;
	int movementDir;
	int eFlags;
	int eventSequence;
	int events[4];
	unsigned int eventParms[4];
	int oldEventSequence;
	int clientNum;
	unsigned int weapon;
	weaponstate_t weaponstate;
	float fWeaponPosFrac;
	int adsDelayTime;
	//TODO: check if one of two the above is "int viewmodelIndex" instead
	vec3_t viewangles;
#ifdef PATCH_1_1
	byte pad[8196];
#elif PATCH_1_5
	byte pad[8192];
#endif
} playerState_t;

typedef struct usercmd_s
{
	int serverTime;
	byte buttons; // console, chat, ads, attack, use
	byte wbuttons; // lean left, lean right, reload
	byte weapon;
	byte flags;
	int angles[3];
	signed char forwardmove, rightmove, upmove;
	byte unknown;
} usercmd_t;

struct pmove_t
{
	playerState_t* ps;
	usercmd_t cmd;
	// some remains
};

typedef void(*Cvar_Set_t)(char*, char*);
typedef cvar_t* (*Cvar_Get_t)(const char*, const char*, int);

#ifdef PATCH_1_1
typedef cvar_t* (*Cvar_FindVar_t)(const char*);
#endif

extern Cvar_Set_t Cvar_Set;
extern Cvar_Get_t Cvar_Get;

#ifdef PATCH_1_1
extern Cvar_FindVar_t Cvar_FindVar;

char* Cvar_VariableString(const char*);
int Cvar_VariableIntegerValue(const char* var_name);

void Q_strncpyz(char *dest, const char *src, int destsize);
void Com_sprintf(char *dest, int size, const char *fmt, ...);
char* Cmd_Argv(int index);
int Cmd_Argc();

void Info_SetValueForKey(char *s, const char *key, const char *value);
#endif

char* Info_ValueForKey(const char *s, const char *key);

extern DWORD game_mp;
extern DWORD cgame_mp;
#define GAME_OFF(x) (game_mp + (x - 0x20000000))
#define CGAME_OFF(x) (cgame_mp + (x - 0x30000000))

#ifdef PATCH_1_1
typedef void(*CL_BeginDownload_t)(const char*, const char*);
static CL_BeginDownload_t CL_BeginDownload = (CL_BeginDownload_t)0x4100D0;
typedef void(*CL_NextDownload_t)(void);
static CL_NextDownload_t CL_NextDownload = (CL_NextDownload_t)0x410190;

char* Q_CleanStr(char* string, bool colors = false);

char* Com_CleanHostname(char* hostname, bool colors);
#endif