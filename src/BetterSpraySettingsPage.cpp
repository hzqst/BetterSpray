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

	m_pLoadSpary = new vgui::Button(this, "LoadSpary", "#GameUI_BetterSpray_Load", this, "OpenLoadSparyDialog");
	m_pRefreshSpray = new vgui::Button(this, "RefreshSpray", "#GameUI_BetterSpray_Refresh", this, "RefreshSpray");
	m_pInvertAlpha = new vgui::CheckButton(this, "InvertAlpha", "#GameUI_BetterSpray_InvertAlpha");

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
	if (m_pSparyWad)
	{
		m_pSparyWad->Load("tempdecal.wad");
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
	}

	if (1)
	{
		auto steamId = SteamUser()->GetSteamID();
		auto userId = std::format("{0}", steamId.ConvertToUint64());

		std::string fileName = std::format("{0}.jpg", userId);

		std::string filePath = std::format("{0}/{1}", CUSTOM_SPRAY_DIRECTORY, fileName);

		auto SprayBitmapLoader = [this](const char* userId, FIBITMAP* fiB) -> int {

			auto fiB32 = fiB;

			Draw_LoadSprayTexture_ConvertToBGRA32(&fiB32);
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

			return 0;
		};

		auto result = Draw_LoadSprayTexture(userId.c_str(), filePath.c_str(), NULL, SprayBitmapLoader);

		//For "GAMEDOWNLOAD"
		//if (result == -1) {
		//	result = Draw_LoadSprayTexture(userId.c_str(), filePath.c_str(), NULL, SprayBitmapLoader);
		//}

		if (result == 0 && m_pSparyBitmapJPG) {
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

#ifdef MessageBox
#undef MessageBox
#endif
	auto fiB = FreeImage_LoadU(fiFormat, wszFullPath, 0);
	if (fiB)
	{
		auto bInvertedAlpha = m_pInvertAlpha->IsSelected() ? true : false;

		bool bSuccess = BS_UploadSprayBitmap(fiB, true, true, bInvertedAlpha);
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