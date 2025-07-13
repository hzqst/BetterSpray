#include <metahook.h>
#include <cvardef.h>
#include <IGameUI.h>
#include <vgui/VGUI.h>
#include <vgui/IPanel.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/QueryBox.h>
#include <vgui_controls/MessageMap.h>
#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/CvarSlider.h>
#include <vgui_controls/CvarToggleCheckButton.h>
#include <vgui_controls/CvarTextEntry.h>

#include <IVGUI2Extension.h>
#include "SprayDatabase.h"

#include "plugins.h"

#include "BetterSprayDialog.h"

static vgui::DHANDLE<CBetterSprayDialog> s_hBetterSprayDialog;

/*
=================================================================================================================
GameUI Callbacks
=================================================================================================================
*/

class CVGUI2Extension_GameUICallbacks : public IVGUI2Extension_GameUICallbacks
{
public:
	int GetAltitude() const override
	{
		return 0;
	}

	const char* GetControlModuleName() const
	{
		return "BetterSpray";
	}

	void Initialize(CreateInterfaceFn* factories, int count) override
	{
		if (!vgui::VGui_InitInterfacesList("BetterSpray", factories, count))
		{
			Sys_Error("Failed to VGui_InitInterfacesList");
			return;
		}
	}

	void PreStart(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion, void* system) override
	{
		if (g_pFileSystem)
		{
			if (!vgui::localize()->AddFile(g_pFileSystem, "bettersprays/gameui_%language%.txt"))
			{
				if (!vgui::localize()->AddFile(g_pFileSystem, "bettersprays/gameui_english.txt"))
				{
					//Sys_Error("Failed to load \"bettersprays/gameui_english.txt\"");
				}
			}
		}
		else if (g_pFileSystem_HL25)
		{
			if (!vgui::localize()->AddFile((IFileSystem*)g_pFileSystem_HL25, "bettersprays/gameui_%language%.txt"))
			{
				if (!vgui::localize()->AddFile((IFileSystem*)g_pFileSystem_HL25, "bettersprays/gameui_english.txt"))
				{
					//Sys_Error("Failed to load \"bettersprays/gameui_english.txt\"");
				}
			}
		}
	}

	void Start(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion, void* system) override
	{
		
	}

	void Shutdown(void) override
	{

	}

	void PostShutdown(void) override
	{

	}

