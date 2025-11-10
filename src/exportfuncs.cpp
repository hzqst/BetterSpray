#include <metahook.h>
#include <cmath>
#include <format>
#include <cvardef.h>

#include "plugins.h"
#include "privatehook.h"
#include "exportfuncs.h"

#include "SprayDatabase.h"
#include "UtilHTTPClient.h"
#include "UtilThreadTask.h"

#include <FreeImage.h>

#include <ScopeExit/ScopeExit.h>

#include <steam_api.h>

//Fuck microsoft
#undef min
#undef max
#include <md5.h>

cl_enginefunc_t gEngfuncs = { 0 };
engine_studio_api_t IEngineStudio = { 0 };

static bool IsPixelWhite(FIBITMAP* fib, int x, int y)
{
	RGBQUAD color{};
	if (FreeImage_GetPixelColor(fib, x, y, &color) && color.rgbRed > 220 && color.rgbGreen > 220 && color.rgbBlue > 220)
	{
		return true;
	}
	return false;
}

static bool IsPixelBack(FIBITMAP* fib, int x, int y)
{
	RGBQUAD color{};
	if (FreeImage_GetPixelColor(fib, x, y, &color) && color.rgbRed < 30 && color.rgbGreen < 30 && color.rgbBlue < 30)
	{
		return true;
	}

	return false;
}

static bool IsPixelRed(FIBITMAP* fib, int x, int y)
{
	RGBQUAD color{};
	if (FreeImage_GetPixelColor(fib, x, y, &color) && color.rgbRed > 128 && color.rgbGreen < 128 && color.rgbBlue < 128)
	{
		return true;
	}

	return false;
}

static bool IsPixelGreen(FIBITMAP* fib, int x, int y)
{
	RGBQUAD color{};
	if (FreeImage_GetPixelColor(fib, x, y, &color) && color.rgbRed < 128 && color.rgbGreen > 128 && color.rgbBlue < 128)
	{
		return true;
	}

	return false;
}

static bool IsPixelBlue(FIBITMAP* fib, int x, int y)
{
	RGBQUAD color{};
	if (FreeImage_GetPixelColor(fib, x, y, &color) && color.rgbRed < 128 && color.rgbGreen < 128 && color.rgbBlue > 128)
	{
		return true;
	}

	return false;
}

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

bool EngineIsInLevel()
{
	auto levelName = gEngfuncs.pfnGetLevelName();
	if (levelName && levelName[0])
		return true;

	return false;
}

/*
	Purpose: Check if playerindex has custom decal
*/
bool Draw_HasCustomDecal(int playerindex)
{
	if (playerindex >= 0 && playerindex < MAX_CLIENTS)
	{
		auto playerInfo = (player_info_sc_t*)IEngineStudio.PlayerInfo(playerindex);

		if (!playerInfo->customdata.pNext)
			return false;

		if (!playerInfo->customdata.pNext->bInUse)
			return false;

		if (!playerInfo->customdata.pNext->pInfo)
			return false;

		if (!playerInfo->customdata.pNext->pBuffer)
			return false;

		return true;
	}

	return false;
}

/*
	Purpose: Get info about player custom decal
*/
bool Draw_GetCustomDecalInfo(
	int playerindex,
	OUT uint64_t* pSteamID64,
	OUT cachewad_t** ppOutCachedWad,
	OUT void** ppBuffer,
	OUT int* pBufferSize,
	OUT void* pubMD5Hash)
{
	auto playerInfo = (player_info_sc_t*)IEngineStudio.PlayerInfo(playerindex);

	if (playerInfo && playerInfo->m_nSteamID != 0)
	{
		customization_t* pCust = playerInfo->customdata.pNext;
		if (pCust != NULL && pCust->pBuffer != NULL && pCust->pInfo != NULL)
		{
			if (pSteamID64)
			{
				(*pSteamID64) = playerInfo->m_nSteamID;
			}
			if (ppOutCachedWad)
			{
				(*ppOutCachedWad) = (cachewad_t*)pCust->pInfo;
			}
			if (ppBuffer)
			{
				(*ppBuffer) = pCust->pBuffer;
			}
			if (pBufferSize)
			{
				(*pBufferSize) = pCust->resource.nDownloadSize;
			}
			if (pubMD5Hash)
			{
				memcpy(pubMD5Hash, pCust->resource.rgucMD5_hash, 16);
			}
			return true;
		}
	}

	return false;
}

/*
	Purpose: Upload RGBA8 spray texture to OpenGL.
*/

void Draw_UploadSprayTextureRGBA8(int playerindex, FIBITMAP* fiB)
{
	if (EngineIsInLevel() && Draw_HasCustomDecal(playerindex))
	{
		texture_t* tex = gPrivateFuncs.Draw_DecalTexture(~playerindex);

		if (tex)
		{
			size_t pos = 0;
			size_t w = FreeImage_GetWidth(fiB);
			size_t h = FreeImage_GetHeight(fiB);

			byte* imageData = FreeImage_GetBits(fiB);

			//Don't change width and height or otherwise it sucks
			//tex->width = w;
			//tex->height = h;

#ifndef GL_LINEAR_MIPMAP_LINEAR
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#endif
			tex->name[0] = '?';

			tex->gl_texturenum = gPrivateFuncs.GL_LoadTexture2(tex->name, GLT_DECAL, w, h, imageData, true, (g_iEngineType == ENGINE_SVENGINE) ? TEX_TYPE_RGBA_SVENGINE : TEX_TYPE_RGBA, nullptr, GL_LINEAR_MIPMAP_LINEAR);
		}
	}
}

/*
	Purpose: Find playerindex (1~MAX_CLIENTS) by userid (SteamID64), return -1 if not found
*/
int EngineFindPlayerIndexByUserId(const char* userId)
{
	for (int playerindex = 0; playerindex <= gEngfuncs.GetMaxClients(); ++playerindex)
	{
		auto playerInfo = (player_info_sc_t*)IEngineStudio.PlayerInfo(playerindex);

		if (playerInfo && playerInfo->m_nSteamID != 0)
		{
			char userIdSteamID64[32]{};
			snprintf(userIdSteamID64, sizeof(userIdSteamID64), "%llu", playerInfo->m_nSteamID);

			if (!strcmp(userId, userIdSteamID64)) {
				return playerindex;
			}
		}
	}

	return -1;
}

/*
	Purpose: Find playerindex (1~MAX_CLIENTS) by SteamID64, return -1 if not found
*/
int EngineFindPlayerIndexBySteamID64(uint64_t nSteamID64)
{
	for (int playerindex = 0; playerindex <= gEngfuncs.GetMaxClients(); ++playerindex)
	{
		auto playerInfo = (player_info_sc_t*)IEngineStudio.PlayerInfo(playerindex);

		if (playerInfo && playerInfo->m_nSteamID == nSteamID64)
		{
			return playerindex;
		}
	}

	return -1;
}

class CLoadSprayTextureWorkItemContext : public IThreadedTask
{
public:
	CLoadSprayTextureWorkItemContext(int playerindex, FIBITMAP* fiB) :
		m_playerindex(playerindex),
		m_fiB(fiB)
	{
	}

	~CLoadSprayTextureWorkItemContext()
	{
		if (m_fiB) {
			FreeImage_Unload(m_fiB);
			m_fiB = nullptr;
		}
	}

	void Destroy() override
	{
		delete this;
	}

	bool ShouldRun(float time) override
	{
		return true;
	}

	void Run(float time) override
	{
		Draw_UploadSprayTextureRGBA8(m_playerindex, m_fiB);
	}

public:
	int m_playerindex{};
	FIBITMAP* m_fiB{};
};

void Draw_LoadSprayTexture_BGRA8ToRGBA8(FIBITMAP* fiB)
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
}

