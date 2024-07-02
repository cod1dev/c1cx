#include <windows.h>

static void __nop(unsigned int start, unsigned int end)
{
	int len = (end < start) ? end : (end - start);
	DWORD tmp;
	VirtualProtect((void*)start, len, PAGE_EXECUTE_READWRITE, &tmp);
	memset((void*)start, 0x90, len);
	FlushInstructionCache(GetCurrentProcess(), (void*)start, len);
	VirtualProtect((void*)start, len, tmp, &tmp);
}

static void __call(unsigned int off, unsigned int loc)
{
	DWORD tmp;
	VirtualProtect((void*)off, 5, PAGE_EXECUTE_READWRITE, &tmp);
	int foffset = loc - (off + 5);
	memcpy((void*)(off + 1), &foffset, 4);
	FlushInstructionCache(GetCurrentProcess(), (void*)off, 5);
	VirtualProtect((void*)off, 5, tmp, &tmp);
}

static void __jmp(unsigned int off, unsigned int loc)
{
	DWORD tmp;
	VirtualProtect((void*)off, 5, PAGE_EXECUTE_READWRITE, &tmp);
	* (unsigned char*)off = 0xe9;
	int foffset = loc - (off + 5);
	memcpy((void*)(off + 1), &foffset, 4);
	FlushInstructionCache(GetCurrentProcess(), (void*)off, 5);
	VirtualProtect((void*)off, 5, tmp, &tmp);
}

static void XUNLOCK(void* addr, size_t len)
{
	DWORD tmp;
	VirtualProtect(addr, len, PAGE_EXECUTE_READWRITE, &tmp);
}

void hook_jmp(int from, int to);

class cHook
{
public:
	int from;
	int to;
	unsigned char oldCode[5];
	cHook(int from, int to);
	void hook();
	void unhook();
};