#define cls_realtime ((int*)0x155F3E0)
#define cls_state ((int*)0x155F2C0)
#define cls_downloadRestart ((PINT)0x15EEFBC)
#define clc_downloadList ((PCHAR)0x15EEBBC)
#define clc_stringData ((PCHAR)0x1436A7C)
#define clc_stringOffsets ((PINT)0x1434A7C)
#define clc_serverAddress ((netadr_t*)0x15CE86C)
#define clc_demoplaying ((PINT)0x15EF004)
#define cs1 (clc_stringData + clc_stringOffsets[1])

#define gameWindow ((HWND*)0x16C35E8)

static bool unlock_client_structure()
{
	__try
	{
		XUNLOCK((void*)cls_realtime, sizeof(int));
		XUNLOCK((void*)cls_state, sizeof(int));
		XUNLOCK((void*)cls_downloadRestart, 4);
		XUNLOCK((void*)0x155F2C0, 4);
		XUNLOCK((void*)clc_demoplaying, 4);
		XUNLOCK((void*)clc_downloadList, 4096);
		XUNLOCK((void*)0x15EEAAC, 64);
	}
	__except (1)
	{
		return false;
	}
	return true;
}