bool Draw_LoadSprayTexture_DecodeBackground(FIBITMAP* fiB, int* iBackgroundType, bool* bWithAlphaChannel)
{
	unsigned w = FreeImage_GetWidth(fiB);
	unsigned h = FreeImage_GetHeight(fiB);

	if (IsPixelRed(fiB, 0, 0) && IsPixelRed(fiB, 0, h - 1) && IsPixelRed(fiB, w - 1, h - 1) && IsPixelRed(fiB, w - 1, 0))//All R
	{
		(*iBackgroundType) = 0;
		(*bWithAlphaChannel) = false;
		return true;
	}
	if (IsPixelGreen(fiB, 0, 0) && IsPixelGreen(fiB, 0, h - 1) && IsPixelGreen(fiB, w - 1, h - 1) && IsPixelGreen(fiB, w - 1, 0))//All G
	{
		(*iBackgroundType) = 1;
		(*bWithAlphaChannel) = false;
		return true;
	}
	if (IsPixelBlue(fiB, 0, 0) && IsPixelBlue(fiB, 0, h - 1) && IsPixelBlue(fiB, w - 1, h - 1) && IsPixelBlue(fiB, w - 1, 0))//All B
	{
		(*iBackgroundType) = 2;
		(*bWithAlphaChannel) = false;
		return true;
	}
	if (IsPixelRed(fiB, 0, 0) && IsPixelRed(fiB, 0, h - 1) && IsPixelGreen(fiB, w - 1, h - 1) && IsPixelGreen(fiB, w - 1, 0))//leftR rightG
	{
		(*iBackgroundType) = 0;
		(*bWithAlphaChannel) = true;
		return true;
	}
	if (IsPixelGreen(fiB, 0, 0) && IsPixelGreen(fiB, 0, h - 1) && IsPixelBlue(fiB, w - 1, h - 1) && IsPixelBlue(fiB, w - 1, 0))//leftG rightB
	{
		(*iBackgroundType) = 1;
		(*bWithAlphaChannel) = true;
		return true;
	}
	if (IsPixelRed(fiB, 0, 0) && IsPixelRed(fiB, 0, h - 1) && IsPixelBlue(fiB, w - 1, h - 1) && IsPixelBlue(fiB, w - 1, 0))//leftR rightB
	{
		(*iBackgroundType) = 2;
		(*bWithAlphaChannel) = true;
		return true;
	}

	return false;
}

CONVERT_TO_BGRA_STATUS
Draw_LoadSprayTexture_ConvertToBGRA32(FIBITMAP** pfiB)
{
	FIBITMAP* fiB = (*pfiB);

	unsigned width = FreeImage_GetWidth(fiB);
	unsigned height = FreeImage_GetHeight(fiB);

	if (FreeImage_GetBPP(fiB) == 24 &&
		(width == 5160 && height == 2160) ||
		(width == 3440 && height == 1440) ||
		(width == 2560 && height == 1080))
	{
		int newWidth = 1024;
		int newHeight = 1024;
		int iBackgroundType = 0;
		bool bWithAlphaChannel = false;

		if (height == 2160)
			newWidth = newHeight = 1024;
		if (height == 1440)
			newWidth = newHeight = 768;
		if (height == 1080)
			newWidth = newHeight = 512;

		if (Draw_LoadSprayTexture_DecodeBackground(fiB, &iBackgroundType, &bWithAlphaChannel))
		{
			FIBITMAP* newBitmap = nullptr;

			if (iBackgroundType == 0 && !bWithAlphaChannel)
			{
				newBitmap = FreeImage_Allocate(newWidth, newHeight, 24);

				// 从 fiB 最中间的位置提取 newWidth x newHeight 大小图片写入 newBitmap
				int srcX = (width - newWidth) / 2;
				int srcY = (height - newHeight) / 2;

				for (unsigned y = 0; y < newHeight; y++)
				{
					BYTE* srcBits = FreeImage_GetScanLine(fiB, (height - 1) - (srcY + y)) + srcX * 3;
					BYTE* dstBits = FreeImage_GetScanLine(newBitmap, (newHeight - y) - 1);

					memcpy(dstBits, srcBits, newWidth * 3);
				}
			}
			else if (iBackgroundType == 0 && bWithAlphaChannel)
			{
				newWidth *= 2;

				newBitmap = FreeImage_Allocate(newWidth, newHeight, 24);

				// 从 fiB 最中间的位置提取 newWidth x newHeight 大小图片写入 newBitmap
				int srcX = (width - newWidth) / 2;
				int srcY = (height - newHeight) / 2;

				for (unsigned y = 0; y < newHeight; y++)
				{
					BYTE* srcBits = FreeImage_GetScanLine(fiB, (height - 1) - (srcY + y)) + srcX * 3;
					BYTE* dstBits = FreeImage_GetScanLine(newBitmap, (newHeight - y) - 1);

					memcpy(dstBits, srcBits, newWidth * 3);
				}
			}
			else if (iBackgroundType == 1 && !bWithAlphaChannel)
			{
				newBitmap = FreeImage_Allocate(newWidth, newHeight, 24);

				// 从 fiB 最左上角的位置提取 newWidth x newHeight 大小图片写入 newBitmap
				for (unsigned y = 0; y < newHeight; y++)
				{
					BYTE* srcBits = FreeImage_GetScanLine(fiB, (height - 1) - y);
					BYTE* dstBits = FreeImage_GetScanLine(newBitmap, (newHeight - y) - 1);

					memcpy(dstBits, srcBits, newWidth * 3);
				}
			}
			else if (iBackgroundType == 1 && bWithAlphaChannel)
			{
				newWidth *= 2;

				newBitmap = FreeImage_Allocate(newWidth, newHeight, 24);

				// 从 fiB 最左上角的位置提取 newWidth x newHeight 大小图片写入 newBitmap
				for (unsigned y = 0; y < newHeight; y++)
				{
					BYTE* srcBits = FreeImage_GetScanLine(fiB, (height - 1) - y);
					BYTE* dstBits = FreeImage_GetScanLine(newBitmap, (newHeight - y) - 1);

					memcpy(dstBits, srcBits, newWidth * 3);
				}
			}
			else if (iBackgroundType == 2 && !bWithAlphaChannel)
			{
				newBitmap = FreeImage_Allocate(newWidth, newHeight, 24);

				// 从 fiB 最右上角的位置提取 newWidth x newHeight 大小图片写入 newBitmap
				int srcX = width - newWidth;

				for (unsigned y = 0; y < newHeight; y++)
				{
					BYTE* srcBits = FreeImage_GetScanLine(fiB, (height - 1) - y) + srcX * 3;
					BYTE* dstBits = FreeImage_GetScanLine(newBitmap, (newHeight - y) - 1);

					memcpy(dstBits, srcBits, newWidth * 3);
				}
			}
			else if (iBackgroundType == 2 && bWithAlphaChannel)
			{
				newWidth *= 2;

				newBitmap = FreeImage_Allocate(newWidth, newHeight, 24);

				// 从 fiB 最右上角的位置提取 newWidth x newHeight 大小图片写入 newBitmap
				int srcX = width - newWidth;

				for (unsigned y = 0; y < newHeight; y++)
				{
					BYTE* srcBits = FreeImage_GetScanLine(fiB, (height - 1) - y) + srcX * 3;
					BYTE* dstBits = FreeImage_GetScanLine(newBitmap, y);

					memcpy(dstBits, srcBits, newWidth * 3);
				}
			}

			(*pfiB) = newBitmap;

			FreeImage_Unload(fiB);

			return CONVERT_BGRA_HAS_BACKGROUND;
		}

		return CONVERT_BGRA_FAILED;
	}

	if (FreeImage_GetBPP(fiB) == 24 && width == height * 2)
	{
		auto newBitmap = FreeImage_Allocate(width / 2, height, 32);

		if (newBitmap)
		{
			// 检查最右下角像素是否为纯白色
			bool bInvertedAlpha = false;

			if (IsPixelWhite(fiB, width - 1, height - 1) ||
				IsPixelWhite(fiB, width - 2, height - 2) ||
				IsPixelWhite(fiB, width - 3, height - 3) ||
				IsPixelWhite(fiB, width - 4, height - 4))
			{
				bInvertedAlpha = true;
			}

			// 处理每一行像素
			for (unsigned y = 0; y < height; y++)
			{
				BYTE* srcBits = FreeImage_GetScanLine(fiB, y);
				BYTE* dstBits = FreeImage_GetScanLine(newBitmap, y);

				// 处理每一列像素
				for (unsigned x = 0; x < width / 2; x++)
				{
					// 左半部分作为RGB通道
					dstBits[FI_RGBA_BLUE] = srcBits[FI_RGBA_BLUE];
					dstBits[FI_RGBA_GREEN] = srcBits[FI_RGBA_GREEN];
					dstBits[FI_RGBA_RED] = srcBits[FI_RGBA_RED];

					// 右半部分计算Alpha值
					BYTE* alphaSrc = srcBits + (width / 2) * 3; // 指向右半部分对应位置
					int alphaValue = (alphaSrc[FI_RGBA_RED] + alphaSrc[FI_RGBA_GREEN] + alphaSrc[FI_RGBA_BLUE]) / 3;

					// 根据bInvertedAlpha决定是否反转alpha值
					if (bInvertedAlpha)
					{
						dstBits[FI_RGBA_ALPHA] = 255 - (BYTE)alphaValue;
					}
					else
					{
						dstBits[FI_RGBA_ALPHA] = (BYTE)alphaValue;
					}

					// 移动到下一个像素
					srcBits += 3;
					dstBits += 4;
				}
			}

			(*pfiB) = newBitmap;

			FreeImage_Unload(fiB);

			return CONVERT_BGRA_HAS_ALPHA_REGION;
		}
	}

	if (FreeImage_GetBPP(fiB) != 32)
	{
		(*pfiB) = FreeImage_ConvertTo32Bits(fiB);
		FreeImage_Unload(fiB);

		return CONVERT_BGRA_NON_32BPP;
	}

	if (width * height > 4096 * 4096)
		return CONVERT_BGRA_FAILED;

	return CONVERT_BGRA_OK;
}

