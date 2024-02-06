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

static void __call(unsigned int off, unsigned int loc)
{
#ifdef _WIN32
	DWORD tmp;
	VirtualProtect((void*)off, 5, PAGE_EXECUTE_READWRITE, &tmp);
#endif
	int foffset = loc - (off + 5);
	memcpy((void*)(off + 1), &foffset, 4);
	FlushInstructionCache(GetCurrentProcess(), (void*)off, 5);
#ifdef _WIN32
	VirtualProtect((void*)off, 5, tmp, &tmp);
#endif
}

static void __jmp(unsigned int off, unsigned int loc)
{
#ifdef _WIN32
	DWORD tmp;
	VirtualProtect((void*)off, 5, PAGE_EXECUTE_READWRITE, &tmp);
#endif
	* (unsigned char*)off = 0xe9;
	int foffset = loc - (off + 5);
	memcpy((void*)(off + 1), &foffset, 4);
	FlushInstructionCache(GetCurrentProcess(), (void*)off, 5);
#ifdef _WIN32
	VirtualProtect((void*)off, 5, tmp, &tmp);
#endif
}

static void XUNLOCK(void* addr, size_t len)
{
#if _WIN32
	DWORD tmp;
	VirtualProtect(addr, len, PAGE_EXECUTE_READWRITE, &tmp);
#endif
}