#include "hooking.h"

void hook_jmp(int from, int to)
{
    int relative = to - (from + 5); // +5 is the position of next opcode
    memset((void*)from, 0xE9, 1); // JMP-OPCODE
    memcpy((void*)(from + 1), &relative, 4); // set relative address with endian
}

cHook::cHook(int from, int to)
{
    this->from = from;
    this->to = to;
}

void cHook::hook()
{
    memcpy((void*)oldCode, (void*)from, 5);
    hook_jmp(from, to);
}

void cHook::unhook()
{
    memcpy((void*)from, (void*)oldCode, 5);
}