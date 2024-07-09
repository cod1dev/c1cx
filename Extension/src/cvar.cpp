#include <windows.h>
#include "shared.h"

Cvar_Get_t Cvar_Get = (Cvar_Get_t)0x439350;
Cvar_Set_t Cvar_Set = (Cvar_Set_t)0x439650;

Cvar_FindVar_t Cvar_FindVar = (Cvar_FindVar_t)0x439280;

char* Cvar_VariableString(const char* var_name)
{
	cvar_t* var = Cvar_FindVar(var_name);
	if (!var)
		return (char*)"";
	return var->string;
}
int Cvar_VariableIntegerValue(const char* var_name)
{
	cvar_t* var = Cvar_FindVar(var_name);
	if (!var)
		return 0;
	return var->integer;
}