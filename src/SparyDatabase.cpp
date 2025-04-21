#include <metahook.h>
#include "plugins.h"

#include "UtilHTTPClient.h"
#include "SparyDatabase.h"

#include <string>
#include <unordered_map>
#include <algorithm>
#include <format>

#include "../thirdparty/tinyxml2/tinyxml2.h"

class CSteamScreenshotFloatHelpInfo
{
public:
	std::string fileId;          // 从data-publishedfileid获取
	std::string imageUrl;        // 从background-image获取
	std::string description;     // 从q.ellipsis获取的文本
};

static unsigned int g_uiAllocatedTaskId = 0;

class CUtilHTTPCallbacks : public IUtilHTTPCallbacks
{
private:
	ISparyQuery* m_pQueryTask{};

public:
	CUtilHTTPCallbacks(ISparyQuery* p) : m_pQueryTask(p)
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

class CSparyQueryBase : public ISparyQuery
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
	CSparyQueryBase()
	{
		m_uiTaskId = g_uiAllocatedTaskId;
		g_uiAllocatedTaskId++;
	}

	~CSparyQueryBase()
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

	SparyQueryState GetState() const override
	{
		if (m_bFailed)
			return SparyQueryState_Failed;

		if (m_bFinished)
			return SparyQueryState_Finished;

		return SparyQueryState_Querying;
	}

	unsigned int GetTaskId() const override
	{
		return m_uiTaskId;
	}

	void OnFailure() override
	{
		m_bFailed = true;
		m_flNextRetryTime = (float)gEngfuncs.GetAbsoluteTime() + 5.0f;

		SparyDatabase()->DispatchQueryStateChangeCallback(this, GetState());
	}

	void OnFinish() override
	{
		m_bFinished = true;

		SparyDatabase()->DispatchQueryStateChangeCallback(this, GetState());
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
		SparyDatabase()->DispatchQueryStateChangeCallback(this, GetState());
	}
};

class CSparyQueryTaskList;

class CSparyQueryImageLinkTask : public CSparyQueryBase
{
public:
	std::string m_fileId;
	CSparyQueryTaskList* m_pQueryTaskList;

public:
	CSparyQueryImageLinkTask(CSparyQueryTaskList* parent, const std::string& fileId) :
		CSparyQueryBase(),
		m_pQueryTaskList(parent),
		m_fileId(fileId)
	{

	}

	void StartQuery() override
	{
		CSparyQueryBase::StartQuery();

		m_Url = std::format("https://steamcommunity.com/sharedfiles/filedetails/?id={0}&insideModal=1", m_fileId);

		auto pRequestInstance = UtilHTTPClient()->CreateAsyncRequest(m_Url.c_str(), UtilHTTPMethod::Get, new CUtilHTTPCallbacks(this));

		if (!pRequestInstance)
		{
			CSparyQueryBase::OnFailure();
			return;
		}

		UtilHTTPClient()->AddToRequestPool(pRequestInstance);

		m_RequestId = pRequestInstance->GetRequestId();

		pRequestInstance->Send();
	}

