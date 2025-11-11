#include <interface.h>
#include <IFileSystem.h>
#include "SprayDatabase.h"
#include "BetterSpraySettingsPage.h"
#include "exportfuncs.h"
#include "plugins.h"
#include "wad3.hpp"

#include <vgui_controls/FileOpenDialog.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/MemoryBitmap.h>
#include <vgui_controls/MessageBox.h>

#include <ScopeExit/ScopeExit.h>

#include <steam_api.h>

#include <format>

CBetterSpraySettingsPage::CBetterSpraySettingsPage(vgui::Panel* parent, const char* name) :
	BaseClass(parent, name)
{
	SetSize(vgui::scheme()->GetProportionalScaledValue(624), vgui::scheme()->GetProportionalScaledValue(300));

	m_pSparyImageWAD = new vgui::ImagePanel(this, "SparyImageWAD");
	m_pSparyImageJPG = new vgui::ImagePanel(this, "SparyImageJPG");

	m_pRandomBackground = new vgui::CheckButton(this, "RandomBackground", "#GameUI_BetterSpray_RandomBackground");
	m_pWithAlpha = new vgui::CheckButton(this, "WithAlpha", "#GameUI_BetterSpray_WithAlpha");
	m_pInvertAlpha = new vgui::CheckButton(this, "InvertAlpha", "#GameUI_BetterSpray_InvertAlpha");
	m_pRefreshSpray = new vgui::Button(this, "RefreshSpray", "#GameUI_BetterSpray_Refresh", this, "RefreshSpray");
	m_pLoadSpary = new vgui::Button(this, "LoadSpary", "#GameUI_BetterSpray_Load", this, "OpenLoadSparyDialog");

	m_pSparyWad = new WadFile();

	LoadControlSettings("bettersprays/BetterSpraySettingsPage.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());

	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

CBetterSpraySettingsPage::~CBetterSpraySettingsPage()
{
	if (m_pSparyBitmapJPG) {
		delete m_pSparyBitmapJPG;
		m_pSparyBitmapJPG = nullptr;
	}

	if (m_pSparyBitmapWAD) {
		delete m_pSparyBitmapWAD;
		m_pSparyBitmapWAD = nullptr;
	}

	if (m_pSparyWad) {
		delete m_pSparyWad;
		m_pSparyWad = nullptr;
	}
}

void CBetterSpraySettingsPage::ApplyChangesToConVar(const char* pConVarName, int value)
{
	char szCmd[256] = { 0 };
	Q_snprintf(szCmd, sizeof(szCmd) - 1, "%s %d\n", pConVarName, value);
	gEngfuncs.pfnClientCmd(szCmd);
}

void CBetterSpraySettingsPage::ApplyChanges(void)
{

}

void CBetterSpraySettingsPage::OnApplyChanges(void)
{
	ApplyChanges();
}

void CBetterSpraySettingsPage::OnResetData(void)
{
	unsigned char rgucMD5_WAD[16]{};
	char wadHash[64]{};
	char fileName[256]{};
	char filePath[256]{};

	bool bTempDecalLoaded = false;

	if (m_pSparyWad)
	{
		bTempDecalLoaded = m_pSparyWad->Load("tempdecal.wad");
		auto tex = m_pSparyWad->Get("{logo");

		if (tex)
		{
			if (m_pSparyBitmapWAD) {
				delete m_pSparyBitmapWAD;
				m_pSparyBitmapWAD = nullptr;
			}

			m_pSparyBitmapWAD = new vgui::MemoryBitmap(tex->GetPixels(), tex->GetWidth(), tex->GetHeight());
			m_pSparyImageWAD->SetImage(m_pSparyBitmapWAD);
		}
		else
		{
			m_pSparyImageWAD->SetImage((vgui::IImage *)nullptr);
		}

		if (bTempDecalLoaded) {
			memcpy(rgucMD5_WAD, m_pSparyWad->GetMD5(), sizeof(rgucMD5_WAD));
		}
	}

	if (1)
	{
		auto steamId = SteamUser()->GetSteamID();

		char userId[32]{};
		snprintf(userId, sizeof(userId), "%llu", steamId.ConvertToUint64());

		const auto& SprayBitmapLoader = [this](const char* userId, FIBITMAP* fiB) -> LOADSPRAYTEXTURE_STATUS {

			auto fiB32 = fiB;

			while (1)
			{
				auto result = Draw_LoadSprayTexture_ConvertToBGRA32(&fiB32);

				if (result > CONVERT_BGRA_OK)
					continue;

				if (result == CONVERT_BGRA_OK)
					break;

				if (result < CONVERT_BGRA_OK)
				{
					return LOAD_SPARY_FAILED_NOT_FOUND;
				}
			}

			Draw_LoadSprayTexture_BGRA8ToRGBA8(fiB32);

			size_t pos = 0;
			size_t w = FreeImage_GetWidth(fiB32);
			size_t h = FreeImage_GetHeight(fiB32);

			byte* imageData = FreeImage_GetBits(fiB32);

			if (m_pSparyBitmapJPG) {
				delete m_pSparyBitmapJPG;
				m_pSparyBitmapJPG = nullptr;
			}
			m_pSparyBitmapJPG = new vgui::MemoryBitmap(imageData, w, h);

			FreeImage_Unload(fiB32);

			return LOAD_SPARY_OK;
		};

		LOADSPRAYTEXTURE_STATUS st = LOAD_SPARY_FAILED_NOT_FOUND;

		if (bTempDecalLoaded && st == LOAD_SPARY_FAILED_NOT_FOUND)
		{
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

			snprintf(fileName, sizeof(fileName), "%s_%s.jpg", userId, wadHash);
			snprintf(filePath, sizeof(filePath), "%s/%s", CUSTOM_SPRAY_DIRECTORY, fileName);

			st = Draw_LoadSprayTexture(userId, filePath, nullptr, SprayBitmapLoader);
		}

		if (st == LOAD_SPARY_FAILED_NOT_FOUND)
		{
			//fallback to {SteamID}.jpg
			snprintf(fileName, sizeof(fileName), "%s.jpg", userId);
			snprintf(filePath, sizeof(filePath), "%s/%s", CUSTOM_SPRAY_DIRECTORY, fileName);
			st = Draw_LoadSprayTexture(userId, filePath, nullptr, SprayBitmapLoader);
		}

		if (st == LOAD_SPARY_OK && m_pSparyBitmapJPG) {
			m_pSparyImageJPG->SetImage(m_pSparyBitmapJPG);
		}
		else {
			m_pSparyImageJPG->SetImage((vgui::IImage*)nullptr);
		}
	}
}

void CBetterSpraySettingsPage::OpenLoadSparyDialog(void)
{
	int sw{};
	int sh{};
	vgui::surface()->GetScreenSize(sw, sh);

	auto pFileDialog = new vgui::FileOpenDialog(this, "#GameUI_BetterSpray_LoadSpray", vgui::FileOpenDialogType_t::FOD_OPEN, new KeyValues("FileInfo"));
	pFileDialog->SetProportional(false);
	pFileDialog->SetAutoDelete(true);
	//pFileDialog->SetStartDirectory("svencoop_downloads/");
	pFileDialog->AddFilter("*.tga;*.png;*.bmp;*.jpg;*.jpeg;*.webp", "#GameUI_BetterSpray_SparyFilter", true, "image");
	pFileDialog->Activate();
	int w{}, h{};
	pFileDialog->GetSize(w, h);
	pFileDialog->SetPos((sw - w) / 2, (sh - h) / 2);
	vgui::input()->SetAppModalSurface(pFileDialog->GetVPanel());
}

void CBetterSpraySettingsPage::OnCommand(const char* command)
{
	if (!strcmp(command, "OpenLoadSparyDialog"))
	{
		OpenLoadSparyDialog();
	}
	else if (!strcmp(command, "RefreshSpray"))
	{
		OnResetData();
	}
	else if (!stricmp(command, "OK"))
	{
		ApplyChanges();
	}
	else if (!stricmp(command, "Apply"))
	{
		ApplyChanges();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CBetterSpraySettingsPage::OnDataChanged()
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}

void CBetterSpraySettingsPage::OnFileSelected(const char* fullpath)
{
	wchar_t wszFullPath[MAX_PATH]{};
	Q_UTF8ToUnicode(fullpath, wszFullPath, MAX_PATH);

	auto fiFormat = FreeImage_GetFileTypeU(wszFullPath);

	if (!FreeImage_FIFSupportsReading(fiFormat))
		return;

	auto fiB = FreeImage_LoadU(fiFormat, wszFullPath, 0);

	if (fiB)
	{
		BS_UploadSprayBitmapArgs args;

		args.bNormalizeToSquare = true;
		args.bWithAlphaChannel = m_pWithAlpha->IsSelected() ? true : false;
		args.bAlphaInverted = m_pInvertAlpha->IsSelected() ? true : false;
		args.bRandomBackground = m_pRandomBackground->IsSelected() ? true : false;

		bool bSuccess = BS_UploadSprayBitmap(fiB, &args);

		FreeImage_Unload(fiB);

		OnResetData();

		if (bSuccess)
		{
			auto box = new vgui::MessageBox("#GameUI_BetterSpray_Title", "#GameUI_BetterSpray_SprayUploaded", this);
			box->SetOKButtonText("#GameUI_OK");
			box->SetCancelCommand(new KeyValues("Command", "command", "ReleaseModalWindow"));
			box->AddActionSignalTarget(this);
			box->DoModal();
		}
		else
		{
			auto box = new vgui::MessageBox("#GameUI_BetterSpray_Title", "#GameUI_BetterSpray_FailedToSaveImageFile", this);
			box->SetOKButtonText("#GameUI_OK");
			box->SetCancelCommand(new KeyValues("Command", "command", "ReleaseModalWindow"));
			box->AddActionSignalTarget(this);
			box->DoModal();
		}
	}
	else
	{
		auto box = new vgui::MessageBox("#GameUI_BetterSpray_Title", "#GameUI_BetterSpray_FailedToLoadImageFile", this);
		box->SetOKButtonText("#GameUI_OK");
		box->SetCancelCommand(new KeyValues("Command", "command", "ReleaseModalWindow"));
		box->AddActionSignalTarget(this);
		box->DoModal();
	}
}