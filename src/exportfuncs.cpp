#include <metahook.h>
#include <cmath>
#include <format>
#include "cvardef.h"
#include "plugins.h"
#include "privatehook.h"
#include "SparyDatabase.h"
#include "UtilHTTPClient.h"

#include <FreeImage.h>

#include <ScopeExit/ScopeExit.h>

cl_enginefunc_t gEngfuncs = {0};
engine_studio_api_t IEngineStudio = { 0 };

unsigned WINAPI FI_Read(void* buffer, unsigned size, unsigned count, fi_handle handle)
{
	if (FILESYSTEM_ANY_READ(buffer, size * count, handle))
		return count;
	return 0;
}

unsigned WINAPI FI_Write(void* buffer, unsigned size, unsigned count, fi_handle handle)
{
	if (FILESYSTEM_ANY_WRITE(buffer, size * count, handle))
		return count;
	return 0;
}

int WINAPI FI_Seek(fi_handle handle, long offset, int origin)
{
	FILESYSTEM_ANY_SEEK(handle, offset, (FileSystemSeek_t)origin);
	return 0;
}

long WINAPI FI_Tell(fi_handle handle)
{
	return FILESYSTEM_ANY_TELL(handle);
}

static int FindPlayerIndexByUserId(const char* userId)
{
	for (int i = 0; i < gEngfuncs.GetMaxClients(); ++i)
	{
		auto playerInfo = (player_info_sc_t*)IEngineStudio.PlayerInfo(i);

		if (g_iEngineType == ENGINE_SVENGINE)
		{
			if (playerInfo && playerInfo->m_nSteamID != 0)
			{
				auto steamId = std::format("{0}", playerInfo->m_nSteamID);

				if (!strcmp(userId, steamId.c_str())) {
					return i;
				}
			}
		}
	}

	return -1;
}

int Draw_UploadSparyTextureBGRA8(int playerindex, FIBITMAP* fiB)
{
	size_t pos = 0;
	size_t w = FreeImage_GetWidth(fiB);
	size_t h = FreeImage_GetHeight(fiB);

	byte* imageData = FreeImage_GetBits(fiB);

	FreeImage_FlipVertical(fiB);

	for (unsigned y = 0; y < h; ++y) {
		// Get a pointer to the start of the pixel row.
		BYTE* bits = FreeImage_GetScanLine(fiB, y);
		for (unsigned x = 0; x < w; ++x) {
			// Swap the red and blue bytes.
			// Assuming the format is BGRA, where the bytes are in the order (B, G, R, A).
			BYTE temp = bits[FI_RGBA_RED];
			bits[FI_RGBA_RED] = bits[FI_RGBA_BLUE];
			bits[FI_RGBA_BLUE] = temp;

			// Move to the next pixel (4 bytes per pixel).
			bits += 4;
		}
	}
	texture_t* tex = gPrivateFuncs.Draw_DecalTexture(~playerindex);

	if (tex)
	{
		//Don't change width and height or otherwise it sucks
		//tex->width = w;
		//tex->height = h;

#ifndef GL_LINEAR_MIPMAP_LINEAR
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#endif
		tex->name[0] = '?';

		tex->gl_texturenum = gPrivateFuncs.GL_LoadTexture2(tex->name, GLT_DECAL, w, h, imageData, true, (g_iEngineType == ENGINE_SVENGINE) ? TEX_TYPE_RGBA_SVENGINE : TEX_TYPE_RGBA, nullptr, GL_LINEAR_MIPMAP_LINEAR);

		return 0;
	}

	return -6;
}

