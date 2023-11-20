#include "shared.h"
#include "client.h"

UINT_PTR pfield_charevent_return = (UINT_PTR)0x40CB77;
UINT_PTR pfield_charevent_continue = (UINT_PTR)0x40CB23;
__declspec(naked) void Field_CharEvent_IgnoreTilde()
{
	__asm
	{
		cmp ebx, 20h
		jge check
		jmp pfield_charevent_return

		check :
		cmp ebx, 126
			jl checked
			jmp pfield_charevent_return

			checked :
		jmp pfield_charevent_continue
	}
}
// cmp ebx, 20h is 3 bytes, we need 5 for a jmp...
// jl ... is 2 bytes 7c54 (assuming when subtracing the addresses)
// so it works out
// - Richard