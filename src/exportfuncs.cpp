#include <metahook.h>
#include <cmath>
#include <format>
#include "cvardef.h"
#include "plugins.h"
#include "privatehook.h"
#include "SprayDatabase.h"
#include "UtilHTTPClient.h"
#include "UtilThreadTask.h"

#include <FreeImage.h>

#include <ScopeExit/ScopeExit.h>

#include <steam_api.h>

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


/*
	Purpose: Upload BGRA8 spray texture to OpenGL.
*/

void Draw_UploadSprayTextureBGRA8(int playerindex, FIBITMAP* fiB)
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
	}
}

static int Draw_FindPlayerIndexByUserId(const char* userId)
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

class CLoadSprayTextureWorkItemContext : public IThreadedTask
{
public:
	CLoadSprayTextureWorkItemContext(int playerindex, const char* userId, FIBITMAP* fiB) :
		m_playerindex(playerindex),
		m_userId(userId),
		m_fiB(fiB)
	{
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
		Draw_UploadSprayTextureBGRA8(m_playerindex, m_fiB);
		FreeImage_Unload(m_fiB);
	}

public:
	int m_playerindex{};
	std::string m_userId{};
	FIBITMAP* m_fiB{};
};

void Draw_LoadSprayTexture_WorkItem(CLoadSprayTextureWorkItemContext* ctx)
{
	FIBITMAP *fiB = ctx->m_fiB;
	
	FIBITMAP* newBitmap = nullptr;
	
	// 检查是否为24bpp图像
	if (FreeImage_GetBPP(fiB) == 24)
	{
		unsigned width = FreeImage_GetWidth(fiB);
		unsigned height = FreeImage_GetHeight(fiB);
		
		// 检查宽度是否为高度的两倍
		if (width == height * 2)
		{
			// 创建一个新的32bpp图像，宽度为原图的一半
			newBitmap = FreeImage_Allocate(width / 2, height, 32);
			
			if (newBitmap)
			{
				// 检查最右下角像素是否为纯白色
				bool bInvertedAlpha = false;

				RGBQUAD color;
				FreeImage_GetPixelColor(fiB, width - 1, height - 1, &color);
				if (color.rgbRed > 250 && color.rgbGreen >= 250 && color.rgbBlue >= 250)
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

				ctx->m_fiB = newBitmap;

				FreeImage_Unload(fiB);

				GameThreadTaskScheduler()->QueueTask(ctx);
				return;
			}
		}
	}
	
	newBitmap = FreeImage_ConvertTo32Bits(fiB);

	ctx->m_fiB = newBitmap;

	FreeImage_Unload(fiB);

	GameThreadTaskScheduler()->QueueTask(ctx);
}

/*
	Purpose: Load spray texture from FileSystem.
*/

int Draw_LoadSprayTexture(int playerindex, const char* userId, const char* fileName, const char* pathId)
{
	if (playerindex < 0)
	{
		playerindex = Draw_FindPlayerIndexByUserId(userId);
	}

	if (playerindex >= 0 && playerindex < gEngfuncs.GetMaxClients())
	{
		std::string filePath = std::format("{0}/{1}", CUSTOM_SPRAY_DIRECTORY, fileName);

		auto fileHandle = FILESYSTEM_ANY_OPEN(filePath.c_str(), "rb", pathId);

		if (!fileHandle)
		{
			gEngfuncs.Con_DPrintf("Draw_UploadSprayTexture: Could not open \"%s\" for reading.\n", filePath.c_str());
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
			gEngfuncs.Con_Printf("Draw_UploadSprayTexture: Could not load \"%s\", Unknown format.\n", filePath.c_str());
			return -2;
		}

		if (!FreeImage_FIFSupportsReading(fiFormat))
		{
			gEngfuncs.Con_Printf("Draw_UploadSprayTexture: Could not load \"%s\", Unsupported format.\n", filePath.c_str());
			return -3;
		}

		auto fiB = FreeImage_LoadFromHandle(fiFormat, &fiIO, (fi_handle)fileHandle, 0);

		if (!fiB)
		{
			gEngfuncs.Con_Printf("Draw_UploadSprayTexture: Could not load \"%s\", FreeImage_LoadFromHandle failed.\n", filePath.c_str());
			return -4;
		}

		
#if 0
		SCOPE_EXIT{ FreeImage_Unload(fiB); };

		auto fiBGRA8 = FreeImage_ConvertTo32Bits(fiB);

		if (!fiBGRA8)
		{
			gEngfuncs.Con_Printf("Draw_UploadSprayTexture: Could not load \"%s\", FreeImage_ConvertTo32Bits failed.\n", filePath.c_str());
			return -5;
		}

		SCOPE_EXIT{ FreeImage_Unload(fiBGRA8); };

		Draw_UploadSprayTextureBGRA8(playerindex, fiBGRA8);

#else

		auto ctx = new CLoadSprayTextureWorkItemContext(playerindex, userId, fiB);

		auto hWorkItemHandle = g_pMetaHookAPI->CreateWorkItem(g_pMetaHookAPI->GetGlobalThreadPool(), [](void* context) -> bool {

			auto ctx = (CLoadSprayTextureWorkItemContext*)context;

			Draw_LoadSprayTexture_WorkItem(ctx);

			return true;

		}, ctx);

		g_pMetaHookAPI->QueueWorkItem(g_pMetaHookAPI->GetGlobalThreadPool(), hWorkItemHandle);
#endif


		return 0;
	}
	else
	{
		gEngfuncs.Con_DPrintf("Draw_UploadSprayTexture: invalid player index.\n");
	}

	return -7;
}

