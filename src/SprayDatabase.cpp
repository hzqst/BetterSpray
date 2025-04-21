#include <metahook.h>
#include "plugins.h"

#include "UtilHTTPClient.h"
#include "SprayDatabase.h"

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <format>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/HTMLparser.h>

#include <ScopeExit/ScopeExit.h>

static unsigned int g_uiAllocatedTaskId = 0;

int Draw_UploadSprayTexture(int playerindex, const char* userId, const char* fileName, const char* pathId);

class ISprayQuery : public IBaseSprayQuery
{
public:
	virtual void OnImageFileAcquired(const std::string& fileName) {}
	virtual void BuildQueryImageLink(const std::string& fileId) {}
	virtual void BuildQueryImageFile(const std::string& fileName, const std::string& actualMediaUrl) {}
};

class CUtilHTTPCallbacks : public IUtilHTTPCallbacks
{
private:
	ISprayQuery* m_pQueryTask{};

public:
	CUtilHTTPCallbacks(ISprayQuery* p) : m_pQueryTask(p)
	{

	}

	void Destroy() override
	{
		delete this;
	}

	void OnResponseComplete(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) override
	{
		if (!RequestInstance->IsRequestSuccessful())
		{
			m_pQueryTask->OnFailure();
			return;
		}

		if (ResponseInstance->IsResponseError())
		{
			m_pQueryTask->OnFailure();
			return;
		}

		auto pPayload = ResponseInstance->GetPayload();

		if (!m_pQueryTask->OnProcessPayload((const char*)pPayload->GetBytes(), pPayload->GetLength()))
		{
			m_pQueryTask->OnFailure();
			return;
		}
	}

	void OnUpdateState(UtilHTTPRequestState NewState) override
	{
		if (NewState == UtilHTTPRequestState::Finished)
		{
			m_pQueryTask->OnFinish();
		}
	}

	void OnReceiveData(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* pData, size_t cbSize) override
	{
		//Only stream request has OnReceiveData
	}
};

class CSprayQueryBase : public ISprayQuery
{
private:
	bool m_bFinished{};
	bool m_bFailed{};
	float m_flNextRetryTime{};
	unsigned int m_uiTaskId{};

protected:
	std::string m_Url;
	UtilHTTPRequestId_t m_RequestId{ UTILHTTP_REQUEST_INVALID_ID };

public:
	CSprayQueryBase()
	{
		m_uiTaskId = g_uiAllocatedTaskId;
		g_uiAllocatedTaskId++;
	}

	~CSprayQueryBase()
	{
		if (m_RequestId != UTILHTTP_REQUEST_INVALID_ID)
		{
			UtilHTTPClient()->DestroyRequestById(m_RequestId);
			m_RequestId = UTILHTTP_REQUEST_INVALID_ID;
		}
	}

	const char* GetUrl() const override
	{
		return m_Url.c_str();
	}

	bool IsFinished() const override
	{
		return m_bFinished;
	}

	bool IsFailed() const override
	{
		return m_bFailed;
	}

	SprayQueryState GetState() const override
	{
		if (m_bFailed)
			return SprayQueryState_Failed;

		if (m_bFinished)
			return SprayQueryState_Finished;

		return SprayQueryState_Querying;
	}

	unsigned int GetTaskId() const override
	{
		return m_uiTaskId;
	}

	void OnFailure() override
	{
		m_bFailed = true;
		m_flNextRetryTime = (float)gEngfuncs.GetAbsoluteTime() + 5.0f;

		SprayDatabase()->DispatchQueryStateChangeCallback(this, GetState());
	}

	void OnFinish() override
	{
		m_bFinished = true;

		SprayDatabase()->DispatchQueryStateChangeCallback(this, GetState());
	}

	void RunFrame(float flCurrentAbsTime) override
	{
		if (IsFailed() && flCurrentAbsTime > m_flNextRetryTime)
		{
			StartQuery();
		}
	}

