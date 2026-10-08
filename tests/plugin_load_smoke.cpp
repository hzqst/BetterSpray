#include <metahook.h>
#include <ISprayDatabase.h>

#include <cassert>
#include <cstdio>
#include <cstring>

static int versionErrors = 0;

static void ReportVersionError(const char*, ...)
{
    ++versionErrors;
}

int wmain(int argc, wchar_t** argv)
{
    assert(5 == argc);
    HMODULE dependencies[3] = {};
    for (int index = 0; index < 3; ++index)
    {
        dependencies[index] = LoadLibraryW(argv[index + 2]);
        if (!dependencies[index])
        {
            fwprintf(stderr, L"Could not load %ls (Windows error %lu)\n", argv[index + 2], GetLastError());
            return 1;
        }
    }
    HMODULE plugin = LoadLibraryW(argv[1]);
    if (!plugin)
    {
        fwprintf(stderr, L"Could not load %ls (Windows error %lu)\n", argv[1], GetLastError());
        return 1;
    }
    auto factory = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(plugin, "CreateInterface"));
    assert(nullptr != factory);
    int  status    = IFACE_FAILED;
    auto lifecycle = static_cast<IPluginsV4*>(factory(METAHOOK_PLUGIN_API_VERSION_V4, &status));
    assert(IFACE_OK == status);
    assert(nullptr != lifecycle);
    const char* version = lifecycle->GetVersion();
    assert(nullptr != version);
    assert(0u < strlen(version));
    auto database = static_cast<ISprayDatabase*>(factory(SPARY_DATABASE_INTERFACE_VERSION, &status));
    assert(IFACE_OK == status);
    assert(nullptr != database);
    assert(nullptr == factory("UnknownInterface", &status));
    assert(IFACE_FAILED == status);

    //An older host must be rejected before any new API function is accessed.
    metahook_api_t api      = {};
    api.SysError            = ReportVersionError;
    mh_interface_t host     = {};
    host.MetaHookAPIVersion = METAHOOK_API_VERSION - 1;
    mh_enginesave_t save    = {};
    cl_enginefunc_t engine  = {};
    lifecycle->Init(&api, &host, &save);
    lifecycle->LoadEngine(&engine);
    assert(1 == versionErrors);
    FreeLibrary(plugin);
    for (int index = 2; index >= 0; --index)
        FreeLibrary(dependencies[index]);
    return 0;
}