void Draw_LoadSprayTexture_WorkItem(CLoadSprayTextureWorkItemContext* ctx)
{
	while (1)
	{
		auto result = Draw_LoadSprayTexture_ConvertToBGRA32(&ctx->m_fiB);

		if (result > CONVERT_BGRA_OK)
			continue;

		if (result == CONVERT_BGRA_OK)
			break;

		if (result < CONVERT_BGRA_OK)
		{
			ctx->Destroy();
			return;
		}
	}

	Draw_LoadSprayTexture_BGRA8ToRGBA8(ctx->m_fiB);

	GameThreadTaskScheduler()->QueueTask(ctx);
}

/*
	Purpose: Load spray texture from FileSystem. userId and filePath must be specified
*/

LOADSPRAYTEXTURE_STATUS Draw_LoadSprayTexture(const char* userId, const char* filePath, const char* pathId OPTIONAL, const fnDraw_LoadSprayTextureCallback& callback)
{
	auto fileHandle = FILESYSTEM_ANY_OPEN(filePath, "rb", pathId);

	if (!fileHandle)
	{
		gEngfuncs.Con_DPrintf("Draw_LoadSprayTexture: Could not open \"%s\" for reading.\n", filePath);
		return LOAD_SPARY_FAILED_NOT_FOUND;
	}

	SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(fileHandle); };

	FreeImageIO fiIO;
	fiIO.read_proc = FI_Read;
	fiIO.write_proc = FI_Write;
	fiIO.seek_proc = FI_Seek;
	fiIO.tell_proc = FI_Tell;

	auto fiFormat = FIF_UNKNOWN;

	if (fiFormat == FIF_UNKNOWN)
	{
		fiFormat = FreeImage_GetFileTypeFromHandle(&fiIO, (fi_handle)fileHandle);
	}

	if (fiFormat == FIF_UNKNOWN)
	{
		gEngfuncs.Con_Printf("Draw_LoadSprayTexture: Could not load \"%s\", Unknown format.\n", filePath);
		return LOAD_SPARY_FAILED_UNKNOWN_FORMAT;
	}

	if (!FreeImage_FIFSupportsReading(fiFormat))
	{
		gEngfuncs.Con_Printf("Draw_LoadSprayTexture: Could not load \"%s\", Unsupported format.\n", filePath);
		return LOAD_SPARY_FAILED_UNSUPPORTED_FORMAT;
	}

	auto fiB = FreeImage_LoadFromHandle(fiFormat, &fiIO, (fi_handle)fileHandle, 0);

	if (!fiB)
	{
		gEngfuncs.Con_Printf("Draw_LoadSprayTexture: Could not load \"%s\", FreeImage_LoadFromHandle failed.\n", filePath);
		return LOAD_SPARY_FAILED_COULD_NOT_LOAD;
	}

	return callback(userId, fiB);
}

/*
	Purpose: Load spray texture from FileSystem, try {SteamID}_{wadHash}.jpg and {SteamID}.jpg
*/

LOADSPRAYTEXTURE_STATUS Draw_LoadSprayTextures(const char* userId, const char* wadHash, const char* pathId OPTIONAL, const fnDraw_LoadSprayTextureCallback& callback)
{
	char filePath[256]{};
	snprintf(filePath, sizeof(filePath), "%s/%s_%s.jpg", CUSTOM_SPRAY_DIRECTORY, userId, wadHash);

	auto st = Draw_LoadSprayTexture(userId, filePath, pathId, callback);

	if (st == LOAD_SPARY_FAILED_NOT_FOUND)
	{
		//fallback to {SteamID}.jpg
		snprintf(filePath, sizeof(filePath), "%s/%s.jpg", CUSTOM_SPRAY_DIRECTORY, userId);
		st = Draw_LoadSprayTexture(userId, filePath, pathId, callback);
	}

	return st;
}

