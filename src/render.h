#include "stdafx.h"
#include "gl/gl.h"
#include "gl/glu.h"
#pragma comment(lib, "opengl32")
#pragma comment(lib, "glu32.lib")

#ifdef PATCH_1_1
static float vColorBlack[4] = { 0,0,0,1 };
static float vColorWhite[4] = { 1, 1, 1, 1 };
static float vColorSelected[4] = { 1, 1, 0, 1 };

typedef void(*RE_SetColor_t)(const float *rgba);
extern RE_SetColor_t RE_SetColor;
typedef void (QDECL *SCR_DrawString_t)(float x, float y, int font, float scale, float* color, const char* text, float spacing, int limit, int flags);
extern SCR_DrawString_t SCR_DrawString;
#endif