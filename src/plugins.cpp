#include <metahook.h>
#include "exportfuncs.h"
#include "privatehook.h"

cl_exportfuncs_t gExportfuncs = { 0 };
mh_interface_t* g_pInterface = NULL;
metahook_api_t* g_pMetaHookAPI = NULL;
mh_enginesave_t* g_pMetaSave = NULL;
IFileSystem* g_pFileSystem = NULL;
IFileSystem_HL25* g_pFileSystem_HL25 = NULL;

DWORD g_dwEngineBuildnum = 0;
int g_iEngineType = 0;

mh_dll_info_t g_EngineDLLInfo = { 0 };
mh_dll_info_t g_MirrorEngineDLLInfo = { 0 };
mh_dll_info_t g_ClientDLLInfo = { 0 };
mh_dll_info_t g_MirrorClientDLLInfo = { 0 };

void IPluginsV4::Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave)
{
	g_pInterface = pInterface;
	g_pMetaHookAPI = pAPI;
	g_pMetaSave = pSave;
}

void IPluginsV4::Shutdown(void)
{
}

void IPluginsV4::LoadEngine(cl_enginefunc_t* pEngfuncs)
{
	g_iEngineType = g_pMetaHookAPI->GetEngineType();
	g_dwEngineBuildnum = g_pMetaHookAPI->GetEngineBuildnum();
	g_EngineDLLInfo.ImageBase = g_pMetaHookAPI->GetEngineBase();
	g_EngineDLLInfo.ImageSize = g_pMetaHookAPI->GetEngineSize();
	g_EngineDLLInfo.TextBase = g_pMetaHookAPI->GetSectionByName(g_EngineDLLInfo.ImageBase, ".text\x0\x0\x0", &g_EngineDLLInfo.TextSize);
	g_EngineDLLInfo.DataBase = g_pMetaHookAPI->GetSectionByName(g_EngineDLLInfo.ImageBase, ".data\x0\x0\x0", &g_EngineDLLInfo.DataSize);
	g_EngineDLLInfo.RdataBase = g_pMetaHookAPI->GetSectionByName(g_EngineDLLInfo.ImageBase, ".rdata\x0\x0", &g_EngineDLLInfo.RdataSize);

	g_MirrorEngineDLLInfo.ImageBase = g_pMetaHookAPI->GetMirrorEngineBase();
	g_MirrorEngineDLLInfo.ImageSize = g_pMetaHookAPI->GetMirrorEngineSize();

	if (g_MirrorEngineDLLInfo.ImageBase)
	{
		g_MirrorEngineDLLInfo.TextBase = g_pMetaHookAPI->GetSectionByName(g_MirrorEngineDLLInfo.ImageBase, ".text\x0\x0\x0", &g_MirrorEngineDLLInfo.TextSize);
		g_MirrorEngineDLLInfo.DataBase = g_pMetaHookAPI->GetSectionByName(g_MirrorEngineDLLInfo.ImageBase, ".data\x0\x0\x0", &g_MirrorEngineDLLInfo.DataSize);
		g_MirrorEngineDLLInfo.RdataBase = g_pMetaHookAPI->GetSectionByName(g_MirrorEngineDLLInfo.ImageBase, ".rdata\x0\x0", &g_MirrorEngineDLLInfo.RdataSize);
	}

	memcpy(&gEngfuncs, pEngfuncs, sizeof(gEngfuncs));
}

void IPluginsV4::LoadClient(cl_exportfuncs_t* pExportFunc)
{
	memcpy(&gExportfuncs, pExportFunc, sizeof(gExportfuncs));

	g_ClientDLLInfo.ImageBase = g_pMetaHookAPI->GetClientBase();
	g_ClientDLLInfo.ImageSize = g_pMetaHookAPI->GetClientSize();
	g_ClientDLLInfo.TextBase = g_pMetaHookAPI->GetSectionByName(g_ClientDLLInfo.ImageBase, ".text\0\0\0", &g_ClientDLLInfo.TextSize);
	g_ClientDLLInfo.DataBase = g_pMetaHookAPI->GetSectionByName(g_ClientDLLInfo.ImageBase, ".data\0\0\0", &g_ClientDLLInfo.DataSize);
	g_ClientDLLInfo.RdataBase = g_pMetaHookAPI->GetSectionByName(g_ClientDLLInfo.ImageBase, ".rdata\0\0", &g_ClientDLLInfo.RdataSize);

	g_MirrorClientDLLInfo.ImageBase = g_pMetaHookAPI->GetMirrorClientBase();
	g_MirrorClientDLLInfo.ImageSize = g_pMetaHookAPI->GetMirrorClientSize();

	if (g_MirrorClientDLLInfo.ImageBase)
	{
		g_MirrorClientDLLInfo.TextBase = g_pMetaHookAPI->GetSectionByName(g_MirrorClientDLLInfo.ImageBase, ".text\0\0\0", &g_MirrorClientDLLInfo.TextSize);
		g_MirrorClientDLLInfo.DataBase = g_pMetaHookAPI->GetSectionByName(g_MirrorClientDLLInfo.ImageBase, ".data\0\0\0", &g_MirrorClientDLLInfo.DataSize);
		g_MirrorClientDLLInfo.RdataBase = g_pMetaHookAPI->GetSectionByName(g_MirrorClientDLLInfo.ImageBase, ".rdata\0\0", &g_MirrorClientDLLInfo.RdataSize);
	}

	pExportFunc->HUD_GetStudioModelInterface = HUD_GetStudioModelInterface;

	Client_FillAddress(g_MirrorClientDLLInfo.ImageBase ? g_MirrorClientDLLInfo : g_ClientDLLInfo, g_ClientDLLInfo);
	Client_InstallHooks();
}

void IPluginsV4::ExitGame(int iResult)
{
}

const char* IPluginsV4::GetVersion()
{
	return "1.0.0";
}


EXPOSE_SINGLE_INTERFACE(IPluginsV4, IPluginsV4, METAHOOK_PLUGIN_API_VERSION_V4);