LOADSPRAYTEXTURE_STATUS Draw_LoadSprayTexture_AsyncLoadInGame(int playerindex, FIBITMAP* fiB)
{
	if (!EngineIsInLevel())
	{
		gEngfuncs.Con_Printf("[BetterSpray] Spray loading procedure interrupted, not in level.\n");
		return LOAD_SPARY_FAILED_NOT_IN_LEVEL;
	}

	if (!Draw_HasCustomDecal(playerindex))
	{
		gEngfuncs.Con_Printf("[BetterSpray] Player #%d has no custom decal.\n", playerindex);
		return LOAD_SPARY_FAILED_NO_CUSTOM_DECAL;
	}

	auto ctx = new CLoadSprayTextureWorkItemContext(playerindex, fiB);

	auto hWorkItemHandle = g_pMetaHookAPI->CreateWorkItem(g_pMetaHookAPI->GetGlobalThreadPool(), [](void* context) -> bool {

		auto ctx = (CLoadSprayTextureWorkItemContext*)context;

		Draw_LoadSprayTexture_WorkItem(ctx);

		return true;

		}, ctx);

	g_pMetaHookAPI->QueueWorkItem(g_pMetaHookAPI->GetGlobalThreadPool(), hWorkItemHandle);

	return LOAD_SPARY_OK;
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
			uint64_t nSteamID64{};
			unsigned char rgucMD5[16]{};

			if (Draw_GetCustomDecalInfo(playerindex, &nSteamID64, nullptr, nullptr, nullptr, rgucMD5))
			{
				char userId[32]{};
				snprintf(userId, sizeof(userId), "%llu", nSteamID64);

				auto queryStatus = SprayDatabase()->GetPlayerSprayQueryStatus(userId);

				if (queryStatus == SprayQueryState_Unknown)
				{
					char wadHash[64]{};
					snprintf(wadHash, sizeof(wadHash), "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
						rgucMD5[0],
						rgucMD5[1],
						rgucMD5[2],
						rgucMD5[3],
						rgucMD5[4],
						rgucMD5[5],
						rgucMD5[6],
						rgucMD5[7],
						rgucMD5[8],
						rgucMD5[9],
						rgucMD5[10],
						rgucMD5[11],
						rgucMD5[12],
						rgucMD5[13],
						rgucMD5[14],
						rgucMD5[15]);

					auto result = Draw_LoadSprayTextures(userId, wadHash, nullptr, [playerindex](const char* userId, FIBITMAP* fiB) -> LOADSPRAYTEXTURE_STATUS {
						return Draw_LoadSprayTexture_AsyncLoadInGame(playerindex, fiB);
						});

					if (result == LOAD_SPARY_OK)
					{
						retval->name[0] = '?';
					}

					if (result == LOAD_SPARY_OK)
					{
						SprayDatabase()->UpdatePlayerSprayQueryStatus(userId, SprayQueryState_Finished);
					}
					else if (result == LOAD_SPARY_FAILED_NOT_FOUND)
					{
						//File not found ?
						SprayDatabase()->QueryPlayerSpray(playerindex, userId);
					}
					else
					{
						//Could be invalid file or what, tonikaku no more retry.
						SprayDatabase()->UpdatePlayerSprayQueryStatus(userId, SprayQueryState_Failed);
					}
				}
				else if (queryStatus == SprayQueryState_Finished)
				{
					char wadHash[64]{};
					snprintf(wadHash, sizeof(wadHash), "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
						rgucMD5[0],
						rgucMD5[1],
						rgucMD5[2],
						rgucMD5[3],
						rgucMD5[4],
						rgucMD5[5],
						rgucMD5[6],
						rgucMD5[7],
						rgucMD5[8],
						rgucMD5[9],
						rgucMD5[10],
						rgucMD5[11],
						rgucMD5[12],
						rgucMD5[13],
						rgucMD5[14],
						rgucMD5[15]);

					auto result = Draw_LoadSprayTextures(userId, wadHash, nullptr, [playerindex](const char* userId, FIBITMAP* fiB) -> LOADSPRAYTEXTURE_STATUS {
						return Draw_LoadSprayTexture_AsyncLoadInGame(playerindex, fiB);
						});

					if (result == 0)
					{
						retval->name[0] = '?';
					}
				}
				else
				{
					//Querying? do nothing
				}
			}
		}
	}

	return retval;
}

void Draw_GetValidateSparySize(size_t& ow, size_t& oh) {
	float w = static_cast<float>(ow);
	float h = static_cast<float>(oh);
	if (w * h > 14336.0f) {
		if (w > h) {
			h = h / w * 256.0f;
			w = 256.0f;
		}
		else {
			w = w / h * 256.0f;
			h = 256.0f;
		}
		while (w * h > 14436.0f) {
			w *= 0.95f;
			h *= 0.95f;
		}
	}
	w = static_cast<float>(static_cast<size_t>(w));
	h = static_cast<float>(static_cast<size_t>(h));
	float gap = 16.0f;
	float dw = fmodf(w, gap);
	if (dw > gap / 2.0f)
		w = w + (gap - dw);
	else
		w = w - dw;
	float dh = fmodf(h, gap);
	if (dh > gap / 2)
		h = h + (gap - dh);
	else
		h = h - dh;
	ow = static_cast<size_t>(w);
	oh = static_cast<size_t>(h);
}

static int Draw_FindPaletteIndex(RGBQUAD* palette, RGBQUAD rgb)
{
	for (int i = 0; i < 256; i++) {
		RGBQUAD p = palette[i];
		if (p.rgbRed == rgb.rgbRed &&
			p.rgbGreen == rgb.rgbGreen &&
			p.rgbBlue == rgb.rgbBlue)
			return i;
	}
	return -1;
}

