#include "BetterSprayDialog.h"
#include "BetterSpraySettingsPage.h"
#include "TaskListPage.h"

#include "exportfuncs.h"

CBetterSprayDialog::CBetterSprayDialog(vgui::Panel* parent, const char *name) :
	BaseClass(parent, name)
{
	SetDeleteSelfOnClose(true);

	SetTitle("#GameUI_BetterSpray_Title", false);

	m_pBetterSpraySettingsPage = new CBetterSpraySettingsPage(this, "BetterSpraySettingsPage");
	m_pBetterSpraySettingsPage->MakeReadyForUse();

	m_pTaskListPage = new CTaskListPage(this, "TaskListPage");
	m_pTaskListPage->MakeReadyForUse();

	SetMinimumSize(640, 384);
	SetSize(640, 384);

	m_pTabPanel = new vgui::PropertySheet(this, "Tabs");
	m_pTabPanel->SetTabWidth(72);
	m_pTabPanel->AddPage(m_pBetterSpraySettingsPage, "#GameUI_BetterSpray_SettingsPage");
	m_pTabPanel->AddPage(m_pTaskListPage, "#GameUI_BetterSpray_TaskListPage");

	m_pTabPanel->AddActionSignalTarget(this);

	LoadControlSettings("bettersprays/BetterSprayDialog.res", "GAME");

	m_pTabPanel->SetActivePage(m_pBetterSpraySettingsPage);

	vgui::ivgui()->AddTickSignal(GetVPanel());
}

CBetterSprayDialog::~CBetterSprayDialog()
{

}

void CBetterSprayDialog::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{
		m_pTabPanel->ApplyChanges();
		Close();
		return;
	}
	else if (!stricmp(command, "Apply"))
	{
		m_pTabPanel->ApplyChanges();
		return;
	}

	BaseClass::OnCommand(command);
}