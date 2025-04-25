#pragma once

#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Menu.h>

class CTaskListPage;
class CBetterSpraySettingsPage;

class CBetterSprayDialog : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CBetterSprayDialog, vgui::Frame);

	CBetterSprayDialog(vgui::Panel *parent, const char* name);
	~CBetterSprayDialog();

private:

	void OnCommand(const char* command) override;

	typedef vgui::Frame BaseClass;

	CBetterSpraySettingsPage* m_pBetterSpraySettingsPage{};
	CTaskListPage* m_pTaskListPage{};
	vgui::PropertySheet* m_pTabPanel{};
};