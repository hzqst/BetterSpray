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

//typedef int (*fnDraw_LoadSprayTextureCallback)(int playerindex, const char* userId, FIBITMAP* fiB);

using fnDraw_LoadSprayTextureCallback = std::function<int(const char* userId, FIBITMAP* fiB)>;

bool EngineIsInLevel();
int EngineFindPlayerIndexByUserId(const char* userId);
bool Draw_HasCustomDecal(int playerindex);
int Draw_LoadSprayTexture_AsyncLoadInGame(int playerindex, FIBITMAP* fiB);
int Draw_LoadSprayTexture(const char* userId, const char* filePath, const char* pathId, const fnDraw_LoadSprayTextureCallback& callback);
int Draw_LoadSprayTextures(const char* userId, const char* pathId, const fnDraw_LoadSprayTextureCallback &callback);
void Draw_LoadSprayTexture_BGRA8ToRGBA8(FIBITMAP* fiB);
int Draw_LoadSprayTexture_ConvertToBGRA32(FIBITMAP** pfiB);

bool BS_UploadSprayBitmap(FIBITMAP* fiB, bool bNormalizeToSquare, bool bWithAlphaChannel, bool bAlphaInverted, bool bRandomBackground);