/*
	Purpose: Load spray texture from FileSystem, try {SteamID}.jpg
*/

int Draw_LoadSprayTextures(int playerindex, const char* userId, const char* pathId)
{
	auto fileName = std::format("{0}.jpg", userId);

	auto err = Draw_LoadSprayTexture(playerindex, userId, fileName.c_str(), "GAMEDOWNLOAD");

	return err;
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

					auto queryStatus = SprayDatabase()->GetPlayerSprayQueryStatus(userId.c_str());

					if (queryStatus == SprayQueryState_Unknown)
					{
						auto err = Draw_LoadSprayTextures(playerindex, userId.c_str(), "GAMEDOWNLOAD");

						if (err == 0)
						{
							retval->name[0] = '?';

							SprayDatabase()->UpdatePlayerSprayQueryStatus(userId.c_str(), SprayQueryState_Finished);
						}
						else if (err == -1)
						{
							//File not found ?
							SprayDatabase()->QueryPlayerSpray(playerindex, userId.c_str());
						}
						else
						{
							//Could be invalid file or what, tonikaku no more retry.
							SprayDatabase()->UpdatePlayerSprayQueryStatus(userId.c_str(), SprayQueryState_Failed);
						}
					}
					else if (queryStatus == SprayQueryState_Finished)
					{
						auto err = Draw_LoadSprayTextures(playerindex, userId.c_str(), "GAMEDOWNLOAD");

						if (err == 0)
						{
							retval->name[0] = '?';
						}
					}
				}
			}
		}
	}

	return retval;
}

