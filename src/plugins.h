#pragma once

#include <metahook.h>

static_assert(METAHOOK_API_VERSION >= 115, "BetterSpray requires MetaHook API 115 and a merged gamedata catalog");

class IFileSystem;
extern IFileSystem* g_pFileSystem;
extern IFileSystem_HL25* g_pFileSystem_HL25;

extern mh_dll_info_t g_EngineDLLInfo;
extern mh_dll_info_t g_MirrorEngineDLLInfo;
extern mh_dll_info_t g_ClientDLLInfo;
extern mh_dll_info_t g_MirrorClientDLLInfo;
extern int g_iEngineType;
extern DWORD g_dwEngineBuildnum;

#define MHPluginName "BetterSpray"
#define Sys_Error(msg, ...) g_pMetaHookAPI->SysError("["  MHPluginName   "] " msg, __VA_ARGS__);

//Required symbols are resolved exclusively through the host gamedata catalog.
inline PVOID GamedataResolvePtr(PVOID moduleBase, const char* moduleName, const char* symbolName, mh_gamesymbol_kind_t kind)
{
	PVOID address = nullptr;
	auto status = g_pMetaHookAPI->ResolveGameSymbol(moduleBase, symbolName, kind, &address);
	if (status != MH_GAMESYMBOL_OK || !address)
	{
		const char* reason = status == MH_GAMESYMBOL_OK ? "null address" : g_pMetaHookAPI->GetGameSymbolStatusString(status);
		Sys_Error("Could not resolve gamedata symbol: %s (module %s, %s)\nEngine buildnum: %d",
			symbolName, moduleName, reason, g_dwEngineBuildnum);
		return nullptr;
	}
	return address;
}

#define Install_InlineHook(fn) if(!g_phook_##fn) { g_phook_##fn = g_pMetaHookAPI->InlineHook((void *)gPrivateFuncs.fn, fn, (void **)&gPrivateFuncs.fn); }
#define Uninstall_Hook(fn) if(g_phook_##fn){g_pMetaHookAPI->UnHook(g_phook_##fn);g_phook_##fn = NULL;}