	virtual void StartQuery()
	{
		if (m_RequestId != UTILHTTP_REQUEST_INVALID_ID)
		{
			UtilHTTPClient()->DestroyRequestById(m_RequestId);
			m_RequestId = UTILHTTP_REQUEST_INVALID_ID;
		}

		m_bFailed = false;
		m_bFinished = false;
		SprayDatabase()->DispatchQueryStateChangeCallback(this, GetState());
	}
};

class CSprayQueryImageFileTask : public CSprayQueryBase
{
public:
	std::string m_userId;
	std::string m_fileName;
	std::string m_actualMediaUrl;
	ISprayQuery* m_pQueryTaskList{};

public:
	CSprayQueryImageFileTask(ISprayQuery* parent, const std::string& userId, const std::string& fileName, const std::string& actualMediaUrl) :
		CSprayQueryBase(),
		m_pQueryTaskList(parent),
		m_userId(userId),
		m_fileName(fileName),
		m_actualMediaUrl(actualMediaUrl)
	{

	}

	void StartQuery() override
	{
		CSprayQueryBase::StartQuery();

		m_Url = m_actualMediaUrl;

		auto pRequestInstance = UtilHTTPClient()->CreateAsyncRequest(m_Url.c_str(), UtilHTTPMethod::Get, new CUtilHTTPCallbacks(this));

		if (!pRequestInstance)
		{
			CSprayQueryBase::OnFailure();
			return;
		}

		UtilHTTPClient()->AddToRequestPool(pRequestInstance);

		m_RequestId = pRequestInstance->GetRequestId();

		pRequestInstance->Send();
	}

	bool OnProcessPayload(const char* data, size_t size) override
	{
		FILESYSTEM_ANY_CREATEDIR(CUSTOM_SPRAY_DIRECTORY, "GAMEDOWNLOAD");

		std::string filePath = std::format("{0}/{1}", CUSTOM_SPRAY_DIRECTORY, m_fileName);

		auto FileHandle = FILESYSTEM_ANY_OPEN(filePath.c_str(), "wb", "GAMEDOWNLOAD");

		if (FileHandle)
		{
			FILESYSTEM_ANY_WRITE(data, size, FileHandle);
			FILESYSTEM_ANY_CLOSE(FileHandle);

			gEngfuncs.Con_DPrintf("[BetterSpray] File \"%s\" acquired!\n", m_fileName.c_str());

			m_pQueryTaskList->OnImageFileAcquired(m_fileName);
		}
		else
		{
			SprayDatabase()->UpdatePlayerSprayQueryStatus(m_userId.c_str(), SprayQueryState_Failed);

			gEngfuncs.Con_DPrintf("[BetterSpray] Could not open \"%s\" for write!\n", m_fileName.c_str());
		}

		return true;
	}

	const char* GetName() const override
	{
		return "QueryImageFile";
	}

	const char* GetIdentifier() const override
	{
		return m_fileName.c_str();
	}
};

class CSprayQueryImageLinkTask : public CSprayQueryBase
{
public:
	std::string m_userId;
	std::string m_fileId;
	ISprayQuery* m_pQueryTaskList{};

public:
	CSprayQueryImageLinkTask(ISprayQuery* parent, const std::string& userId, const std::string& fileId) :
		CSprayQueryBase(),
		m_pQueryTaskList(parent),
		m_userId(userId),
		m_fileId(fileId)
	{

	}

	void StartQuery() override
	{
		CSprayQueryBase::StartQuery();

		m_Url = std::format("https://steamcommunity.com/sharedfiles/filedetails/?id={0}&insideModal=1", m_fileId);

		auto pRequestInstance = UtilHTTPClient()->CreateAsyncRequest(m_Url.c_str(), UtilHTTPMethod::Get, new CUtilHTTPCallbacks(this));

		if (!pRequestInstance)
		{
			CSprayQueryBase::OnFailure();
			return;
		}

		UtilHTTPClient()->AddToRequestPool(pRequestInstance);

		m_RequestId = pRequestInstance->GetRequestId();

		pRequestInstance->Send();
	}