/*
	Purpose: write fiB to tempdecal.wad, and hash the WAD file content as MD5
*/
/*
  Purpose: write fiB to tempdecal.wad, and hash the WAD file content as MD5
*/
void BS_SaveBitmapToTempDecal(FIBITMAP* fiB, unsigned char* rgucMD5_WAD)
{
	// 获取原始图像尺寸
	size_t w = FreeImage_GetWidth(fiB);
	size_t h = FreeImage_GetHeight(fiB);
	size_t nw = w;
	size_t nh = h;

	// 验证并调整图像尺寸
	Draw_GetValidateSparySize(nw, nh);

	// 缩放图像到新尺寸
	FIBITMAP* nimg = FreeImage_Rescale(fiB, nw, nh);
	if (!nimg) {
		gEngfuncs.Con_Printf("BS_SaveToTempDecal: Failed to rescale image!");
		return;
	}

	SCOPE_EXIT{ FreeImage_Unload(nimg); };

	// 量化透明像素
	size_t bpp = FreeImage_GetBPP(nimg);
	size_t pitch = FreeImage_GetPitch(nimg);
	BYTE* bits = FreeImage_GetBits(nimg);
	size_t bitnum = bpp / 8;
	bits += pitch * (nh - 1);

	for (size_t y = 0; y < nh; y++) {
		BYTE* pixel = (BYTE*)bits;
		for (size_t x = 0; x < nw; x++) {
			if (bitnum == 4) { // 32bpp
				BYTE alpha = pixel[FI_RGBA_ALPHA];
				if (alpha < 125) {
					pixel[FI_RGBA_RED] = 0;
					pixel[FI_RGBA_GREEN] = 0;
					pixel[FI_RGBA_BLUE] = 255;
				}
				pixel[FI_RGBA_ALPHA] = 255;
			}
			pixel += bitnum;
		}
		bits -= pitch;
	}

	// 颜色量化到256色
	int numPalette = 256;
	FIBITMAP* qimg = FreeImage_ColorQuantizeEx(nimg, FIQ_WUQUANT, numPalette);

	if (!qimg)
	{
		gEngfuncs.Con_Printf("BS_SaveToTempDecal: Failed to quantize image!");
		return;
	}

	// 交换调色板
	RGBQUAD* palette = FreeImage_GetPalette(qimg);

	int bluindex = -1;
	for (size_t i = 0; i < 256; i++) {
		RGBQUAD p = palette[i];
		if (p.rgbRed == 0 && p.rgbGreen == 0 && p.rgbBlue == 255) {
			auto tem = palette[255];
			palette[255] = palette[i];
			palette[i] = tem;
			bluindex = (int)i;
			break;
		}
	}

	//leave 0,0,255 at palette[255] 
	if (bluindex == -1)
	{
		FreeImage_Unload(qimg);
		numPalette = 255;
		qimg = FreeImage_ColorQuantizeEx(nimg, FIQ_WUQUANT, numPalette);
	}

	auto hFileHandle = FILESYSTEM_ANY_OPEN("tempdecal.wad", "wb");

	if (hFileHandle) {

		Chocobo1::MD5 hasher;

		size_t size = nw * nh;

		// 辅助函数：计算填充字节数
		auto requiredPadding = [](int length, int padToMultipleOf) {
			int excess = length % padToMultipleOf;
			return excess == 0 ? 0 : padToMultipleOf - excess;
			};

		// 提前计算 lump 大小和偏移量
		int lumpsize = (int)(sizeof(BSPMipTexHeader_t) + size + (size / 4) + (size / 16) + (size / 64) + sizeof(short) + 256 * 3);
		int padding = requiredPadding(lumpsize, 4);
		lumpsize += padding;

		int lumpoffset = sizeof(WAD3Header_t) + lumpsize;

		// 写入WAD3头部
		const char magic[] = "WAD3";
		FILESYSTEM_ANY_WRITE(magic, 4, hFileHandle);
		hasher.addData(magic, 4);

		// 写入lump数量(1) 
		unsigned int headerbuf = 1;
		FILESYSTEM_ANY_WRITE(&headerbuf, 4, hFileHandle);
		hasher.addData(&headerbuf, 4);

		// 写入lump偏移量（提前计算好的值）
		FILESYSTEM_ANY_WRITE(&lumpoffset, 4, hFileHandle);
		hasher.addData(&lumpoffset, 4);

		// 写入mips头部
		BSPMipTexHeader_t header;
		memset(&header, 0, sizeof(header));  // 先将整个结构体清零
		header.name[0] = '{';
		header.name[1] = 'L';
		header.name[2] = 'O';
		header.name[3] = 'G';
		header.name[4] = 'O';
		header.name[5] = '\0';  // 确保字符串正确终止
		header.width = nw;
		header.height = nh;
		header.offsets[0] = sizeof(BSPMipTexHeader_t);
		header.offsets[1] = sizeof(BSPMipTexHeader_t) + size;
		header.offsets[2] = sizeof(BSPMipTexHeader_t) + size + (size / 4);
		header.offsets[3] = sizeof(BSPMipTexHeader_t) + size + (size / 4) + (size / 16);
		FILESYSTEM_ANY_WRITE(&header, sizeof(BSPMipTexHeader_t), hFileHandle);
		hasher.addData(&header, sizeof(BSPMipTexHeader_t));

		auto qbits = FreeImage_GetBits(qimg);

		BYTE* flipped = new BYTE[size];
		for (size_t i = 0; i < nh; i++) {
			memcpy(flipped + i * nw, qbits + (nh - i - 1) * nw, nw);
		}

		if (bluindex != -1) {
			for (size_t i = 0; i < size; i++) {
				if ((int)flipped[i] == bluindex)
					flipped[i] = 255;
				else if (flipped[i] == 255)
					flipped[i] = (BYTE)bluindex;
			}
		}

		// 写入 mips 数据
		auto write_mips = [&](int mips_level) {
			int lv = pow(2, mips_level);
			for (size_t i = 0; i < (nh / lv); i++) {
				for (size_t j = 0; j < (nw / lv); j++) {

					BYTE buf = flipped[i * lv * nw + j * lv];

					FILESYSTEM_ANY_WRITE(&buf, 1, hFileHandle);
					hasher.addData(&buf, 1);
				}
			}
			};

		for (int i = 0; i < 4; i++) {
			write_mips(i);
		}

		delete[] flipped;

		// 写入调色板
		short colorused = 256;
		FILESYSTEM_ANY_WRITE(&colorused, sizeof(short), hFileHandle);
		hasher.addData(&colorused, sizeof(short));

		for (int i = 0; i < numPalette; i++) {
			RGBQUAD p = palette[i];

			FILESYSTEM_ANY_WRITE(&p.rgbRed, 1, hFileHandle);
			FILESYSTEM_ANY_WRITE(&p.rgbGreen, 1, hFileHandle);
			FILESYSTEM_ANY_WRITE(&p.rgbBlue, 1, hFileHandle);

			hasher.addData(&p.rgbRed, 1);
			hasher.addData(&p.rgbGreen, 1);
			hasher.addData(&p.rgbBlue, 1);
		}

		if (numPalette == 255) {
			RGBQUAD p = { 255, 0, 0, 255 };

			FILESYSTEM_ANY_WRITE(&p.rgbRed, 1, hFileHandle);
			FILESYSTEM_ANY_WRITE(&p.rgbGreen, 1, hFileHandle);
			FILESYSTEM_ANY_WRITE(&p.rgbBlue, 1, hFileHandle);

			hasher.addData(&p.rgbRed, 1);
			hasher.addData(&p.rgbGreen, 1);
			hasher.addData(&p.rgbBlue, 1);
		}

		// 写入填充字节以对齐到4字节边界
		headerbuf = 0;
		for (int i = 0; i < padding; i++) {
			FILESYSTEM_ANY_WRITE(&headerbuf, 1, hFileHandle);
			hasher.addData(&headerbuf, 1);
		}

		// 写入lump信息
		WAD3Lump_t lump = {
		  sizeof(WAD3Header_t),  // filepos 修正为WAD3 header的大小
		  lumpsize,  // disksize
		  lumpsize,  // size
		  0x43, // type (miptex) 
		  0,    // compression
		  0,    // pad1
		  "{LOGO\0"  // name
		};
		FILESYSTEM_ANY_WRITE(&lump, sizeof(WAD3Lump_t), hFileHandle);
		hasher.addData(&lump, sizeof(WAD3Lump_t));

		FILESYSTEM_ANY_CLOSE(hFileHandle);

		//hash the WAD file content as MD5. 
		const auto& md5 = hasher.finalize();
		const auto& md5Array = md5.toArray();

		for (int i = 0; i < 16; ++i)
		{
			rgucMD5_WAD[i] = md5Array[i];
		}
	}
	else
	{
		gEngfuncs.Con_Printf("BS_SaveToTempDecal: Failed to open \"tempdecal.wad\" for writing!");
	}

	FreeImage_Unload(qimg);
}

bool Draw_ValidateImageFormatMemoryIO(const void* data, size_t dataSize, const char* identifier)
{
	FIMEMORY* fim = FreeImage_OpenMemory((BYTE*)data, dataSize);

	if (!fim)
	{
		gEngfuncs.Con_Printf("Draw_ValidateImageFormatMemoryIO: FreeImage_OpenMemory failed for \"%s\".\n", identifier);
		return false;
	}

	SCOPE_EXIT{ FreeImage_CloseMemory(fim); };

	FREE_IMAGE_FORMAT fiFormat = FIF_UNKNOWN;

	if (fiFormat == FIF_UNKNOWN)
	{
		fiFormat = FreeImage_GetFileTypeFromMemory(fim);
	}

	if (fiFormat == FIF_UNKNOWN)
	{
		gEngfuncs.Con_Printf("Draw_ValidateImageFormatMemoryIO: Could not load \"%s\", Unknown format.\n", identifier);
		return false;
	}

	return true;
}

FIBITMAP* BS_NormalizeToSquareRGBA32(FIBITMAP* fiB)
{
	// 获取原图像的宽度和高度
	unsigned int width = FreeImage_GetWidth(fiB);
	unsigned int height = FreeImage_GetHeight(fiB);

	// 计算新图像的尺寸（取宽高的最大值）
	unsigned int newSize = (width > height) ? width : height;

	// 创建一个新的方形图像 Always 32bpp
	FIBITMAP* newBitmap = FreeImage_Allocate(newSize, newSize, 32);
	if (!newBitmap)
	{
		return NULL;
	}

	// 获取左上角像素的颜色值
	RGBQUAD cornerColor{};
	FreeImage_GetPixelColor(fiB, 0, 0, &cornerColor);

	// 使用FreeImage_FillBackground填充整个新图像
	FreeImage_FillBackground(newBitmap, &cornerColor);

	// 计算原图像在新图像中的起始位置（居中）
	unsigned int offsetX = (newSize - width) / 2;
	unsigned int offsetY = (newSize - height) / 2;

	RGBQUAD color{};

	// 遍历原图像的每个像素
	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			// 获取原图像像素的Alpha值
			FreeImage_GetPixelColor(fiB, x, y, &color);

			// 写入到新图像中（考虑偏移量）
			FreeImage_SetPixelColor(newBitmap, x + offsetX, y + offsetY, &color);
		}
	}

	return newBitmap;
}

