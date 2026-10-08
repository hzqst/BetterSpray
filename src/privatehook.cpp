#include <metahook.h>
#include "plugins.h"
#include "privatehook.h"

private_funcs_t gPrivateFuncs = {0};

static hook_t* g_phook_Draw_DecalTexture = nullptr;

bool Engine_FillAddress(const mh_dll_info_t&, const mh_dll_info_t& RealDllInfo)
{
    //Keep an installed hook's original-call trampoline on repeated initialization.
    if (gPrivateFuncs.GL_LoadTexture2 && gPrivateFuncs.Draw_DecalTexture)
        return true;

    gPrivateFuncs = {0};

    //The host resolves RVAs against the real module, not the mirror scan image.
    auto texture = GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "GL_LoadTexture2", MH_GAMESYMBOL_KIND_FUNCTION);
    if (!texture)
        return false;

    auto decal = GamedataResolvePtr(RealDllInfo.ImageBase, "engine", "Draw_DecalTexture", MH_GAMESYMBOL_KIND_FUNCTION);
    if (!decal)
        return false;

    gPrivateFuncs.GL_LoadTexture2   = (decltype(gPrivateFuncs.GL_LoadTexture2))texture;
    gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))decal;
    return true;
}

void Engine_InstallHooks()
{
    if (gPrivateFuncs.GL_LoadTexture2 && gPrivateFuncs.Draw_DecalTexture)
    {
        Install_InlineHook(Draw_DecalTexture);
    }
}

void Engine_UninstallHooks()
{
    Uninstall_Hook(Draw_DecalTexture);
}

void Client_FillAddress(const mh_dll_info_t&, const mh_dll_info_t&)
{
}

void Client_InstallHooks()
{
}

void Client_UninstallHooks()
{
}
