#pragma once

#include <APIProxy.h>
#include <r_studioint.h>

extern cl_enginefunc_t gEngfuncs;
extern engine_studio_api_t IEngineStudio;

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s** ppinterface, struct engine_studio_api_s* pstudio);