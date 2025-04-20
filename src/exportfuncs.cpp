#include <metahook.h>
#include <cmath>
#include "cvardef.h"
#include "plugins.h"
#include "privatehook.h"

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
			auto playerInfo = (player_info_sc_t *)IEngineStudio.PlayerInfo(playerindex);

			if (g_iEngineType == ENGINE_SVENGINE)
			{
				if (playerInfo && playerInfo->userid != 0)
				{
					return nullptr;
				}
			}
			else
			{

			}
		}
	}

	return retval;
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s** ppinterface, struct engine_studio_api_s* pstudio)
{
	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));

	auto ret = gExportfuncs.HUD_GetStudioModelInterface ? gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio) : 1;

	return ret;
}