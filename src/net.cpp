#include "shared.h"
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <wsipx.h>
#include "client.h"

NET_SendPacket_t NET_SendPacket = (NET_SendPacket_t)0x4493E0;
NET_OutOfBandPrint_t NET_OutOfBandPrint = (NET_OutOfBandPrint_t)0x449490;
CL_Netchan_Encode_t CL_Netchan_Encode = (CL_Netchan_Encode_t)0x414710;
Netchan_Transmit_t Netchan_Transmit = (Netchan_Transmit_t)0x448B70;
Netchan_TransmitNextFragment_t Netchan_TransmitNextFragment = (Netchan_TransmitNextFragment_t)0x448980;
MSG_initHuffman_t MSG_initHuffman = (MSG_initHuffman_t)0x448390;

void MSG_Init(msg_t *buf, byte *data, int length) {
	if (!*(int*)0x14073F0)
		MSG_initHuffman();
	memset(buf, 0, sizeof(*buf));
	buf->data = data;
	buf->maxsize = length;
}

#define DO( src,dest )	  \
	copy[0] = s[src];	\
	copy[1] = s[src + 1];	\
	sscanf( copy, "%x", &val );	  \
	( (struct sockaddr_ipx *)sadr )->dest = val

qboolean Sys_StringToSockaddr(const char *s, struct sockaddr *sadr) {
	struct hostent  *h;
	int val;
	char copy[MAX_STRING_CHARS];

	memset(sadr, 0, sizeof(*sadr));

	// check for an IPX address
	// rain - too easy to falsely match a real hostname
	//	if( ( strlen( s ) == 21 ) && ( s[8] == '.' ) ) {
	((struct sockaddr_in *)sadr)->sin_family = AF_INET;
	((struct sockaddr_in *)sadr)->sin_port = 0;

	if (s[0] >= '0' && s[0] <= '9') {
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = inet_addr(s);
	}
	else {
		if ((h = gethostbyname(s)) == 0) {
			return qfalse;
		}
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = *(int *)h->h_addr_list[0];
	}

	return qtrue;
}

#undef DO

void SockadrToNetadr(struct sockaddr *s, netadr_t *a) {
	a->type = NA_IP;
	*(int *)&a->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;
	a->port = ((struct sockaddr_in *)s)->sin_port;
}

void NetadrToSockadr(netadr_t *a, struct sockaddr *s) {
	memset(s, 0, sizeof(*s));

	if (a->type == NA_BROADCAST) {
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_port = a->port;
		((struct sockaddr_in *)s)->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if (a->type == NA_IP)    {
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_addr.s_addr = *(int *)&a->ip;
		((struct sockaddr_in *)s)->sin_port = a->port;
	}
}

qboolean Sys_StringToAdr(const char *s, netadr_t *a) {
	struct sockaddr sadr;

	if (!Sys_StringToSockaddr(s, &sadr)) {
		return qfalse;
	}

	SockadrToNetadr(&sadr, a);
	return qtrue;
}

//tmp relocation
short   BigShort(short l) {
	byte b1, b2;

	b1 = l & 255;
	b2 = (l >> 8) & 255;

	return (b1 << 8) + b2;
}

qboolean    NET_StringToAdr(const char *s, netadr_t *a) {
	qboolean r;
	char base[MAX_STRING_CHARS];
	char    *port;

	if (!strcmp(s, "localhost")) {
		memset(a, 0, sizeof(*a));
		a->type = NA_LOOPBACK;
		return qtrue;
	}

	// look for a port number
	Q_strncpyz(base, s, sizeof(base));
	port = strstr(base, ":");
	if (port) {
		*port = 0;
		port++;
	}

	r = Sys_StringToAdr(base, a);

	if (!r) {
		a->type = NA_BAD;
		return qfalse;
	}

	// inet_addr returns this if out of range
	if (a->ip[0] == 255 && a->ip[1] == 255 && a->ip[2] == 255 && a->ip[3] == 255) {
		a->type = NA_BAD;
		return qfalse;
	}

	if (port) {
		a->port = BigShort((short)atoi(port));
	}
	else {
		a->port = BigShort(28960);
	}

	return qtrue;
}

const char  *NET_AdrToString(netadr_t a) {
	static  char    s[64];

	if (a.type == NA_LOOPBACK) {
		Com_sprintf(s, sizeof(s), "loopback");
	}
	else if (a.type == NA_BOT) {
		Com_sprintf(s, sizeof(s), "bot");
	}
	else if (a.type == NA_IP) {
		Com_sprintf(s, sizeof(s), "%i.%i.%i.%i:%hu",
			a.ip[0], a.ip[1], a.ip[2], a.ip[3], BigShort(a.port));
	}
	else {
		Com_sprintf(s, sizeof(s), "%02x%02x%02x%02x.%02x%02x%02x%02x%02x%02x:%hu",
			a.ipx[0], a.ipx[1], a.ipx[2], a.ipx[3], a.ipx[4], a.ipx[5], a.ipx[6], a.ipx[7], a.ipx[8], a.ipx[9],
			BigShort(a.port));
	}

	return s;
}

const char  *NET_BaseAdrToString(netadr_t a) {
	static  char    s[64];

	if (a.type == NA_LOOPBACK) {
		Com_sprintf(s, sizeof(s), "loopback");
	}
	else if (a.type == NA_BOT) {
		Com_sprintf(s, sizeof(s), "bot");
	}
	else if (a.type == NA_IP) {
		Com_sprintf(s, sizeof(s), "%i.%i.%i.%i",
			a.ip[0], a.ip[1], a.ip[2], a.ip[3]);
	}
	else {
		Com_sprintf(s, sizeof(s), "%02x%02x%02x%02x.%02x%02x%02x%02x%02x%02x",
			a.ipx[0], a.ipx[1], a.ipx[2], a.ipx[3], a.ipx[4], a.ipx[5], a.ipx[6], a.ipx[7], a.ipx[8], a.ipx[9]);
	}

	return s;
}

qboolean    NET_CompareAdr(netadr_t a, netadr_t b) {
	if (a.type != b.type) {
		return qfalse;
	}

	if (a.type == NA_LOOPBACK) {
		return qtrue;
	}

	if (a.type == NA_IP) {
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port) {
			return qtrue;
		}
		return qfalse;
	}

	if (a.type == NA_IPX) {
		if ((memcmp(a.ipx, b.ipx, 10) == 0) && a.port == b.port) {
			return qtrue;
		}
		return qfalse;
	}

	Com_Printf("NET_CompareAdr: bad address type\n");
	return qfalse;
}

bool    NET_IsLocalAddress(netadr_t adr) {
	return adr.type == NA_LOOPBACK;
}

qboolean    NET_CompareBaseAdr(netadr_t a, netadr_t b) {
	if (a.type != b.type) {
		return qfalse;
	}

	if (a.type == NA_LOOPBACK) {
		return qtrue;
	}

	if (a.type == NA_IP) {
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3]) {
			return qtrue;
		}
		return qfalse;
	}

	if (a.type == NA_IPX) {
		if ((memcmp(a.ipx, b.ipx, 10) == 0)) {
			return qtrue;
		}
		return qfalse;
	}


	Com_Printf("NET_CompareBaseAdr: bad address type\n");
	return qfalse;
}