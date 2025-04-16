#pragma once

#include <cdll_int.h>

extern cl_enginefunc_t gEngfuncs;
extern cl_exportfuncs_t gExportfuncs;

int Initialize(struct cl_enginefuncs_s *pEnginefuncs, int iVersion);
void HUD_Init(void);
int HUD_Redraw(float time, int intermission);
void Sys_ErrorEx(const char* fmt, ...);