FIBITMAP* BS_NormalizeToSquareRGB24(FIBITMAP* fiB, bool bInvertedAlpha)
{
	// 获取原图像的宽度和高度
	unsigned int width = FreeImage_GetWidth(fiB);
	unsigned int height = FreeImage_GetHeight(fiB);

	// 计算新图像的尺寸（取宽高的最大值）
	unsigned int newSize = (width > height) ? width : height;

	// 创建一个新的方形图像 Always 24bpp
	FIBITMAP* newBitmap = FreeImage_Allocate(newSize, newSize, 24);
	if (!newBitmap)
	{
		return NULL;
	}

	// 获取左上角像素的颜色值
	RGBQUAD cornerColor{};
	FreeImage_GetPixelColor(fiB, 0, 0, &cornerColor);

	// 使用FreeImage_FillBackground填充整个新图像
	FreeImage_FillBackground(newBitmap, &cornerColor);

	// 计算原图像在新图像中的起始位置（居中）
	unsigned int offsetX = (newSize - width) / 2;
	unsigned int offsetY = (newSize - height) / 2;

	RGBQUAD color;

	// 遍历原图像的每个像素
	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			// 获取原图像像素的Alpha值
			FreeImage_GetPixelColor(fiB, x, y, &color);

			if (bInvertedAlpha && color.rgbReserved < 128)
			{
				color.rgbRed = 255 - color.rgbRed;
				color.rgbGreen = 255 - color.rgbGreen;
				color.rgbBlue = 255 - color.rgbBlue;
			}
			// 写入到新图像中（考虑偏移量）
			FreeImage_SetPixelColor(newBitmap, x + offsetX, y + offsetY, &color);
		}
	}

	return newBitmap;
}

FIBITMAP* BS_NormalizeToSquareA24(FIBITMAP* fiB, bool bInvertedAlpha)
{
	// 获取原图像的宽度和高度
	unsigned int width = FreeImage_GetWidth(fiB);
	unsigned int height = FreeImage_GetHeight(fiB);

	// 计算新图像的尺寸（取宽高的最大值）
	unsigned int newSize = (width > height) ? width : height;

	// 创建一个新的方形图像 Always 24bpp
	FIBITMAP* newBitmap = FreeImage_Allocate(newSize, newSize, 24);
	if (!newBitmap)
	{
		return NULL;
	}

	// 获取左上角像素的颜色值
	RGBQUAD cornerColor{};

	// 使用FreeImage_FillBackground填充整个新图像
	FreeImage_FillBackground(newBitmap, &cornerColor);

	// 计算原图像在新图像中的起始位置（居中）
	unsigned int offsetX = (newSize - width) / 2;
	unsigned int offsetY = (newSize - height) / 2;

	// 将fiB的Alpha通道的值作为R、G、B值，写入newBitmap（如果fiB比newBitmap小则居中写入）
	RGBQUAD color;
	BYTE alpha;

	// 检查原图像是否有Alpha通道
	bool hasAlpha = FreeImage_IsTransparent(fiB);

	// 遍历原图像的每个像素
	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			// 获取原图像像素的Alpha值
			if (hasAlpha)
			{
				FreeImage_GetPixelColor(fiB, x, y, &color);
				alpha = color.rgbReserved; // Alpha通道值
			}
			else
			{
				alpha = 255; // 如果没有Alpha通道，默认为不透明
			}

			// 将Alpha值设置为新像素的RGB值
			color.rgbRed = alpha;
			color.rgbGreen = alpha;
			color.rgbBlue = alpha;
			color.rgbReserved = 255;

			if (bInvertedAlpha)
			{
				color.rgbRed = 255 - color.rgbRed;
				color.rgbGreen = 255 - color.rgbGreen;
				color.rgbBlue = 255 - color.rgbBlue;
			}

			// 写入到新图像中（考虑偏移量）
			FreeImage_SetPixelColor(newBitmap, x + offsetX, y + offsetY, &color);
		}
	}

	return newBitmap;
}

FIBITMAP* BS_NormalizeToSquareRGBA24(FIBITMAP* fibSquareRGB, FIBITMAP* fibSquareA, bool bInvertedAlpha)
{
	unsigned int width = FreeImage_GetWidth(fibSquareRGB);
	unsigned int height = FreeImage_GetHeight(fibSquareRGB);

	// 创建一个新图像，宽度为fibSquareRGB和fibSquareA之和，将fibSquareRGB置于左侧，fibSquareA置于右侧
	FIBITMAP* newBitmap = FreeImage_Allocate(width * 2, height, 24);

	if (!newBitmap)
	{
		return NULL;
	}

	// 复制fibSquareRGB到左侧
	FreeImage_Paste(newBitmap, fibSquareRGB, 0, 0, 255);

	// 复制fibSquareA到右侧
	FreeImage_Paste(newBitmap, fibSquareA, width, 0, 255);

	return newBitmap;
}

FIBITMAP* BS_CreateBackgroundRGBA24(FIBITMAP* fibSource, FIBITMAP* fibBackground, int iRandomBackgroundType, bool bWithAlphaChannel)
{
	auto width = FreeImage_GetWidth(fibSource);
	auto height = FreeImage_GetHeight(fibSource);
	FIBITMAP* newFibSource = nullptr;
	FIBITMAP* newBackground = nullptr;
	unsigned int nw{};
	unsigned int nh{};

	if (height <= 512)
	{
		newFibSource = FreeImage_Rescale(fibSource, (width * 512.0f / height), 512);
		newBackground = FreeImage_Rescale(fibBackground, 2560, 1080);
	}
	else if (height <= 768)
	{
		auto newFibSource = FreeImage_Rescale(fibSource, (width * 768.0f / height), 768);
		newBackground = FreeImage_Rescale(fibBackground, 3440, 1440);
	}
	else
	{
		auto newFibSource = FreeImage_Rescale(fibSource, (width * 1024.0f / height), 1024);
		newBackground = FreeImage_Rescale(fibBackground, 5160, 2160);
	}

	{
		nw = FreeImage_GetWidth(newBackground);
		nh = FreeImage_GetHeight(newBackground);

		if (iRandomBackgroundType == 0)
			FreeImage_Paste(newBackground, newFibSource, (nw / 2) - (FreeImage_GetWidth(newFibSource) / 2) - 1, (nh / 2) - (FreeImage_GetWidth(newFibSource) / 2) - 1, 255);
		if (iRandomBackgroundType == 1)
			FreeImage_Paste(newBackground, newFibSource, 0, 0, 255);
		if (iRandomBackgroundType == 2)
			FreeImage_Paste(newBackground, newFibSource, nw - FreeImage_GetWidth(newFibSource) - 1, 0, 255);

		FreeImage_Unload(newFibSource);
	}

	if (!bWithAlphaChannel)
	{
		if (iRandomBackgroundType == 0)//all R
		{
			RGBQUAD mark;

			mark = { 0, 0, 255, 255 };
			FreeImage_SetPixelColor(newBackground, 0, 0, &mark);
			FreeImage_SetPixelColor(newBackground, 0, 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, 0, &mark);
			mark = { 0, 0, 255, 255 };
			FreeImage_SetPixelColor(newBackground, 0, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, 0, nh - 2, &mark);
			FreeImage_SetPixelColor(newBackground, 1, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, nh - 2, &mark);
			mark = { 0, 0, 255, 255 };
			FreeImage_SetPixelColor(newBackground, nw - 1, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 1, nh - 2, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, nh - 2, &mark);
			mark = { 0, 0, 255, 255 };
			FreeImage_SetPixelColor(newBackground, nw - 1, 0, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, 0, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 1, 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, 1, &mark);
		}
		if (iRandomBackgroundType == 1)//all G
		{
			RGBQUAD mark;

			mark = { 0, 255, 0, 255 };
			FreeImage_SetPixelColor(newBackground, 0, 0, &mark);
			FreeImage_SetPixelColor(newBackground, 0, 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, 0, &mark);
			mark = { 0, 255, 0, 255 };
			FreeImage_SetPixelColor(newBackground, 0, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, 0, nh - 2, &mark);
			FreeImage_SetPixelColor(newBackground, 1, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, nh - 2, &mark);
			mark = { 0, 255, 0, 255 };
			FreeImage_SetPixelColor(newBackground, nw - 1, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 1, nh - 2, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, nh - 2, &mark);
			mark = { 0, 255, 0, 255 };
			FreeImage_SetPixelColor(newBackground, nw - 1, 0, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, 0, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 1, 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, 1, &mark);
		}
		if (iRandomBackgroundType == 2)//all B
		{
			RGBQUAD mark;

			mark = { 255, 0, 0, 255 };
			FreeImage_SetPixelColor(newBackground, 0, 0, &mark);
			FreeImage_SetPixelColor(newBackground, 0, 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, 0, &mark);
			mark = { 255, 0, 0, 255 };
			FreeImage_SetPixelColor(newBackground, 0, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, 0, nh - 2, &mark);
			FreeImage_SetPixelColor(newBackground, 1, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, nh - 2, &mark);
			mark = { 255, 0, 0, 255 };
			FreeImage_SetPixelColor(newBackground, nw - 1, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 1, nh - 2, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, nh - 2, &mark);
			mark = { 255, 0, 0, 255 };
			FreeImage_SetPixelColor(newBackground, nw - 1, 0, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, 0, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 1, 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, 1, &mark);
		}
	}
	else
	{
		if (iRandomBackgroundType == 0)//leftR rightG
		{
			RGBQUAD mark;

			mark = { 0, 0, 255, 255 };
			FreeImage_SetPixelColor(newBackground, 0, 0, &mark);
			FreeImage_SetPixelColor(newBackground, 0, 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, 0, &mark);
			mark = { 0, 0, 255, 255 };
			FreeImage_SetPixelColor(newBackground, 0, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, 0, nh - 2, &mark);
			FreeImage_SetPixelColor(newBackground, 1, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, nh - 2, &mark);
			mark = { 0, 255, 0, 255 };
			FreeImage_SetPixelColor(newBackground, nw - 1, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 1, nh - 2, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, nh - 2, &mark);
			mark = { 0, 255, 0, 255 };
			FreeImage_SetPixelColor(newBackground, nw - 1, 0, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, 0, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 1, 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, 1, &mark);
		}
		if (iRandomBackgroundType == 1)//leftG rightB
		{
			RGBQUAD mark;

			mark = { 0, 255, 0, 255 };
			FreeImage_SetPixelColor(newBackground, 0, 0, &mark);
			FreeImage_SetPixelColor(newBackground, 0, 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, 0, &mark);
			mark = { 0, 255, 0, 255 };
			FreeImage_SetPixelColor(newBackground, 0, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, 0, nh - 2, &mark);
			FreeImage_SetPixelColor(newBackground, 1, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, nh - 2, &mark);
			mark = { 255, 0, 0, 255 };
			FreeImage_SetPixelColor(newBackground, nw - 1, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 1, nh - 2, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, nh - 2, &mark);
			mark = { 255, 0, 0, 255 };
			FreeImage_SetPixelColor(newBackground, nw - 1, 0, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, 0, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 1, 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, 1, &mark);
		}
		if (iRandomBackgroundType == 2)//leftR rightB
		{
			RGBQUAD mark;

			mark = { 0, 0, 255, 255 };
			FreeImage_SetPixelColor(newBackground, 0, 0, &mark);
			FreeImage_SetPixelColor(newBackground, 0, 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, 0, &mark);
			mark = { 0, 0, 255, 255 };
			FreeImage_SetPixelColor(newBackground, 0, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, 0, nh - 2, &mark);
			FreeImage_SetPixelColor(newBackground, 1, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, 1, nh - 2, &mark);
			mark = { 255, 0, 0, 255 };
			FreeImage_SetPixelColor(newBackground, nw - 1, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 1, nh - 2, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, nh - 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, nh - 2, &mark);
			mark = { 255, 0, 0, 255 };
			FreeImage_SetPixelColor(newBackground, nw - 1, 0, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, 0, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 1, 1, &mark);
			FreeImage_SetPixelColor(newBackground, nw - 2, 1, &mark);
		}
	}

	return newBackground;
}

