static void XUNLOCK(void *addr, size_t len)
{
#if _WIN32
	DWORD tmp;
	VirtualProtect(addr, len, PAGE_EXECUTE_READWRITE, &tmp);
#endif
}

static void _memset(void* addr, int v, size_t len)
{
#ifdef _WIN32
	DWORD tmp;
	VirtualProtect((void*)addr, len, PAGE_EXECUTE_READWRITE, &tmp);
#endif
	memset(addr, v, len);
	FlushInstructionCache(GetCurrentProcess(), (void*)addr, len);
}

static void _memcpy(void* v1, void* v2, size_t len)
{
#ifdef _WIN32
	DWORD tmp;
	VirtualProtect((void*)v1, len, PAGE_EXECUTE_READWRITE, &tmp);
#endif
	memcpy(v1, v2, len);
	FlushInstructionCache(GetCurrentProcess(), (void*)v1, len);
}

static int search_memory(int current, int end, BYTE* bytes, size_t len)
{
	while ((memcmp((void*)current, bytes, len)) && (current < end))
		current++;
	return (current == end) ? -1 : current;
}

static int patch_memory(int start, int end, BYTE* search_for, BYTE* patched, size_t len)
{
	int s = search_memory(start, end, search_for, len);
#ifdef _WIN32
	DWORD tmp;
#endif
	if (-1 != s) {
#ifdef _WIN32
		VirtualProtect((void*)s, len, PAGE_EXECUTE_READWRITE, &tmp);
#endif
		memcpy((void*)s, patched, len);
		FlushInstructionCache(GetCurrentProcess(), (void*)s, len);
#ifdef _WIN32
		VirtualProtect((void*)s, len, tmp, &tmp);
#endif
	}
	return s;
}

/*
Hooking
*/
static void __nop(unsigned int start, unsigned int end)
{
	int len = (end < start) ? end : (end - start);
#ifdef _WIN32
	DWORD tmp;
	VirtualProtect((void*)start, len, PAGE_EXECUTE_READWRITE, &tmp);
#endif
	memset((void*)start, 0x90, len);
	FlushInstructionCache(GetCurrentProcess(), (void*)start, len);
#ifdef _WIN32
	VirtualProtect((void*)start, len, tmp, &tmp);
#endif
}

static void __jmp(unsigned int off, unsigned int loc)
{
#ifdef _WIN32
	DWORD tmp;
	VirtualProtect((void*)off, 5, PAGE_EXECUTE_READWRITE, &tmp);
#endif
	*(unsigned char*)off = 0xe9;
	int foffset = loc - (off + 5);
	memcpy((void*)(off + 1), &foffset, 4);
	FlushInstructionCache(GetCurrentProcess(), (void*)off, 5);
#ifdef _WIN32
	VirtualProtect((void*)off, 5, tmp, &tmp);
#endif
}

static void __call(unsigned int off, unsigned int loc)
{
#ifdef _WIN32
	DWORD tmp;
	VirtualProtect((void*)off, 5, PAGE_EXECUTE_READWRITE, &tmp);
#endif
	int foffset = loc - (off + 5);
	memcpy((void *)(off + 1), &foffset, 4);
	FlushInstructionCache(GetCurrentProcess(), (void*)off, 5);
#ifdef _WIN32
	VirtualProtect((void*)off, 5, tmp, &tmp);
#endif
}