	bool OnProcessPayload(const char* data, size_t size) override
	{

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

class CSparyQueryImageFileTask : public CSparyQueryBase
{
public:
	std::string m_localFileName;
	std::string m_networkFileName;
	CSparyQueryTaskList* m_pQueryTaskList;

public:
	CSparyQueryImageFileTask(CSparyQueryTaskList* parent, const std::string& localFileName, const std::string& networkFileName) :
		CSparyQueryBase(),
		m_pQueryTaskList(parent),
		m_localFileName(localFileName),
		m_networkFileName(networkFileName)
	{

	}

	void StartQuery() override;

	bool OnProcessPayload(const char* data, size_t size) override;

	const char* GetName() const override
	{
		return "QueryImageFileTask";
	}

	const char* GetIdentifier() const override
	{
		return m_localFileName.c_str();
	}
};

class CSparyQueryTaskList : public CSparyQueryBase
{
public:
	int m_playerIndex{};
	std::string m_userId;
	bool m_bReloaded{};

	std::vector<std::shared_ptr<ISparyQuery>> m_SubQueryList;

public:
	CSparyQueryTaskList(int playerIndex, const std::string& userId) :
		CSparyQueryBase(),
		m_playerIndex(playerIndex),
		m_userId(userId)
	{

	}

	bool IsFinished() const override
	{
		if (!CSparyQueryBase::IsFinished())
			return false;

		for (auto &itor : m_SubQueryList)
		{
			if (!itor->IsFinished())
				return false;
		}

		return true;
	}

	void StartQuery() override
	{
		CSparyQueryBase::StartQuery();

		//https://steamcommunity.com/profiles/76561197985557911/screenshots/?appid=0&sort=newestfirst&browsefilter=myfiles&view=grid


		//https://steamcommunity.com/sharedfiles/filedetails/?id=3150988426&insideModal=1

		/*

			<link rel="image_src" href="https://images.steamusercontent.com/ugc/2343629042955038684/25220472841ADF3109D791055CD6131053EDF085/?imw=512&amp;imh=216&amp;ima=fit&amp;impolicy=Letterbox&amp;imcolor=%23000000&amp;letterbox=true">
		<meta property="og:image" content="https://images.steamusercontent.com/ugc/2343629042955038684/25220472841ADF3109D791055CD6131053EDF085/?imw=512&amp;imh=216&amp;ima=fit&amp;impolicy=Letterbox&amp;imcolor=%23000000&amp;letterbox=true">
		<meta name="twitter:image" content="https://images.steamusercontent.com/ugc/2343629042955038684/25220472841ADF3109D791055CD6131053EDF085/?imw=512&amp;imh=216&amp;ima=fit&amp;impolicy=Letterbox&amp;imcolor=%23000000&amp;letterbox=true" />

		*/

		m_Url = std::format("https://steamcommunity.com/profiles/{0}/screenshots/?appid=225840&sort=newestfirst&browsefilter=myfiles&view=grid", m_userId);

		auto pRequestInstance = UtilHTTPClient()->CreateAsyncRequest(m_Url.c_str(), UtilHTTPMethod::Get, new CUtilHTTPCallbacks(this));
		
		pRequestInstance->SetFollowLocation(true);

		if (!pRequestInstance)
		{
			CSparyQueryBase::OnFailure();
			return;
		}

		UtilHTTPClient()->AddToRequestPool(pRequestInstance);

		m_RequestId = pRequestInstance->GetRequestId();

		pRequestInstance->Send();
	}

	void BuildQueryInternal(const std::string& fileId)
	{
		if (std::find_if(m_SubQueryList.begin(), m_SubQueryList.end(), [&fileId](const std::shared_ptr<ISparyQuery>& p) {

			if (!strcmp(p->GetName(), "QueryImageLink") &&
				!strcmp(p->GetIdentifier(), fileId.c_str()))
			{
				return true;
			}

			return false;

		}) == m_SubQueryList.end())
		{
			auto QueryInstance = std::make_shared<CSparyQueryImageLinkTask>(this, fileId);

			QueryInstance->StartQuery();

			m_SubQueryList.emplace_back(QueryInstance);
		}
	}

	bool OnProcessPayload(const char* data, size_t size) override
	{
		// 创建存储结果的vector
		std::vector<std::shared_ptr<CSteamScreenshotFloatHelpInfo>> floatHelpList;

		// 创建XML文档对象
		tinyxml2::XMLDocument doc;

		// 提取<body>和</body>之间的内容
		std::string htmlContent(data, size);
		std::string bodyContent;
		
		size_t bodyStart = htmlContent.find("<body");
		size_t bodyEnd = htmlContent.find("</body>");
		
		if (bodyStart != std::string::npos && bodyEnd != std::string::npos) {
			// 找到<body>标签的结束位置
			size_t bodyTagEnd = htmlContent.find(">", bodyStart);
			if (bodyTagEnd != std::string::npos) {
				// 提取<body>和</body>之间的内容
				bodyContent = htmlContent.substr(bodyTagEnd + 1, bodyEnd - bodyTagEnd - 1);
			}
		} else {
			// 如果找不到body标签，使用原始内容
			bodyContent = htmlContent;
		}

		// 包装HTML内容以便解析
		std::string wrappedHtml = "<root>";
		wrappedHtml.append(bodyContent);
		wrappedHtml += "</root>";

		// 解析HTML
		if (doc.Parse(wrappedHtml.c_str(), wrappedHtml.length()) != tinyxml2::XML_SUCCESS) {

			gEngfuncs.Con_DPrintf("[BetterSpary] HTML parse error: %s\n", doc.ErrorStr());
			return false;
		}

		// 获取根节点
		tinyxml2::XMLElement* root = doc.RootElement();
		if (!root) {
			return false;
		}

		// 遍历所有div元素
		for (tinyxml2::XMLElement* divElement = root->FirstChildElement("div");
			divElement;
			divElement = divElement->NextSiblingElement("div"))
		{
			// 检查class是否为floatHelp
			const char* classAttr = divElement->Attribute("class");
			if (!classAttr || strcmp(classAttr, "floatHelp") != 0) {
				continue;
			}

			auto info = std::make_shared<CSteamScreenshotFloatHelpInfo>();

			// 获取a标签
			if (tinyxml2::XMLElement* aElement = divElement->FirstChildElement("a")) {
				// 获取fileId
				const char* publishedId = aElement->Attribute("data-publishedfileid");
				if (publishedId) {
					info->fileId = publishedId;
				}

				// 获取图片div
				if (tinyxml2::XMLElement* imgDiv = aElement->FirstChildElement("div")) {
					const char* style = imgDiv->Attribute("style");
					if (style) {
						// 从style属性中提取图片URL
						std::string styleStr(style);
						size_t start = styleStr.find("url('") + 5;
						size_t end = styleStr.find("')", start);
						if (start != std::string::npos && end != std::string::npos) {
							info->imageUrl = styleStr.substr(start, end - start);
						}
					}

					// 查找description文本
					tinyxml2::XMLElement* descDiv = imgDiv->FirstChildElement("div");
					while (descDiv) {
						if (tinyxml2::XMLElement* hoverDiv = descDiv->FirstChildElement("div")) {
							if (tinyxml2::XMLElement* descriptionDiv = hoverDiv->FirstChildElement("div")) {
								if (tinyxml2::XMLElement* qElement = descriptionDiv->FirstChildElement("q")) {
									const char* text = qElement->GetText();
									if (text) {
										info->description = text;
									}
								}
							}
						}
						descDiv = descDiv->NextSiblingElement("div");
					}
				}
			}

			// 如果至少有fileId，则添加到列表中
			if (!info->fileId.empty()) {
				floatHelpList.push_back(info);

				// 调试输出
				gEngfuncs.Con_DPrintf("[BetterSpary] Found screenshot: id=%s, desc=%s\n",
					info->fileId.c_str(),
					info->description.c_str());
			}
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
};

void CSparyQueryImageFileTask::StartQuery()
{
	

}

bool CSparyQueryImageFileTask::OnProcessPayload(const char* data, size_t size)
{
	gEngfuncs.Con_DPrintf("[BetterSpary] File \"%s\" acquired!\n", m_localFileName.c_str());

#if 0

	FILESYSTEM_ANY_CREATEDIR("custom_sprays", "GAMEDOWNLOAD");

	std::string filePathDir = std::format("custom_sprays/{0}", m_pQueryTaskList->m_localFileNameBase);
	FILESYSTEM_ANY_CREATEDIR(filePathDir.c_str(), "GAMEDOWNLOAD");

	std::string filePath = std::format("models/player/{0}/{1}", m_pQueryTaskList->m_localFileNameBase, m_localFileName);
	auto FileHandle = FILESYSTEM_ANY_OPEN(filePath.c_str(), "wb", "GAMEDOWNLOAD");

	if (FileHandle)
	{
		FILESYSTEM_ANY_WRITE(data, size, FileHandle);
		FILESYSTEM_ANY_CLOSE(FileHandle);
	}

	m_pQueryTaskList->OnModelFileWriteFinished();
#endif
	return true;
}

typedef struct SparyTextureStorage_s
{
	int gltexturenum{};
	char userId[64]{};
}SparyTextureStorage_t;

class CSparyDatabase : public ISparyDatabase
{
private:
	std::vector<std::shared_ptr<ISparyQuery>> m_QueryTaskList;
	std::vector<ISparyQueryStateChangeHandler*> m_StateChangeCallbacks;
	SparyTextureStorage_t m_SparyTextures[32]{};

public:

	void Init() override
	{
		//do nothing
	}

	void Shutdown() override
	{
		m_QueryTaskList.clear();
		m_StateChangeCallbacks.clear();
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
		Purpose: Build network query instance
	*/

	bool BuildQueryListInternal(int playerindex, const std::string& userId)
	{
		for (const auto& p : m_QueryTaskList)
		{
			if (!strcmp(p->GetName(), "QueryTaskList") &&
				!strcmp(p->GetIdentifier(), userId.c_str()) )
			{
				return false;
			}
		}

		auto QueryList = std::make_shared<CSparyQueryTaskList>(playerindex, userId);

		m_QueryTaskList.emplace_back(QueryList);

		QueryList->StartQuery();

		return true;
	}

	/*
		Purpose: Build network query instance based on userId (steamId64)
	*/

	bool BuildQueryList(int playerindex, const char* userId)
	{
		return BuildQueryListInternal(playerindex, userId);
	}

	int FindSparyTextureId(int playerindex, const char* userId) override
	{
		if(!strcmp(m_SparyTextures[playerindex].userId, userId))
			return m_SparyTextures[playerindex].gltexturenum;

		return 0;
	}

	void QueryPlayerSpary(int playerindex, const char* userId) override
	{
		gEngfuncs.Con_DPrintf("[BetterSpary] Downloading spary for userId \"%s\"...\n", playerindex, userId);

		BuildQueryList(playerindex, userId);
	}

	void EnumQueries(IEnumSparyQueryHandler* handler) override
	{
		for (const auto& p : m_QueryTaskList)
		{
			handler->OnEnumQuery(p.get());
		}
	}

	void RegisterQueryStateChangeCallback(ISparyQueryStateChangeHandler* handler) override
	{
		auto itor = std::find_if(m_StateChangeCallbacks.begin(), m_StateChangeCallbacks.end(), [handler](ISparyQueryStateChangeHandler* it) {
			return it == handler;
		});

		if (itor == m_StateChangeCallbacks.end())
		{
			m_StateChangeCallbacks.emplace_back(handler);
		}
	}

	void UnregisterQueryStateChangeCallback(ISparyQueryStateChangeHandler* handler) override
	{
		auto itor = std::find_if(m_StateChangeCallbacks.begin(), m_StateChangeCallbacks.end(), [handler](ISparyQueryStateChangeHandler* it) {
			return it == handler;
			});

		if (itor != m_StateChangeCallbacks.end())
		{
			m_StateChangeCallbacks.erase(itor);
		}
	}

	void DispatchQueryStateChangeCallback(ISparyQuery* pQuery, SparyQueryState newState) override
	{
		for (auto callback : m_StateChangeCallbacks)
		{
			callback->OnQueryStateChanged(pQuery, newState);
		}
	}
};

static CSparyDatabase s_SparyDatabase;

ISparyDatabase* SparyDatabase()
{
	return &s_SparyDatabase;
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CSparyDatabase, ISparyDatabase, SPARY_DATABASE_INTERFACE_VERSION, s_SparyDatabase)