int BS_GetRandomBackgroundType(FIBITMAP* fibBackground)
{
	auto w = FreeImage_GetWidth(fibBackground);
	auto h = FreeImage_GetHeight(fibBackground);

	if (IsPixelWhite(fibBackground, 0, h - 1) &&
		IsPixelWhite(fibBackground, 1, h - 2) &&
		IsPixelWhite(fibBackground, 2, h - 3))
	{
		return 1;
	}

	if (
		IsPixelWhite(fibBackground, w - 1, h - 1) &&
		IsPixelWhite(fibBackground, w - 2, h - 2) &&
		IsPixelWhite(fibBackground, w - 3, h - 3))
	{
		return 2;
	}

	return 0;
}

FIBITMAP* BS_LoadRandomBackground(int numBackgrounds)
{
	std::string filePath = std::format("bettersprays/random_background_{0}.jpg", gEngfuncs.pfnRandomLong(0, numBackgrounds));

	FIBITMAP* fiB = NULL;

	{
		auto fileHandle = FILESYSTEM_ANY_OPEN(filePath.c_str(), "rb");

		if (!fileHandle)
		{
			gEngfuncs.Con_Printf("[BetterSpray] Could not open \"%s\" for reading.\n", filePath.c_str());
			return nullptr;
		}

		SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(fileHandle); };

		FreeImageIO fiIO;
		fiIO.read_proc = FI_Read;
		fiIO.write_proc = FI_Write;
		fiIO.seek_proc = FI_Seek;
		fiIO.tell_proc = FI_Tell;

		auto fiFormat = FIF_UNKNOWN;

		if (fiFormat == FIF_UNKNOWN)
		{
			fiFormat = FreeImage_GetFileTypeFromHandle(&fiIO, (fi_handle)fileHandle);
		}

		if (fiFormat == FIF_UNKNOWN)
		{
			gEngfuncs.Con_Printf("[BetterSpray] Could not open \"%s\", Unknown format.\n", filePath.c_str());
			return nullptr;
		}

		if (!FreeImage_FIFSupportsReading(fiFormat))
		{
			gEngfuncs.Con_Printf("[BetterSpray] Could not open \"%s\", Unsupported format.\n", filePath.c_str());
			return nullptr;
		}

		fiB = FreeImage_LoadFromHandle(fiFormat, &fiIO, (fi_handle)fileHandle, 0);

		if (!fiB)
		{
			gEngfuncs.Con_Printf("[BetterSpray] Could not upload \"%s\", FreeImage_LoadFromHandle failed.\n", filePath.c_str());
			return nullptr;
		}
	}

	return fiB;
}