	bool OnProcessPayload(const char* data, size_t size) override
	{
		// 使用libxml解析HTML
		htmlDocPtr doc = htmlReadMemory(data, size, nullptr, nullptr, HTML_PARSE_NOWARNING | HTML_PARSE_NOERROR);
		if (!doc) {
			gEngfuncs.Con_DPrintf("[BetterSpray] Failed to parse HTML\n");

			SprayDatabase()->UpdatePlayerSprayQueryStatus(m_userId.c_str(), SprayQueryState_Failed);

			return false;
		}

		SCOPE_EXIT() { xmlFreeDoc(doc); };

		// 使用XPath查找ActualMedia图片元素
		xmlXPathContextPtr context = xmlXPathNewContext(doc);
		if (!context) {
			gEngfuncs.Con_DPrintf("[BetterSpray] Failed to create XPath context\n");

			SprayDatabase()->UpdatePlayerSprayQueryStatus(m_userId.c_str(), SprayQueryState_Failed);

			return false;
		}

		SCOPE_EXIT() { xmlXPathFreeContext(context); };

		// 查找id为ActualMedia的img元素
		xmlXPathObjectPtr result = xmlXPathEvalExpression(BAD_CAST "//img[@id='ActualMedia']", context);
		if (!result) {
			gEngfuncs.Con_DPrintf("[BetterSpray] XPath evaluation failed\n");

			SprayDatabase()->UpdatePlayerSprayQueryStatus(m_userId.c_str(), SprayQueryState_Failed);

			return false;
		}

		SCOPE_EXIT() { xmlXPathFreeObject(result); };

		if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
			gEngfuncs.Con_DPrintf("[BetterSpray] No ActualMedia image found\n");

			SprayDatabase()->UpdatePlayerSprayQueryStatus(m_userId.c_str(), SprayQueryState_Failed);

			return false;
		}

		// 获取第一个匹配节点
		xmlNodePtr node = result->nodesetval->nodeTab[0];

		// 获取src属性
		xmlChar* src = xmlGetProp(node, BAD_CAST "src");
		if (!src) {
			gEngfuncs.Con_DPrintf("[BetterSpray] No src attribute found\n");

			SprayDatabase()->UpdatePlayerSprayQueryStatus(m_userId.c_str(), SprayQueryState_Failed);

			return false;
		}

		SCOPE_EXIT() { xmlFree(src); };

		// 保存图片URL
		std::string actualMediaUrl = (const char*)src;

		gEngfuncs.Con_DPrintf("[BetterSpray] Found image URL: %s\n", actualMediaUrl.c_str());

		std::string localFileName = std::format("{0}.jpg", m_userId);

		m_pQueryTaskList->BuildQueryImageFile(localFileName, actualMediaUrl);

		return true;
	}

	const char* GetName() const override
	{
		return "QueryImageLink";
	}

	const char* GetIdentifier() const override
	{
		return m_fileId.c_str();
	}
};

class CSprayQueryTaskList : public CSprayQueryBase
{
public:
	int m_playerIndex{};
	std::string m_userId;

	std::vector<std::shared_ptr<ISprayQuery>> m_SubQueryList;

public:
	CSprayQueryTaskList(int playerIndex, const std::string& userId) :
		CSprayQueryBase(),
		m_playerIndex(playerIndex),
		m_userId(userId)
	{

	}

	bool IsFinished() const override
	{
		if (!CSprayQueryBase::IsFinished())
			return false;

		for (auto& itor : m_SubQueryList)
		{
			if (!itor->IsFinished())
				return false;
		}

		return true;
	}