bool Draw_ValidateImageFormatMemoryIO(const void *data, size_t dataSize, const char *identifier)
{
	FIMEMORY* fim = FreeImage_OpenMemory((BYTE *)data, dataSize);

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

FIBITMAP* BS_NormalizeToSquareRGB(FIBITMAP* fiB)
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
	
	// 使用FreeImage_Paste将原图像粘贴到新图像的中心位置
	//FreeImage_Paste(newBitmap, fiB, offsetX, offsetY, 255);
	
	RGBQUAD color;

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

FIBITMAP *BS_NormalizeToSquareA(FIBITMAP* fiB, bool bAlphaInverted)
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

			if (bAlphaInverted)
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

FIBITMAP* BS_NormalizeToSquareRGBA(FIBITMAP* fiB, bool bAlphaInverted)
{
	auto fibSquareRGB = BS_NormalizeToSquareRGB(fiB);
	auto fibSquareA = BS_NormalizeToSquareA(fiB, bAlphaInverted);

	SCOPE_EXIT{ FreeImage_Unload(fibSquareRGB); };
	SCOPE_EXIT{ FreeImage_Unload(fibSquareA); };

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

void BS_UploadSprayFile(const char *fileName, bool bNormalizeToSquare, bool bWithAlphaChannel, bool bAlphaInverted)
{
	auto steamId = SteamUser()->GetSteamID();
	auto userId = std::format("{0}", steamId.ConvertToUint64());

	std::string filePath = std::format("{0}/{1}", CUSTOM_SPRAY_DIRECTORY, fileName);

	auto fileHandle = FILESYSTEM_ANY_OPEN(filePath.c_str(), "rb");

	if (!fileHandle)
	{
		gEngfuncs.Con_Printf("[BetterSpray] Could not open \"%s\" for reading.\n", filePath.c_str());
		return;
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
		gEngfuncs.Con_Printf("[BetterSpray] Could not upload \"%s\", Unknown format.\n", filePath.c_str());
		return;
	}

	if (!FreeImage_FIFSupportsReading(fiFormat))
	{
		gEngfuncs.Con_Printf("[BetterSpray] Could not upload \"%s\", Unsupported format.\n", filePath.c_str());
		return;
	}

#if 0
	if (fiFormat != FIF_JPEG && fiFormat != FIF_PNG && fiFormat != FIF_TARGA)
	{
		gEngfuncs.Con_Printf("[BetterSpray] Could not upload \"%s\", Only JPEG / PNG / TGA supported.\n", filePath.c_str());
		return;
	}
#endif

	auto fiB = FreeImage_LoadFromHandle(fiFormat, &fiIO, (fi_handle)fileHandle, 0);

	if (!fiB)
	{
		gEngfuncs.Con_Printf("[BetterSpray] Could not upload \"%s\", FreeImage_LoadFromHandle failed.\n", filePath.c_str());
		return;
	}

	SCOPE_EXIT{ FreeImage_Unload(fiB); };

	std::string newFilePath = std::format("{0}/{1}.jpg", CUSTOM_SPRAY_DIRECTORY, userId);

	if (newFilePath != filePath)
	{
		auto newFileHandle = FILESYSTEM_ANY_OPEN(newFilePath.c_str(), "wb", "GAMEDOWNLOAD");

		if (newFileHandle)
		{
			bool bSaved = false;

			if (bNormalizeToSquare)
			{
				if (bWithAlphaChannel && FreeImage_GetBPP(fiB) == 32)
				{
					auto newFIB = BS_NormalizeToSquareRGBA(fiB, bAlphaInverted);

					SCOPE_EXIT{ FreeImage_Unload(newFIB); };

					bSaved = FreeImage_SaveToHandle(FIF_JPEG, newFIB, &fiIO, (fi_handle)newFileHandle);
				}
				else
				{
					auto newFIB = BS_NormalizeToSquareRGB(fiB);

					SCOPE_EXIT{ FreeImage_Unload(newFIB); };

					bSaved = FreeImage_SaveToHandle(FIF_JPEG, newFIB, &fiIO, (fi_handle)newFileHandle);
				}
			}
			else
			{
				bSaved = FreeImage_SaveToHandle(FIF_JPEG, fiB, &fiIO, (fi_handle)newFileHandle);
			}
			
			FILESYSTEM_ANY_CLOSE(newFileHandle);

			if (bSaved)
			{
				gEngfuncs.Con_DPrintf("[BetterSpray] File \"%s\" saved !\n", newFilePath.c_str());
			}
			else
			{
				gEngfuncs.Con_Printf("[BetterSpray] Could not save \"%s\", FreeImage_SaveToHandle failed!\n", newFilePath.c_str());
				return;
			}
		}
		else
		{
			gEngfuncs.Con_Printf("[BetterSpray] Could not open \"%s\" for write !\n", newFilePath.c_str());
			return;
		}
	}

	auto width = FreeImage_GetWidth(fiB);
	auto height = FreeImage_GetHeight(fiB);

	char fullPath[MAX_PATH] = { 0 };
	auto pszFullPath = FILESYSTEM_ANY_GETLOCALPATH(newFilePath.c_str(), fullPath, MAX_PATH);

	if (!pszFullPath)
	{
		gEngfuncs.Con_Printf("[BetterSpray] Could not upload \"%s\", GetLocalPath failed.\n", newFilePath.c_str());
		return;
	}

	SteamScreenshots()->AddScreenshotToLibrary(pszFullPath, nullptr, width, height);

	Draw_LoadSprayTexture(-1, userId.c_str(), newFilePath.c_str(), "GAMEDOWNLOAD");

	gEngfuncs.Con_Printf("[BetterSpray] \"%s\" has been uploaded to Steam screenshot library. Please set \"!Spray\" as screenshot description.\n", newFilePath.c_str());
}

void BS_Upload_f()
{
	if (gEngfuncs.Cmd_Argc() < 2) {
		gEngfuncs.Con_Printf("File name must be specified! e.g. \"bs_upload 123.jpg\" \n");
		return;
	}

	auto fileName = gEngfuncs.Cmd_Argv(1);

	BS_UploadSprayFile(fileName, true, true, true);
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

	gEngfuncs.pfnAddCommand("bs_upload", BS_Upload_f);

	SprayDatabase()->Init();
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