#pragma once

#include "enginedef.h"
#include <com_model.h>
#include <r_studioint.h>

typedef struct walk_context_s
{
	walk_context_s(void* a, size_t l, int d) : address(a), len(l), depth(d)
	{

	}
	void* address;
	size_t len;
	int depth;
}walk_context_t;

typedef struct
{
	texture_t* (*Draw_DecalTexture)(int index);
	//int(*GL_LoadTexture)(char* identifier, int textureType, int width, int height, byte* data, qboolean mipmap, int iPalTextureType, byte* pPal);
	int(*GL_LoadTexture2)(char* identifier, int textureType, int width, int height, byte* data, qboolean mipmap, int iPalTextureType, byte* pPal, int filter);
	//void (*GL_UnloadTexture)(const char* identifier);
}private_funcs_t;

extern private_funcs_t gPrivateFuncs;

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void Engine_InstallHooks();
void Engine_UninstallHooks();
void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void Client_InstallHooks();
void Client_UninstallHooks();

PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo);

texture_t* Draw_DecalTexture(int index);