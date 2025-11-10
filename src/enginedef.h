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

typedef struct WAD3Header_s {
    char signature[4];
    unsigned int lumpsCount;
    unsigned int lumpsOffset;
} WAD3Header_t;

typedef struct WAD3Lump_s {
    int offset;
    int sizeOnDisk;
    int size;
    char type;
    bool compression;
    short dummy;
    char name[16];
} WAD3Lump_t;

typedef struct BSPMipTexHeader_s {
    char name[16];
    unsigned int width;
    unsigned int height;
    unsigned int offsets[4];
} BSPMipTexHeader_t;

typedef struct cachewad_s
{
	char* name;
	void* cache;
	int cacheCount;
	int cacheMax;
	void* lumps;
	int lumpCount;
	int cacheExtra;
	void* pfnCacheBuild;
	int numpaths;
	char** basedirs;
	int* lumppathindices;
	qboolean tempWad;
} cachewad_t;