int Draw_UploadSparyTexture(int playerindex, const char* userId, const char* fileName, const char* pathId)
{
	if (playerindex < 0)
	{
		playerindex = FindPlayerIndexByUserId(userId);
	}

	if (playerindex >= 0 && playerindex < gEngfuncs.GetMaxClients())
	{
		std::string filePath = std::format("{0}/{1}", CUSTOM_SPRAY_DIRECTORY, fileName);

		auto fileHandle = FILESYSTEM_ANY_OPEN(filePath.c_str(), "rb", pathId);

		if (!fileHandle)
		{
			gEngfuncs.Con_DPrintf("Draw_UploadSparyTexture: Could not open \"%s\" for reading.\n", filePath.c_str());
			return -1;
		}

		SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(fileHandle); };

		FreeImageIO fiIO;
		fiIO.read_proc = FI_Read;
		fiIO.write_proc = FI_Write;
		fiIO.seek_proc = FI_Seek;
		fiIO.tell_proc = FI_Tell;

		auto fiFormat = FIF_UNKNOWN;

		//if (fiFormat == FIF_UNKNOWN)
		//{
		//	fiFormat = FreeImage_GetFIFFromFilename(fileName);
		//}

		if (fiFormat == FIF_UNKNOWN)
		{
			fiFormat = FreeImage_GetFileTypeFromHandle(&fiIO, (fi_handle)fileHandle);
		}

		if (fiFormat == FIF_UNKNOWN)
		{
			gEngfuncs.Con_Printf("Draw_UploadSparyTexture: Could not load \"%s\", Unknown format.\n", filePath.c_str());
			return -2;
		}

		if (!FreeImage_FIFSupportsReading(fiFormat))
		{
			gEngfuncs.Con_Printf("Draw_UploadSparyTexture: Could not load \"%s\", Unsupported format.\n", filePath.c_str());
			return -3;
		}

		auto fiB = FreeImage_LoadFromHandle(fiFormat, &fiIO, (fi_handle)fileHandle, 0);

		if (!fiB)
		{
			gEngfuncs.Con_Printf("Draw_UploadSparyTexture: Could not load \"%s\", FreeImage_LoadFromHandle failed.\n", filePath.c_str());
			return -4;
		}

		SCOPE_EXIT{ FreeImage_Unload(fiB); };

		auto fiBGRA8 = FreeImage_ConvertTo32Bits(fiB);

		if (!fiBGRA8)
		{
			gEngfuncs.Con_Printf("Draw_UploadSparyTexture: Could not load \"%s\", FreeImage_ConvertTo32Bits failed.\n", filePath.c_str());
			return -5;
		}

		SCOPE_EXIT{ FreeImage_Unload(fiBGRA8); };

		return Draw_UploadSparyTextureBGRA8(playerindex, fiBGRA8);
	}
	else
	{
		gEngfuncs.Con_DPrintf("Draw_UploadSparyTexture: invalid player index.\n");
	}

	return -7;
}

texture_t* Draw_DecalTexture(int index)
{
	customization_t* pCust = NULL;
	texture_t* retval = gPrivateFuncs.Draw_DecalTexture(index);

	if (index < 0 && retval->name[0] != '?')//The decal texture we replaced starts with "?"
	{
		int playerindex = ~index;

		if (playerindex >= 0 && playerindex < gEngfuncs.GetMaxClients())
		{
			auto playerInfo = (player_info_sc_t*)IEngineStudio.PlayerInfo(playerindex);

			if (g_iEngineType == ENGINE_SVENGINE)
			{
				if (playerInfo && playerInfo->m_nSteamID != 0)
				{
					auto userId = std::format("{0}", playerInfo->m_nSteamID);

					auto queryStatus = SparyDatabase()->GetPlayerSparyQueryStatus(userId.c_str());

					if (queryStatus == SparyQueryState_Unknown)
					{
						auto fileName = std::format("{0}.jpg", userId);

						auto err = Draw_UploadSparyTexture(playerindex, userId.c_str(), fileName.c_str(), "GAMEDOWNLOAD");

						if (err == 0)
						{
							SparyDatabase()->UpdatePlayerSparyQueryStatus(userId.c_str(), SparyQueryState_Finished);
						}
						else if (err == -1)
						{
							//file not found ?
							SparyDatabase()->QueryPlayerSpary(playerindex, userId.c_str());
						}
						else
						{
							//could be invalid file or what
							SparyDatabase()->UpdatePlayerSparyQueryStatus(userId.c_str(), SparyQueryState_Failed);
						}
					}
					else if (queryStatus == SparyQueryState_Finished)
					{
						auto fileName = std::format("{0}.jpg", userId);

						Draw_UploadSparyTexture(playerindex, userId.c_str(), fileName.c_str(), "GAMEDOWNLOAD");
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