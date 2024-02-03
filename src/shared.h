#include "common.h"

#define CVAR_ARCHIVE        1   // set to cause it to be saved to vars.rc
#define CVAR_USERINFO       2   // sent to server on connect or change
#define CVAR_ROM            64  // display only, cannot be set by user at all

#define MAX_STRING_CHARS    1024 // max length of a string passed to Cmd_TokenizeString
#define MAX_STRING_TOKENS   256 // max tokens resulting from Cmd_TokenizeString
#define MAX_RELIABLE_COMMANDS 64
#define MAX_INFO_STRING     1024
#define MAX_INFO_KEY        1024
#define MAX_INFO_VALUE      1024
#define BIG_INFO_STRING     8192 // used for system info key only
#define BIG_INFO_KEY        8192
#define BIG_INFO_VALUE      8192
#define MAX_QPATH 64
#define MAX_OSPATH 256

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

typedef enum
{
	CA_UNINITIALIZED,
	CA_DISCONNECTED = 0,
	CA_CONNECTING,
	CA_CHALLENGING,
	CA_CONNECTED,
} connstate_t;

typedef int fileHandle_t;
typedef float vec_t;
typedef vec_t vec3_t[3];

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

typedef struct playerState_s
{
	int commandTime;
	int pm_type;
	int bobCycle;
	int pm_flags;
	int pm_time;
	vec3_t origin;
	vec3_t velocity;
	char gap_2C[20];
	float leanf;
	int speed;
	char gap_48[12];
	int groundEntityNum;
	char gap_58[12];
	int jumpTime;
	int field_68;
	int legsTime;
	int legsAnim;
	int torsoTime;
	int torsoAnim;
	int movementDir;
	int eFlags;
	char gap_84[24];
	int field_9C;
	int field_A0;
	int field_A4;
	int field_A8;
	int clientNum;
	int weapon;
	int field_B4;
	char gap_B8;
	char gap_B9;
	char gap_BA[2];
	int field_BC;
	vec3_t viewangles;
	char gap_CC[40];
	int health;
	char gap_F8[556];
	vec3_t mins;
	vec3_t maxs;
	float viewheight_prone;
	int viewheight_crouched;
	float viewheight_standing;
	int field_348;
	float runSpeedScale;
	float sprintSpeedScale;
	char gap_354[40];
	float friction;
	char gap_380[68];
	float fTorsoHeight;
	float fTorsoPitch;
	float fWaistPitch;
	char rest[7416];
	int end;
} playerState_t;

typedef struct usercmd_s
{
	int serverTime;
	byte buttons; //console,chat talking, aim down the sight, attackbutton, usebutton
	byte wbuttons; //lean right,left,reload
	byte weapon;
	byte flags;
	int angles[3];
	signed char forwardmove, rightmove, upmove;
	byte unknown; //could be doubleTap or client
} usercmd_t;

typedef struct //usercmd_s i defined in server.h?
{ 
	playerState_t* ps;
	usercmd_t cmd;
	//some remaining
} pmove_t;

typedef void(*Cvar_Set_t)(char*, char*);
typedef cvar_t* (*Cvar_Get_t)(const char*, const char*, int);
typedef cvar_t* (*Cvar_FindVar_t)(const char*);

extern Cvar_Set_t Cvar_Set;
extern Cvar_Get_t Cvar_Get;
extern Cvar_FindVar_t Cvar_FindVar;

char* Cvar_VariableString(const char*);

void Q_strncpyz(char *dest, const char *src, int destsize);
void Com_sprintf(char *dest, int size, const char *fmt, ...);
char* Cmd_Argv(int index);
int Cmd_Argc();

void Info_SetValueForKey(char *s, const char *key, const char *value);
char* Info_ValueForKey(const char *s, const char *key);

extern DWORD game_mp;
extern DWORD cgame_mp;

#define GAME_OFF(x) (game_mp + (x - 0x20000000))
#define CGAME_OFF(x) (cgame_mp + (x - 0x30000000))

typedef void(*CL_BeginDownload_t)(const char*, const char*);
static CL_BeginDownload_t CL_BeginDownload = (CL_BeginDownload_t)0x4100D0;
typedef void(*CL_NextDownload_t)(void);
static CL_NextDownload_t CL_NextDownload = (CL_NextDownload_t)0x410190;

char* Q_CleanStr(char* string, bool colors = false);

char* Com_CleanHostname(char* hostname, bool colors);