	void StartQuery() override
	{
		CSprayQueryBase::StartQuery();
		
		m_Url = std::format("https://steamcommunity.com/profiles/{0}/screenshots/?appid={1}&sort=newestfirst&browsefilter=myfiles&view=grid", m_userId, gEngfuncs.pfnGetAppID());

		auto pRequestInstance = UtilHTTPClient()->CreateAsyncRequest(m_Url.c_str(), UtilHTTPMethod::Get, new CUtilHTTPCallbacks(this));

		pRequestInstance->SetFollowLocation(true);

		if (!pRequestInstance)
		{
			CSprayQueryBase::OnFailure();
			return;
		}

		UtilHTTPClient()->AddToRequestPool(pRequestInstance);

		m_RequestId = pRequestInstance->GetRequestId();

		pRequestInstance->Send();
	}

	bool OnProcessPayload(const char* data, size_t size) override
	{
		class CScreenshotFloatHelpInfo
		{
		public:
			std::string fileId;          // 从data-publishedfileid获取
			std::string imageUrl;        // 从background-image获取
			std::string description;     // 从q.ellipsis获取的文本
		};

		// 创建存储结果的vector
		std::vector<std::shared_ptr<CScreenshotFloatHelpInfo>> floatHelpList;

		// 解析HTML文档
		htmlDocPtr doc = htmlReadMemory(data, size, nullptr, "UTF-8", HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
		if (!doc) {
			gEngfuncs.Con_DPrintf("[BetterSpray] Failed to parse HTML dom.\n");
			return false;
		}
		SCOPE_EXIT() { xmlFreeDoc(doc); };

		// 创建XPath上下文
		xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
		if (!xpathCtx) {
			gEngfuncs.Con_DPrintf("[BetterSpray] Failed to create XPath context.\n");
			return false;
		}
		SCOPE_EXIT() { xmlXPathFreeContext(xpathCtx); };

		// 注册命名空间（如果需要的话）
		// xmlXPathRegisterNs(xpathCtx, BAD_CAST "ns", BAD_CAST "namespace-uri");

		// 查找所有class为floatHelp的div元素
		const xmlChar* xpathExpr = BAD_CAST "//div[@class='floatHelp']";
		xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
		SCOPE_EXIT() {
			if (xpathObj) xmlXPathFreeObject(xpathObj);
		};

		if (xpathObj && xpathObj->nodesetval) {
			xmlNodeSetPtr nodes = xpathObj->nodesetval;
			int size = (nodes) ? nodes->nodeNr : 0;

			// 遍历所有找到的div元素
			for (int i = 0; i < size; ++i) {
				auto info = std::make_shared<CScreenshotFloatHelpInfo>();
				xmlNodePtr divNode = nodes->nodeTab[i];

				// 查找a标签
				xmlNodePtr aNode = xmlFirstElementChild(divNode);
				if (aNode) {
					// 获取fileId
					xmlChar* fileId = xmlGetProp(aNode, BAD_CAST "data-publishedfileid");
					if (fileId) {
						info->fileId = (const char*)fileId;
						xmlFree(fileId);
					}

					// 查找图片div
					xmlNodePtr imgDiv = xmlFirstElementChild(aNode);
					if (imgDiv) {
						// 获取style属性
						xmlChar* style = xmlGetProp(imgDiv, BAD_CAST "style");
						if (style) {
							std::string styleStr((const char*)style);
							size_t start = styleStr.find("url('") + 5;
							size_t end = styleStr.find("')", start);
							if (start != std::string::npos && end != std::string::npos) {
								info->imageUrl = styleStr.substr(start, end - start);
							}
							xmlFree(style);
						}

						// 查找description文本
						// 使用XPath查找q标签
						xmlXPathContextPtr descCtx = xmlXPathNewContext(doc);
						SCOPE_EXIT() { xmlXPathFreeContext(descCtx); };

						descCtx->node = imgDiv;
						xmlXPathObjectPtr qObj = xmlXPathEvalExpression(BAD_CAST ".//q", descCtx);
						SCOPE_EXIT() {
							if (qObj) xmlXPathFreeObject(qObj);
						};

						if (qObj && qObj->nodesetval && qObj->nodesetval->nodeNr > 0) {
							xmlNodePtr qNode = qObj->nodesetval->nodeTab[0];
							xmlChar* content = xmlNodeGetContent(qNode);
							if (content) {
								info->description = (const char*)content;
								xmlFree(content);
							}
						}
					}
				}

				// 如果至少有fileId，则添加到列表中
				if (!info->fileId.empty() && info->description.starts_with("!BetterSpray")) {

					floatHelpList.push_back(info);

					// 调试输出
					gEngfuncs.Con_DPrintf("[BetterSpray] Found spary: fileId=%s, desc=\"%s\".\n", info->fileId.c_str(), info->description.c_str());
				}
			}
		}

		// 从floatHelpList中随机抽取一个元素并为其调用BuildQueryImageLink
		if (!floatHelpList.empty()) {
			// 生成随机索引
			size_t randomIndex = rand() % floatHelpList.size();

			// 获取随机选择的元素
			auto selectedItem = floatHelpList[randomIndex];

			// 调用BuildQueryImageLink
			gEngfuncs.Con_DPrintf("[BetterSpray] Randomly selected spary: fileId=%s\n", selectedItem->fileId.c_str());
			BuildQueryImageLink(selectedItem->fileId);
		}
		else {
			gEngfuncs.Con_DPrintf("[BetterSpray] No valid sprays found.\n");

			SprayDatabase()->UpdatePlayerSprayQueryStatus(m_userId.c_str(), SprayQueryState_Failed);
		}

		return true;
	}

	const char* GetName() const override
	{
		return "QueryTaskList";
	}

	const char* GetIdentifier() const override
	{
		return m_userId.c_str();
	}

public:

	void BuildQueryImageLink(const std::string& fileId) override
	{
		if (std::find_if(m_SubQueryList.begin(), m_SubQueryList.end(), [&fileId](const std::shared_ptr<ISprayQuery>& p) {

			if (!strcmp(p->GetName(), "QueryImageLink") &&
				!strcmp(p->GetIdentifier(), fileId.c_str()))
			{
				return true;
			}

			return false;

			}) == m_SubQueryList.end())
		{
			auto QueryInstance = std::make_shared<CSprayQueryImageLinkTask>(this, m_userId, fileId);

			QueryInstance->StartQuery();

			m_SubQueryList.emplace_back(QueryInstance);
		}
	}

	void BuildQueryImageFile(const std::string& fileName, const std::string& actualMediaUrl) override
	{
		if (std::find_if(m_SubQueryList.begin(), m_SubQueryList.end(), [&fileName](const std::shared_ptr<ISprayQuery>& p) {

			if (!strcmp(p->GetName(), "QueryImageFile") &&
				!strcmp(p->GetIdentifier(), fileName.c_str()))
			{
				return true;
			}

			return false;

			}) == m_SubQueryList.end())
		{
			auto QueryInstance = std::make_shared<CSprayQueryImageFileTask>(this, m_userId, fileName, actualMediaUrl);
			
			QueryInstance->StartQuery();

			m_SubQueryList.emplace_back(QueryInstance);
		}
	}

	void OnImageFileAcquired(const std::string &fileName) override
	{
		int result = Draw_UploadSprayTexture(-1, m_userId.c_str(), fileName.c_str(), "GAMEDOWNLOAD");

		if (result == 0)
		{
			SprayDatabase()->UpdatePlayerSprayQueryStatus(m_userId.c_str(), SprayQueryState_Finished);
		}
		else
		{
			SprayDatabase()->UpdatePlayerSprayQueryStatus(m_userId.c_str(), SprayQueryState_Failed);
		}
	}
};

class CSprayDatabase : public ISprayDatabase
{
private:
	std::vector<std::shared_ptr<ISprayQuery>> m_QueryTaskList;
	std::vector<ISprayQueryStateChangeHandler*> m_StateChangeCallbacks;
	std::unordered_map<std::string, SprayQueryState> m_UserQueryStatus{};

public:

	void Init() override
	{
		//libxml2
		xmlInitParser();
	}

	void Shutdown() override
	{
		m_QueryTaskList.clear();
		m_StateChangeCallbacks.clear();

		xmlCleanupParser();
	}

	void RunFrame() override
	{
		auto flCurrentAbsTime = (float)gEngfuncs.GetAbsoluteTime();

		for (auto itor = m_QueryTaskList.begin(); itor != m_QueryTaskList.end();)
		{
			const auto& p = (*itor);

			p->RunFrame(flCurrentAbsTime);

			if (p->IsFinished())
			{
				itor = m_QueryTaskList.erase(itor);
				continue;
			}

			itor++;
		}
	}

	/*
		Purpose: Build network query instance based on userId (steamId64)
	*/

	bool BuildQuery(int playerindex, const char* userId)
	{
		for (const auto& p : m_QueryTaskList)
		{
			if (!strcmp(p->GetName(), "QueryTaskList") &&
				!strcmp(p->GetIdentifier(), userId))
			{
				return false;
			}
		}

		auto QueryList = std::make_shared<CSprayQueryTaskList>(playerindex, userId);

		m_QueryTaskList.emplace_back(QueryList);

		QueryList->StartQuery();

		return true;
	}

	void QueryPlayerSpray(int playerindex, const char* userId) override
	{
		std::string userIdString = userId;

		auto it = m_UserQueryStatus.find(userIdString);

		if (it == m_UserQueryStatus.end())
		{
			gEngfuncs.Con_DPrintf("[BetterSpray] Querying spary for userId \"%s\"...\n", userId);

			UpdatePlayerSprayQueryStatus(userIdString.c_str(), SprayQueryState_Querying);

			BuildQuery(playerindex, userId);
		}
	}

