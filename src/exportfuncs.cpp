#include <metahook.h>
#include <cmath>
#include <format>
#include "cvardef.h"
#include "plugins.h"
#include "privatehook.h"
#include "SparyDatabase.h"
#include "UtilHTTPClient.h"

cl_enginefunc_t gEngfuncs = {0};
engine_studio_api_t IEngineStudio = { 0 };

texture_t* Draw_DecalTexture(int index)
{
	customization_t* pCust = NULL;
	texture_t* retval = gPrivateFuncs.Draw_DecalTexture(index);

	if (index < 0)
	{
		int playerindex = ~index;

		if (playerindex >= 0 && playerindex < MAX_CLIENTS)
		{
			auto playerInfo = (player_info_sc_t*)IEngineStudio.PlayerInfo(playerindex);

			if (g_iEngineType == ENGINE_SVENGINE)
			{
				if (playerInfo && playerInfo->m_nSteamID != 0)
				{
					auto steamId = std::format("{0}", playerInfo->m_nSteamID);

					auto textureId = SparyDatabase()->FindSparyTextureId(playerindex, steamId.c_str());

					if (textureId)
					{
						retval->gl_texturenum = textureId;
					}
					else
					{
						SparyDatabase()->QueryPlayerSpary(playerindex, steamId.c_str());
					}
				}
			}
		}
	}

	return retval;
}

void HUD_Frame(double frame)
{
	gExportfuncs.HUD_Frame(frame);

	SparyDatabase()->RunFrame();
	UtilHTTPClient()->RunFrame();
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	//gEngfuncs.pfnAddCommand("scmodel_reload", SCModel_Reload_f);

	SparyDatabase()->Init();
}

void HUD_Shutdown(void)
{
	SparyDatabase()->Shutdown();

	gExportfuncs.HUD_Shutdown();

	UtilHTTPClient_Shutdown();
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s** ppinterface, struct engine_studio_api_s* pstudio)
{
	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));

	auto ret = gExportfuncs.HUD_GetStudioModelInterface ? gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio) : 1;

	return ret;
}