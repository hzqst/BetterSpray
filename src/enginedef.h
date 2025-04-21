#pragma once

#include <metahook.h>

typedef enum
{
	GLT_UNKNOWN = -1,
	GLT_SYSTEM = 0,
	GLT_DECAL,
	GLT_HUDSPRITE,
	GLT_STUDIO,
	GLT_WORLD,
	GLT_SPRITE,
	GLT_DETAIL//SvEngine only, blocked
}GL_TEXTURETYPE;


#define TEX_TYPE_NONE	0
#define TEX_TYPE_ALPHA	1
#define TEX_TYPE_ALPHA_SVENGINE	1

#define TEX_TYPE_LUM	2//removed by SvEngine

#define TEX_TYPE_ALPHA_GRADIENT 3
#define TEX_TYPE_RGBA	4

#define TEX_TYPE_ALPHA_GRADIENT_SVENGINE 2
#define TEX_TYPE_RGBA_SVENGINE	3
