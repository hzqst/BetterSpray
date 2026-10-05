#pragma once

#include <metahook.h>

/*
	Purpose: Whether this engine's player_info_t carries the Sven Co-op
	hashedcdkey/m_nSteamID extension used as the per-player identity source.

	Only SvEngine and GoldSrc_HL25 ship that extension. Legacy GoldSrc
	(hl-3248 ~ hl-8684) has the base player_info_t only, so identity-dependent
	paths must be disabled there instead of reading past the struct.
*/
inline bool EngineSupportsPlayerIdentity()
{
	return g_iEngineType == ENGINE_SVENGINE || g_iEngineType == ENGINE_GOLDSRC_HL25;
}
