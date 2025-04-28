#pragma once

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>

class WadFile;

class CBetterSpraySettingsPage : public vgui::PropertyPage
{
public:
	DECLARE_CLASS_SIMPLE(CBetterSpraySettingsPage, vgui::PropertyPage);

	CBetterSpraySettingsPage(vgui::Panel* parent, const char* name);
	~CBetterSpraySettingsPage();

private:

	void OnCommand(const char* command) override;
	void ApplyChanges(void);
	void ApplyChangesToConVar(const char* pConVarName, int value);
	void OpenLoadSparyDialog(void);

	MESSAGE_FUNC(OnApplyChanges, "ApplyChanges");
	MESSAGE_FUNC(OnResetData, "ResetData");
	MESSAGE_FUNC(OnDataChanged, "ControlModified");
	MESSAGE_FUNC_CHARPTR(OnFileSelected, "FileSelected", fullpath);

	typedef vgui::PropertyPage BaseClass;
private:
	vgui::HFont m_hFont{};

	vgui::CheckButton* m_pRandomBackground{};
	vgui::CheckButton* m_pWithAlpha{};
	vgui::CheckButton* m_pInvertAlpha{};
	vgui::Button* m_pLoadSpary{};
	vgui::Button* m_pRefreshSpray{};

	vgui::ImagePanel* m_pSparyImageJPG{};
	vgui::ImagePanel* m_pSparyImageWAD{};

	vgui::IImage* m_pSparyBitmapJPG{};
	vgui::IImage* m_pSparyBitmapWAD{};
	WadFile* m_pSparyWad{};
};