	void ActivateGameUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void ActivateDemoUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void HasExclusiveInput(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void RunFrame(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void ConnectToServer(const char*& game, int& IP, int& port, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		SprayDatabase()->OnConnectToServer();
	}

	void DisconnectFromServer(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void HideGameUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void IsGameUIActive(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void LoadingStarted(const char*& resourceType, const char*& resourceName, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void LoadingFinished(const char*& resourceType, const char*& resourceName, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void StartProgressBar(const char*& progressType, int& progressSteps, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void ContinueProgressBar(int& progressPoint, float& progressFraction, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void StopProgressBar(bool& bError, const char*& failureReason, const char*& extendedReason, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void SetProgressBarStatusText(const char*& statusText, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void SetSecondaryProgressBar(float& progress, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void SetSecondaryProgressBarText(const char*& statusText, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

};

static CVGUI2Extension_GameUICallbacks s_GameUICallbacks;

/*
=================================================================================================================
GameUI OptionDialog Callbacks
=================================================================================================================
*/

class CVGUI2Extension_GameUIOptionDialogCallbacks : public IVGUI2Extension_GameUIOptionDialogCallbacks
{
public:
	int GetAltitude() const override
	{
		return 0;
	}

	void COptionsDialog_ctor(IGameUIOptionsDialogCtorCallbackContext* CallbackContext) override
	{
		
	}

	void COptionsSubVideo_ApplyVidSettings(void*& pPanel, bool& bForceRestart, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void COptionsDialogSubPage_ctor(IGameUIOptionsDialogSubPageCtorCallbackContext* CallbackContext) override
	{

	}

	void COptionsSubPage_OnApplyChanges(void*& pPanel, const char* name, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}
};

static CVGUI2Extension_GameUIOptionDialogCallbacks s_GameUIOptionDialogCallbacks;

/*
=================================================================================================================
GameUI KeyValues Callbacks
=================================================================================================================
*/

class CVGUI2Extension_KeyValuesCallbacks : public IVGUI2Extension_KeyValuesCallbacks
{
public:
	int GetAltitude() const override
	{
		return 1;
	}

	void KeyValues_LoadFromFile(void*& pthis, IFileSystem*& pFileSystem, const char*& resourceName, const char*& pathId, const char* sourceModule, VGUI2Extension_CallbackContext* CallbackContext)
	{
		if (CallbackContext->IsPost && !strcmp(resourceName, "resource/GameMenu.res")) {
			bool* pRealReturnValue = (bool*)CallbackContext->pRealReturnValue;
			if ((*pRealReturnValue) == true) {
				KeyValues* pKeyValues = (KeyValues*)pthis;
				auto name = pKeyValues->GetName();
				KeyValues* SectionQuit = nullptr;
				for (auto p = pKeyValues->GetFirstSubKey(); p; p = p->GetNextKey()) {
					auto command = p->GetString("command");
					if (!stricmp(command, "OpenOptionsDialog"))
						SectionQuit = p;
				}
				if (SectionQuit) {
					auto NameSectionQuit = SectionQuit->GetName();
					int iNameSectionQuit = atoi(NameSectionQuit);
					if (iNameSectionQuit > 0) {
						char szNewNameSectionQuit[32];
						snprintf(szNewNameSectionQuit, sizeof(szNewNameSectionQuit), "%d", iNameSectionQuit + 1);
						SectionQuit->SetName(szNewNameSectionQuit);
						char szNewNameTestButton[32];
						snprintf(szNewNameTestButton, sizeof(szNewNameTestButton), "%d", iNameSectionQuit);
						auto SectionTestButton = new KeyValues(szNewNameTestButton);
						SectionTestButton->SetString("label", "#GameUI_BetterSpray_SectionButton");
						SectionTestButton->SetString("command", "OpenBetterSprayDialog");
						pKeyValues->AddSubKeyAfter(SectionTestButton, SectionQuit);
					}
				}
			}
		}
	}
};

static CVGUI2Extension_KeyValuesCallbacks s_KeyValuesCallbacks;

class CVGUI2Extension_TaskBarCallbacks : public IVGUI2Extension_GameUITaskBarCallbacks
{
	int GetAltitude() const override
	{
		return 0;
	}

	void CTaskBar_ctor(IGameUITaskBarCtorCallbackContext* CallbackContext) override
	{
		
	}

	void CTaskBar_OnCommand(void*& pPanel, const char*& command, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		if (!strcmp(command, "OpenBetterSprayDialog")) {

			if (!s_hBetterSprayDialog)
			{
				s_hBetterSprayDialog = new CBetterSprayDialog((vgui::Panel*)pPanel, "BetterSprayDialog");
			}

			if (s_hBetterSprayDialog)
			{
				s_hBetterSprayDialog->Activate();
			}
		}
	}
};
static CVGUI2Extension_TaskBarCallbacks s_TaskBarCallbacks;

/*
=================================================================================================================
GameUI init & shutdown
=================================================================================================================
*/

void GameUI_InstallHooks(void)
{
	if (!VGUI2Extension())
		return;

	VGUI2Extension()->RegisterGameUICallbacks(&s_GameUICallbacks);
	VGUI2Extension()->RegisterKeyValuesCallbacks(&s_KeyValuesCallbacks);
	VGUI2Extension()->RegisterGameUITaskBarCallbacks(&s_TaskBarCallbacks);
}

void GameUI_UninstallHooks(void)
{
	if (!VGUI2Extension())
		return;

	VGUI2Extension()->UnregisterGameUICallbacks(&s_GameUICallbacks);
	VGUI2Extension()->UnregisterKeyValuesCallbacks(&s_KeyValuesCallbacks);
	VGUI2Extension()->UnregisterGameUITaskBarCallbacks(&s_TaskBarCallbacks);
}