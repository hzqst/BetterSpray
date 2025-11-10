#pragma once

#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>

#include <FreeImage.h>

#include <functional>

extern cl_enginefunc_t gEngfuncs;
extern engine_studio_api_t IEngineStudio;

void HUD_Frame(double frame);
void HUD_Init(void);
int HUD_VidInit(void);
void HUD_Shutdown(void);
int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s** ppinterface, struct engine_studio_api_s* pstudio);

enum CONVERT_TO_BGRA_STATUS
{
	CONVERT_BGRA_FAILED = -1,
	CONVERT_BGRA_OK = 0,
	CONVERT_BGRA_NON_32BPP = 1,
	CONVERT_BGRA_HAS_ALPHA_REGION = 2,
	CONVERT_BGRA_HAS_BACKGROUND = 3,
};

enum LOADSPRAYTEXTURE_STATUS
{
	LOAD_SPARY_FAILED_NO_CUSTOM_DECAL = -10,
	LOAD_SPARY_FAILED_NOT_IN_LEVEL = -9,
	LOAD_SPARY_FAILED_COULD_NOT_LOAD = -4,
	LOAD_SPARY_FAILED_UNSUPPORTED_FORMAT = -3,
	LOAD_SPARY_FAILED_UNKNOWN_FORMAT = -2,
	LOAD_SPARY_FAILED_NOT_FOUND = -1,
	LOAD_SPARY_OK = 0,
};

using fnDraw_LoadSprayTextureCallback = std::function<LOADSPRAYTEXTURE_STATUS(const char* userId, const char* wadHash, FIBITMAP* fiB)>;

bool EngineIsInLevel();
int EngineFindPlayerIndexByUserId(const char* userId);
bool Draw_HasCustomDecal(int playerindex);
LOADSPRAYTEXTURE_STATUS Draw_LoadSprayTexture_AsyncLoadInGame(int playerindex, FIBITMAP* fiB);
LOADSPRAYTEXTURE_STATUS Draw_LoadSprayTexture(const char* userId, const char* wadHash, const char* filePath, const char* pathId OPTIONAL, const fnDraw_LoadSprayTextureCallback& callback);
LOADSPRAYTEXTURE_STATUS Draw_LoadSprayTextures(const char* userId, const char * wadHash, const char* pathId OPTIONAL, const fnDraw_LoadSprayTextureCallback &callback);
void Draw_LoadSprayTexture_BGRA8ToRGBA8(FIBITMAP* fiB);
CONVERT_TO_BGRA_STATUS Draw_LoadSprayTexture_ConvertToBGRA32(FIBITMAP** pfiB);

bool BS_UploadSprayBitmap(FIBITMAP* fiB, bool bNormalizeToSquare, bool bWithAlphaChannel, bool bAlphaInverted, bool bRandomBackground);