bool BS_UploadSprayBitmap(FIBITMAP* fiB, bool bNormalizeToSquare, bool bWithAlphaChannel, bool bInvertedAlpha, bool bRandomBackground)
{
	FreeImageIO fiIO;
	fiIO.read_proc = FI_Read;
	fiIO.write_proc = FI_Write;
	fiIO.seek_proc = FI_Seek;
	fiIO.tell_proc = FI_Tell;

	auto steamId = SteamUser()->GetSteamID();

	char userId[32]{};
	snprintf(userId, sizeof(userId), "%llu", steamId.ConvertToUint64());

	char newFileName[256]{};
	char newFilePath[256]{};

	unsigned char rgucMD5_WAD[16]{};
	char wadHash[64]{};

	auto width = FreeImage_GetWidth(fiB);
	auto height = FreeImage_GetHeight(fiB);
	FileHandle_t hNewFileHandle = nullptr;

	const auto& OpenFileHandle = [&hNewFileHandle, &userId, &rgucMD5_WAD, &newFileName, &newFilePath, &wadHash]() -> bool {

		snprintf(wadHash, sizeof(wadHash), "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
			rgucMD5_WAD[0],
			rgucMD5_WAD[1],
			rgucMD5_WAD[2],
			rgucMD5_WAD[3],
			rgucMD5_WAD[4],
			rgucMD5_WAD[5],
			rgucMD5_WAD[6],
			rgucMD5_WAD[7],
			rgucMD5_WAD[8],
			rgucMD5_WAD[9],
			rgucMD5_WAD[10],
			rgucMD5_WAD[11],
			rgucMD5_WAD[12],
			rgucMD5_WAD[13],
			rgucMD5_WAD[14],
			rgucMD5_WAD[15]);

			snprintf(newFileName, sizeof(newFileName), "%s_%s.jpg", userId, wadHash);
			snprintf(newFilePath, sizeof(newFilePath), "%s/%s", CUSTOM_SPRAY_DIRECTORY, newFileName);

			hNewFileHandle = FILESYSTEM_ANY_OPEN(newFilePath, "wb", "GAMEDOWNLOAD");

			if (!hNewFileHandle)
			{
				gEngfuncs.Con_Printf("[BetterSpray] Could not open \"%s\" for writing.\n", newFilePath);
				return false;
			}

			return true;
		};

	bool bSaved = false;

	if (1)
	{
		FILESYSTEM_ANY_CREATEDIR(CUSTOM_SPRAY_DIRECTORY, "GAMEDOWNLOAD");

		if (bNormalizeToSquare)
		{
			if (bWithAlphaChannel && FreeImage_GetBPP(fiB) == 32)
			{
				auto newFIB32 = BS_NormalizeToSquareRGBA32(fiB);

				if (newFIB32)
				{
					BS_SaveBitmapToTempDecal(newFIB32, rgucMD5_WAD);

					FreeImage_Unload(newFIB32);

					if (!OpenFileHandle())
						return false;
				}

				auto fibSquareRGB24 = BS_NormalizeToSquareRGB24(fiB, bInvertedAlpha);
				auto fibSquareA24 = BS_NormalizeToSquareA24(fiB, bInvertedAlpha);

				auto newFIB24 = BS_NormalizeToSquareRGBA24(fibSquareRGB24, fibSquareA24, bInvertedAlpha);

				FreeImage_Unload(fibSquareRGB24);
				FreeImage_Unload(fibSquareA24);

				if (bRandomBackground)
				{
					auto fibBackground = BS_LoadRandomBackground(3);

					if (fibBackground)
					{
						auto iBackgroundType = BS_GetRandomBackgroundType(fibBackground);
						auto newFIB24WithBackground = BS_CreateBackgroundRGBA24(newFIB24, fibBackground, iBackgroundType, true);

						FreeImage_Unload(fibBackground);

						bSaved = FreeImage_SaveToHandle(FIF_JPEG, newFIB24WithBackground, &fiIO, (fi_handle)hNewFileHandle);

						//if (bSaved)
						//{
						//	width = FreeImage_GetWidth(newFIB24WithBackground);
						//	height = FreeImage_GetHeight(newFIB24WithBackground);
						//}

						FreeImage_Unload(newFIB24WithBackground);
					}
				}
				else
				{
					bSaved = FreeImage_SaveToHandle(FIF_JPEG, newFIB24, &fiIO, (fi_handle)hNewFileHandle);

					//if (bSaved)
					//{
					//	width = FreeImage_GetWidth(newFIB24);
					//	height = FreeImage_GetHeight(newFIB24);
					//}
				}

				FreeImage_Unload(newFIB24);
			}
			else
			{
				//no alpha channel
				auto fibSquareRGB24 = BS_NormalizeToSquareRGB24(fiB, false);

				if (fibSquareRGB24)
				{
					BS_SaveBitmapToTempDecal(fibSquareRGB24, rgucMD5_WAD);

					if (!OpenFileHandle())
						return false;

					if (bRandomBackground)
					{
						auto fibBackground = BS_LoadRandomBackground(3);

						if (fibBackground)
						{
							auto iBackgroundType = BS_GetRandomBackgroundType(fibBackground);
							auto newFIB24WithBackground = BS_CreateBackgroundRGBA24(fibSquareRGB24, fibBackground, iBackgroundType, false);

							FreeImage_Unload(fibBackground);

							bSaved = FreeImage_SaveToHandle(FIF_JPEG, newFIB24WithBackground, &fiIO, (fi_handle)hNewFileHandle);

							//if (bSaved)
							//{
							//	width = FreeImage_GetWidth(newFIB24WithBackground);
							//	height = FreeImage_GetHeight(newFIB24WithBackground);
							//}

							FreeImage_Unload(newFIB24WithBackground);
						}
					}
					else
					{
						bSaved = FreeImage_SaveToHandle(FIF_JPEG, fibSquareRGB24, &fiIO, (fi_handle)hNewFileHandle);

						//if (bSaved)
						//{
						//	width = FreeImage_GetWidth(fibSquareRGB24);
						//	height = FreeImage_GetHeight(fibSquareRGB24);
						//}
					}
				}

				FreeImage_Unload(fibSquareRGB24);
			}
		}
		else
		{
			BS_SaveBitmapToTempDecal(fiB, rgucMD5_WAD);

			if (!OpenFileHandle())
				return false;

			bSaved = FreeImage_SaveToHandle(FIF_JPEG, fiB, &fiIO, (fi_handle)hNewFileHandle);

			//if (bSaved)
			//{
			//	width = FreeImage_GetWidth(fiB);
			//	height = FreeImage_GetHeight(fiB);
			//}
		}

		if (bSaved)
		{
			gEngfuncs.Con_DPrintf("[BetterSpray] File \"%s\" saved !\n", newFilePath);
		}
		else
		{
			gEngfuncs.Con_Printf("[BetterSpray] Could not save to \"%s\", FreeImage_SaveToHandle failed!\n", newFilePath);
		}
	}
	else
	{
		BS_SaveBitmapToTempDecal(fiB, rgucMD5_WAD);

		if (!OpenFileHandle())
			return false;

		//width = FreeImage_GetWidth(fiB);
		//height = FreeImage_GetHeight(fiB);
	}

	if (hNewFileHandle)
	{
		FILESYSTEM_ANY_CLOSE(hNewFileHandle);
		hNewFileHandle = nullptr;
	}

	if (!bSaved)
		return false;

	char fullPath[MAX_PATH] = { 0 };
	auto pszFullPath = FILESYSTEM_ANY_GETLOCALPATH(newFilePath, fullPath, MAX_PATH);

	if (!pszFullPath)
	{
		gEngfuncs.Con_Printf("[BetterSpray] Could not upload \"%s\", GetLocalPath failed.\n", newFilePath);
		return false;
	}

	SteamScreenshots()->AddScreenshotToLibrary(pszFullPath, nullptr, 0, 0);

	if (EngineIsInLevel())
	{
		int playerindex = EngineFindPlayerIndexByUserId(userId);

		if (playerindex != -1)
		{
			Draw_LoadSprayTexture(userId, newFilePath, "GAMEDOWNLOAD", [playerindex](const char* userId, FIBITMAP* fiB) -> LOADSPRAYTEXTURE_STATUS {
				return Draw_LoadSprayTexture_AsyncLoadInGame(playerindex, fiB);
				});
		}
	}

	gEngfuncs.Con_Printf("[BetterSpray] \"%s\" has been committed to Steam screenshot library. Please share it as \"Public\" on Steam with \"!Spray\" as screenshot description.\n", newFilePath);
	return true;
}

void HUD_Frame(double frame)
{
	gExportfuncs.HUD_Frame(frame);

	SprayDatabase()->RunFrame();
	UtilHTTPClient()->RunFrame();

	float time = gEngfuncs.GetAbsoluteTime();

	GameThreadTaskScheduler()->RunTasks(time, 0);
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	SprayDatabase()->Init();
}

int HUD_VidInit(void)
{
	//hpk_maxsize cannot be -1 otherwise we will not be able to use tempdecal.wad.
	auto hpk_maxsize = gEngfuncs.pfnGetCvarPointer("hpk_maxsize");

	if (hpk_maxsize && hpk_maxsize->value < 1)
	{
		gEngfuncs.Cvar_SetValue("hpk_maxsize", 4);
	}

	return gExportfuncs.HUD_VidInit();
}

void HUD_Shutdown(void)
{
	SprayDatabase()->Shutdown();

	gExportfuncs.HUD_Shutdown();

	UtilHTTPClient_Shutdown();
	UtilThreadTask_Shutdown();
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s** ppinterface, struct engine_studio_api_s* pstudio)
{
	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));

	auto ret = gExportfuncs.HUD_GetStudioModelInterface ? gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio) : 1;

	return ret;
}