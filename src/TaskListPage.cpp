#include "TaskListPage.h"
#include "TaskListPanel.h"

#include "exportfuncs.h"

CTaskListPage::CTaskListPage(vgui::Panel* parent, const char* name) :
	BaseClass(parent, name)
{
	SetSize(vgui::scheme()->GetProportionalScaledValue(624), vgui::scheme()->GetProportionalScaledValue(300));

	m_pTaskListPanel = new CTaskListPanel(this, "TaskListPanel");

	LoadControlSettings("bettersprays/TaskListPage.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());
	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());

	SprayDatabase()->RegisterQueryStateChangeCallback(this);
}

CTaskListPage::~CTaskListPage()
{
	SprayDatabase()->UnregisterQueryStateChangeCallback(this);
}

void CTaskListPage::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == vgui::KEY_ENTER)
	{
		if (m_pTaskListPanel->HasFocus() && m_pTaskListPanel->GetSelectedItemsCount() > 0)
		{
			return;
		}
	}

	BaseClass::OnKeyCodeTyped(code);
}

const char * UTIL_GetQueryStateName(SprayQueryState state)
{
	/*
	SprayQueryState_Unknown = 0,
	SprayQueryState_Querying,
	SprayQueryState_Receiving,
	SprayQueryState_Failed,
	SprayQueryState_Finished,
	*/
	const char* s_QueryStateName[] = {
		"Unknown",
		"Querying",
		"Receiving",
		"Failed",
		"Finished"
	};

	if (state >= 0 && state < _ARRAYSIZE(s_QueryStateName))
	{
		return s_QueryStateName[state];
	}

	return "Unknown";
}

void CTaskListPage::AddQueryItem(ISprayQuery* pQuery)
{
	auto kv = new KeyValues("TaskItem");

	kv->SetInt("taskId", pQuery->GetTaskId());
	kv->SetString("name", pQuery->GetName());
	kv->SetString("identifier", pQuery->GetIdentifier());
	kv->SetString("url", pQuery->GetUrl());
	kv->SetString("state", UTIL_GetQueryStateName(pQuery->GetState()));

	m_pTaskListPanel->AddItem(kv, pQuery->GetTaskId(), false, true);

	kv->deleteThis();
}

void CTaskListPage::OnEnumQuery(ISprayQuery* pQuery)
{
	AddQueryItem(pQuery);
}

void CTaskListPage::OnQueryStateChanged(ISprayQuery* pQuery, SprayQueryState newState)
{
	for (int i = 0; i < m_pTaskListPanel->GetItemCount(); ++i)
	{
		auto userData = m_pTaskListPanel->GetItemUserData(i);

		if (userData == pQuery->GetTaskId())
		{
			m_pTaskListPanel->RemoveItem(i);

			break;
		}
	}

	AddQueryItem(pQuery);
}

void CTaskListPage::OnResetData()
{
	OnRefreshTaskList();
}

void CTaskListPage::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("ListSmall", IsProportional());

	if (!m_hFont)
		m_hFont = pScheme->GetFont("DefaultSmall", IsProportional());

	m_pTaskListPanel->SetFont(m_hFont);
}

void CTaskListPage::OnRefreshTaskList()
{
	m_pTaskListPanel->RemoveAll();

	SprayDatabase()->EnumQueries(this);
}