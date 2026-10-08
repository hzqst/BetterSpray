#include <metahook.h>
#include "plugins.h"
#include "privatehook.h"
#include "engine_identity.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

metahook_api_t* g_pMetaHookAPI     = nullptr;
int             g_iEngineType      = ENGINE_SVENGINE;
DWORD           g_dwEngineBuildnum = 10257;

namespace
{
constexpr uintptr_t EngineBase    = 0x10000000;
constexpr uintptr_t MirrorBase    = 0x20000000;
constexpr uintptr_t TextureOffset = 0x400;
constexpr uintptr_t DecalOffset   = 0x800;
constexpr uintptr_t Trampoline    = 0x30000000;
struct Query
{

    PVOID                module;
    std::string          name;
    mh_gamesymbol_kind_t kind;
};
std::vector<Query>       queries;
std::vector<std::string> errors;
mh_gamesymbol_status_t   failure      = MH_GAMESYMBOL_OK;
bool                     nullAddress  = false;
bool                     errorReturns = false;
bool                     firstMissing = false;
int                      scans        = 0;
int                      hooks        = 0;

mh_gamesymbol_status_t Resolve(PVOID module, const char* name, mh_gamesymbol_kind_t kind, PVOID* output)
{
    queries.push_back({module, name, kind});
    assert(reinterpret_cast<PVOID>(EngineBase) == module);
    assert(MH_GAMESYMBOL_KIND_FUNCTION == kind);
    if (std::string(name) == "GL_LoadTexture2")
    {
        if (firstMissing)
        {
            *output = nullptr;
            return MH_GAMESYMBOL_SYMBOL_NOT_FOUND;
        }
        *output = reinterpret_cast<PVOID>(EngineBase + TextureOffset);
        return MH_GAMESYMBOL_OK;
    }
    assert(std::string("Draw_DecalTexture") == name);
    *output = nullAddress ? nullptr : reinterpret_cast<PVOID>(EngineBase + DecalOffset);
    return failure;
}

const char* StatusString(mh_gamesymbol_status_t)
{
    return "fixture status";
}

void ReportError(const char* format, ...)
{
    char    message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    errors.emplace_back(message);
    if (!errorReturns)
        throw std::runtime_error(message);
}

PVOID Scan(PVOID, DWORD, const char*, DWORD)
{
    ++scans;
    return nullptr;
}

hook_t* Install(PVOID original, PVOID, PVOID* originalCall)
{
    ++hooks;
    assert(reinterpret_cast<PVOID>(EngineBase + DecalOffset) == original);
    *originalCall = reinterpret_cast<PVOID>(Trampoline);
    return reinterpret_cast<hook_t*>(static_cast<uintptr_t>(1));
}

BOOL Uninstall(hook_t*)
{
    return TRUE;
}
} // namespace

texture_t* Draw_DecalTexture(int)
{
    return nullptr;
}

int main(int argc, char** argv)
{
    assert(2 == argc);

    // Per-player identity is only valid where player_info_t carries the Sven
    // Co-op extension; every other engine family must degrade gracefully.
    {
        const int engines[] = {ENGINE_UNKNOWN, ENGINE_GOLDSRC_BLOB, ENGINE_GOLDSRC,
                               ENGINE_SVENGINE, ENGINE_GOLDSRC_HL25, ENGINE_GOLDSRC_COF};
        for (int engine : engines)
        {
            g_iEngineType       = engine;
            const bool expected = (engine == ENGINE_SVENGINE || engine == ENGINE_GOLDSRC_HL25);
            assert(expected == EngineSupportsPlayerIdentity());
        }
        g_iEngineType = ENGINE_SVENGINE;
    }

    const std::string scenario = argv[1];
    if (scenario == "first_missing")
        firstMissing = true;
    else if (scenario == "missing" || scenario == "error_returns")
        failure = MH_GAMESYMBOL_SYMBOL_NOT_FOUND;
    else if (scenario == "kind_mismatch")
        failure = MH_GAMESYMBOL_KIND_MISMATCH;
    else if (scenario == "catalog_conflict")
        failure = MH_GAMESYMBOL_CATALOG_CONFLICT;
    else if (scenario == "null_address")
        nullAddress = true;
    else
        assert(std::string("success") == scenario);
    errorReturns = scenario == "error_returns";

    metahook_api_t api            = {};
    api.ResolveGameSymbol         = Resolve;
    api.GetGameSymbolStatusString = StatusString;
    api.SysError                  = ReportError;
    api.SearchPattern             = Scan;
    api.InlineHook                = Install;
    api.UnHook                    = Uninstall;
    g_pMetaHookAPI                = &api;
    mh_dll_info_t engine          = {};
    engine.ImageBase              = reinterpret_cast<PVOID>(EngineBase);
    engine.ImageSize              = 0x10000;
    mh_dll_info_t mirror          = engine;
    mirror.ImageBase              = reinterpret_cast<PVOID>(MirrorBase);

    try
    {
        Engine_FillAddress(mirror, engine);
        Engine_InstallHooks();
    }
    catch (const std::runtime_error& error)
    {
        if (scenario == "success")
        {
            fprintf(stderr, "Unexpected resolution error: %s\n", error.what());
            return 1;
        }
    }

    assert((firstMissing ? size_t(1) : size_t(2)) == queries.size());
    assert(std::string("GL_LoadTexture2") == queries[0].name);
    if (!firstMissing)
        assert(std::string("Draw_DecalTexture") == queries[1].name);
    assert(0 == scans);
    if (scenario == "success")
    {
        assert(errors.empty());
        assert(1 == hooks);
        assert(reinterpret_cast<PVOID>(EngineBase + TextureOffset) == reinterpret_cast<PVOID>(gPrivateFuncs.GL_LoadTexture2));
        assert(reinterpret_cast<PVOID>(Trampoline) == reinterpret_cast<PVOID>(gPrivateFuncs.Draw_DecalTexture));
        Engine_FillAddress(mirror, engine);
        Engine_InstallHooks();
        if (queries.size() != 2 || reinterpret_cast<PVOID>(gPrivateFuncs.Draw_DecalTexture) != reinterpret_cast<PVOID>(Trampoline) || hooks != 1)
        {
            fprintf(stderr, "Repeated initialization replaced the hook trampoline or repeated resolution.\n");
            return 1;
        }
        Engine_UninstallHooks();
    }
    else
    {
        assert(0 == hooks);
        assert(size_t(1) == errors.size());
        assert(std::string::npos != errors[0].find(firstMissing ? "GL_LoadTexture2" : "Draw_DecalTexture"));
        assert(std::string::npos != errors[0].find("engine"));
        assert(std::string::npos != errors[0].find("10257"));
        if (!nullAddress)
            assert(std::string::npos != errors[0].find("fixture status"));
        assert(nullptr == gPrivateFuncs.GL_LoadTexture2);
        assert(nullptr == gPrivateFuncs.Draw_DecalTexture);
    }
    return 0;
}