	SprayQueryState GetPlayerSprayQueryStatus(const char* userId) const override
	{
		std::string userIdString = userId;

		auto it = m_UserQueryStatus.find(userIdString);

		if (it != m_UserQueryStatus.end())
		{
			return it->second;
		}

		return SprayQueryState_Unknown;
	}

	void UpdatePlayerSprayQueryStatus(const char *userId, SprayQueryState newQueryStatus) override
	{
		std::string userIdString = userId;

		auto it = m_UserQueryStatus.find(userId);

		if (it != m_UserQueryStatus.end())
		{
			it->second = newQueryStatus;
		}
		else
		{
			m_UserQueryStatus[userIdString] = newQueryStatus;
		}
	}

	void EnumQueries(IEnumSprayQueryHandler* handler) override
	{
		for (const auto& p : m_QueryTaskList)
		{
			handler->OnEnumQuery(p.get());
		}
	}

	void RegisterQueryStateChangeCallback(ISprayQueryStateChangeHandler* handler) override
	{
		auto itor = std::find_if(m_StateChangeCallbacks.begin(), m_StateChangeCallbacks.end(), [handler](ISprayQueryStateChangeHandler* it) {
			return it == handler;
		});

		if (itor == m_StateChangeCallbacks.end())
		{
			m_StateChangeCallbacks.emplace_back(handler);
		}
	}

	void UnregisterQueryStateChangeCallback(ISprayQueryStateChangeHandler* handler) override
	{
		auto itor = std::find_if(m_StateChangeCallbacks.begin(), m_StateChangeCallbacks.end(), [handler](ISprayQueryStateChangeHandler* it) {
			return it == handler;
			});

		if (itor != m_StateChangeCallbacks.end())
		{
			m_StateChangeCallbacks.erase(itor);
		}
	}

	void DispatchQueryStateChangeCallback(IBaseSprayQuery* pQuery, SprayQueryState newState) override
	{
		for (auto callback : m_StateChangeCallbacks)
		{
			callback->OnQueryStateChanged(pQuery, newState);
		}
	}
};

static CSprayDatabase s_SprayDatabase;

ISprayDatabase* SprayDatabase()
{
	return &s_SprayDatabase;
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CSprayDatabase, ISprayDatabase, SPARY_DATABASE_INTERFACE_VERSION, s_SprayDatabase)