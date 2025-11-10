#include <metahook.h>
#include "plugins.h"

#include "UtilHTTPClient.h"
#include "UtilThreadTask.h"
#include "SprayDatabase.h"
#include "exportfuncs.h"

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

bool Draw_ValidateImageFormatMemoryIO(const void* data, size_t dataSize, const char* identifier);

static int UTIL_GetContentLength(IUtilHTTPResponse* ResponseInstance)
{
	char szContentLength[32]{};
	if (ResponseInstance->GetHeader("Content-Length", szContentLength, sizeof(szContentLength) - 1) && szContentLength[0])
	{
		return atoi(szContentLength);
	}

	return -1;
}

class ISprayQueryInternal : public ISprayQuery
{
public:
	virtual void AddRef() = 0;
	virtual void Release() = 0;

	virtual void OnResponding(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) = 0;
	virtual void OnFinish(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) = 0;
	virtual bool OnStreamComplete(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) = 0;
	virtual bool OnProcessPayload(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) = 0;
	virtual void OnReceiveChunk(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) = 0;
	virtual void OnFailure() = 0;

	virtual void RunFrame(float flCurrentAbsTime) = 0;
	virtual void StartQuery() = 0;

	virtual void OnProcessPayloadWorkItem(void* context) {};//This always run in thread pool !!!
	virtual void OnProcessPayloadWorkItemComplete(void* context) {};
};

class ISprayDatabaseInternal : public ISprayDatabase
{
public:
	virtual void BuildQueryImageLink(const std::string& userId, const std::string& fileId) = 0;
	virtual void BuildQueryImageFile(const std::string& userId, const std::string& fileName, const std::string& actualMediaUrl) = 0;
	virtual void OnImageFileAcquired(const std::string& userId, const std::string& filePath) = 0;
	virtual void UpdatePlayerSprayQueryStatusInternal(const std::string& userId, SprayQueryState newQueryStatus) = 0;
	virtual void DispatchQueryStateChangeCallback(ISprayQuery* pQuery, SprayQueryState newState) = 0;

};

ISprayDatabaseInternal* SprayDatabaseInternal();

template<typename T>
class AutoPtr {
private:
	T* m_ptr;

public:
	AutoPtr() : m_ptr(nullptr) {}
	
	explicit AutoPtr(T* ptr) : m_ptr(ptr) {
		if (m_ptr) {
			m_ptr->AddRef();
		}
	}

	AutoPtr(const AutoPtr& other) : m_ptr(other.m_ptr) {
		if (m_ptr) {
			m_ptr->AddRef();
		}
	}

	AutoPtr(AutoPtr&& other) noexcept : m_ptr(other.m_ptr) {
		other.m_ptr = nullptr;
	}

	~AutoPtr() {
		if (m_ptr) {
			m_ptr->Release();
		}
	}

	AutoPtr& operator=(const AutoPtr& other) {
		if (this != &other) {
			if (m_ptr) {
				m_ptr->Release();
			}
			m_ptr = other.m_ptr;
			if (m_ptr) {
				m_ptr->AddRef();
			}
		}
		return *this;
	}

	AutoPtr& operator=(AutoPtr&& other) noexcept {
		if (this != &other) {
			if (m_ptr) {
				m_ptr->Release();
			}
			m_ptr = other.m_ptr;
			other.m_ptr = nullptr;
		}
		return *this;
	}

	T* operator->() const { return m_ptr; }
	T& operator*() const { return *m_ptr; }
	operator T*() const { return m_ptr; }
	
	T* Get() const { return m_ptr; }
	
	void Reset(T* ptr = nullptr) {
		if (m_ptr) {
			m_ptr->Release();
		}
		m_ptr = ptr;
		if (m_ptr) {
			m_ptr->AddRef();
		}
	}

	T* Detach() {
		T* ptr = m_ptr;
		m_ptr = nullptr;
		return ptr;
	}
};

class CScreenshotFloatHelpInfo
{
public:
	std::string fileId;          // 从data-publishedfileid获取
	std::string imageUrl;        // 从background-image获取
	std::string description;     // 从q.ellipsis获取的文本
};

class CThreadedWorkItemContext : public IThreadedTask
{
public:
	CThreadedWorkItemContext(ISprayQueryInternal* p, const char* data, size_t size) :
		pthis(p),
		payload(data, size)
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
		pthis->OnProcessPayloadWorkItemComplete(this);
	}

public:
	AutoPtr<ISprayQueryInternal> pthis;
	std::string payload;
	std::string errorMessage;
};

class CSprayQueryTaskListWorkItemContext : public CThreadedWorkItemContext
{
public:
	CSprayQueryTaskListWorkItemContext(ISprayQueryInternal* p, const char* data, size_t size) :
		CThreadedWorkItemContext(p, data, size)
	{
	}

public:
	std::vector<std::shared_ptr<CScreenshotFloatHelpInfo>> floatHelpList;
};

class CSprayQueryImageLinkWorkItemContext : public CThreadedWorkItemContext
{
public:
	CSprayQueryImageLinkWorkItemContext(ISprayQueryInternal* p, const char* data, size_t size) :
		CThreadedWorkItemContext(p, data, size)
	{
	}

public:
	std::string actualMediaUrl;
};

class CUtilHTTPCallbacks : public IUtilHTTPCallbacks
{
private:
	//No AutoPtr, because each ISprayQuery has their own IUtilHTTPCallbacks
	ISprayQueryInternal* m_pQueryTask{};

public:
	CUtilHTTPCallbacks(ISprayQueryInternal* p) : m_pQueryTask(p) {}

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

		if (!RequestInstance->IsStream())
		{
			auto pPayload = ResponseInstance->GetPayload();

			if (!m_pQueryTask->OnProcessPayload(RequestInstance, ResponseInstance, (const void*)pPayload->GetBytes(), pPayload->GetLength()))
			{
				m_pQueryTask->OnFailure();
				return;
			}
		}
		else
		{
			if (!m_pQueryTask->OnStreamComplete(RequestInstance, ResponseInstance))
			{
				m_pQueryTask->OnFailure();
				return;
			}
		}
	}

	void OnUpdateState(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, UtilHTTPRequestState NewState) override
	{
		if (NewState == UtilHTTPRequestState::Responding)
		{
			m_pQueryTask->OnResponding(RequestInstance, ResponseInstance);
		}
		if (NewState == UtilHTTPRequestState::Finished)
		{
			m_pQueryTask->OnFinish(RequestInstance, ResponseInstance);
		}
	}

	void OnReceiveData(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* pData, size_t cbSize) override
	{
		//Only stream request has OnReceiveData
		m_pQueryTask->OnReceiveChunk(RequestInstance, ResponseInstance, pData, cbSize);
	}
};

class CSprayQueryBase : public ISprayQueryInternal
{
private:
	volatile long m_RefCount{};
	bool m_bResponding{};
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
		m_RefCount = 1;
		m_uiTaskId = g_uiAllocatedTaskId;
		g_uiAllocatedTaskId++;
	}

	~CSprayQueryBase()
	{
#ifdef _DEBUG
		gEngfuncs.Con_DPrintf("CSprayQuery: deleting query \"%s\"\n", m_Url.c_str());
#endif
		if (m_RequestId != UTILHTTP_REQUEST_INVALID_ID)
		{
			UtilHTTPClient()->DestroyRequestById(m_RequestId);
			m_RequestId = UTILHTTP_REQUEST_INVALID_ID;
		}
	}

	void AddRef() override {
		InterlockedIncrement(&m_RefCount);
	}

	void Release() override {
		if (InterlockedDecrement(&m_RefCount) == 0) {
			delete this;
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

	bool NeedRetry() const override
	{
		return m_flNextRetryTime > 0;
	}

	SprayQueryState GetState() const override
	{
		if (m_bFailed)
			return SprayQueryState_Failed;

		if (m_bFinished)
			return SprayQueryState_Finished;

		if (m_bResponding)
			return SprayQueryState_Receiving;

		return SprayQueryState_Querying;
	}

	unsigned int GetTaskId() const override
	{
		return m_uiTaskId;
	}

	void OnResponding(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) override
	{
		m_bResponding = true;

		SprayDatabaseInternal()->DispatchQueryStateChangeCallback(this, GetState());
	}

	void OnFailure() override
	{
		m_bFailed = true;
		m_flNextRetryTime = (float)gEngfuncs.GetAbsoluteTime() + 5.0f;

		SprayDatabaseInternal()->DispatchQueryStateChangeCallback(this, GetState());
	}

	void OnFinish(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) override
	{
		m_bFinished = true;
		m_bResponding = false;

		SprayDatabaseInternal()->DispatchQueryStateChangeCallback(this, GetState());
	}

	bool OnProcessPayload(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) override
	{
		auto nContentLength = UTIL_GetContentLength(ResponseInstance);

		if (nContentLength >= 0 && size < nContentLength)
		{
			gEngfuncs.Con_Printf("[BetterSpray] Content-Length mismatch for \"%s\": expect %d , got %d !\n", m_Url.c_str(), nContentLength, size);
			return false;
		}

		return true;
	}

	bool OnStreamComplete(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance) override
	{
		return true;
	}

	void OnReceiveChunk(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) override
	{
		//do nothing
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
		SprayDatabaseInternal()->DispatchQueryStateChangeCallback(this, GetState());
	}
};

class CSprayQueryImageFileTask : public CSprayQueryBase
{
public:
	std::string m_userId;
	std::string m_fileName;
	std::string m_actualMediaUrl;

public:
	CSprayQueryImageFileTask(const std::string& userId, const std::string& fileName, const std::string& actualMediaUrl) :
		CSprayQueryBase(),
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
		//auto pRequestInstance = UtilHTTPClient()->CreateAsyncStreamRequest(m_Url.c_str(), UtilHTTPMethod::Get, new CUtilHTTPCallbacks(this));

		if (!pRequestInstance)
		{
			CSprayQueryBase::OnFailure();
			return;
		}

		UtilHTTPClient()->AddToRequestPool(pRequestInstance);

		m_RequestId = pRequestInstance->GetRequestId();

		pRequestInstance->Send();
	}

	bool OnProcessPayload(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) override
	{
		if (!CSprayQueryBase::OnProcessPayload(RequestInstance, ResponseInstance, data, size))
			return false;

		if (!Draw_ValidateImageFormatMemoryIO(data, size, m_fileName.c_str()))
		{
			gEngfuncs.Con_DPrintf("[BetterSpray] Acquired image file \"%s\" has invalid image format !\n", m_fileName.c_str());
			return true;
		}

		FILESYSTEM_ANY_CREATEDIR(CUSTOM_SPRAY_DIRECTORY, "GAMEDOWNLOAD");

		std::string filePath = std::format("{0}/{1}", CUSTOM_SPRAY_DIRECTORY, m_fileName);

		auto FileHandle = FILESYSTEM_ANY_OPEN(filePath.c_str(), "wb", "GAMEDOWNLOAD");

		if (FileHandle)
		{
			FILESYSTEM_ANY_WRITE(data, size, FileHandle);
			FILESYSTEM_ANY_CLOSE(FileHandle);

			gEngfuncs.Con_DPrintf("[BetterSpray] File \"%s\" acquired!\n", filePath.c_str());

			SprayDatabaseInternal()->OnImageFileAcquired(m_userId, filePath);
		}
		else
		{
			gEngfuncs.Con_DPrintf("[BetterSpray] Could not open \"%s\" for write!\n", filePath.c_str());

			SprayDatabaseInternal()->UpdatePlayerSprayQueryStatus(m_userId.c_str(), SprayQueryState_Failed);
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
	bool m_bWorkItemCompleted{};

public:
	CSprayQueryImageLinkTask(const std::string& userId, const std::string& fileId) :
		CSprayQueryBase(),
		m_userId(userId),
		m_fileId(fileId)
	{

	}

	bool IsFinished() const override
	{
		if (!CSprayQueryBase::IsFinished())
			return false;

		if (!m_bWorkItemCompleted)
			return false;

		return true;
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

	void OnProcessPayloadWorkItem(void* context) override
	{
		auto ctx = (CSprayQueryImageLinkWorkItemContext*)context;

		// 使用libxml解析HTML
		htmlDocPtr doc = htmlReadMemory(ctx->payload.c_str(), ctx->payload.size(), nullptr, nullptr, HTML_PARSE_NOWARNING | HTML_PARSE_NOERROR);
		if (!doc) {
			ctx->errorMessage = "htmlReadMemory: Failed to parse HTML document.";
			return;
		}

		SCOPE_EXIT() { xmlFreeDoc(doc); };

		// 使用XPath查找ActualMedia图片元素
		xmlXPathContextPtr XPathContext = xmlXPathNewContext(doc);
		if (!XPathContext) {
			ctx->errorMessage = "xmlXPathNewContext: Failed to create XPath context.";
			return;
		}

		SCOPE_EXIT() { xmlXPathFreeContext(XPathContext); };

		// 查找id为ActualMedia的img元素
		xmlXPathObjectPtr result = xmlXPathEvalExpression(BAD_CAST "//img[@id='ActualMedia']", XPathContext);
		if (!result) {
			ctx->errorMessage = "xmlXPathEvalExpression: Failed to evaluate XPath expression.";
			return;
		}

		SCOPE_EXIT() { xmlXPathFreeObject(result); };

		if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {

			ctx->errorMessage = "xmlXPathNodeSetIsEmpty: No nodes found.";
			return;
		}

		// 获取第一个匹配节点
		xmlNodePtr node = result->nodesetval->nodeTab[0];

		// 获取src属性
		xmlChar* src = xmlGetProp(node, BAD_CAST "src");
		if (!src) {
			ctx->errorMessage = "xmlGetProp: Failed to get src attribute.";
			return;
		}

		SCOPE_EXIT() { xmlFree(src); };

		// 保存图片URL
		ctx->actualMediaUrl = (const char*)src;
	}

	void OnProcessPayloadWorkItemComplete(void* context) override
	{
		auto ctx = (CSprayQueryImageLinkWorkItemContext*)context;

		if (ctx->actualMediaUrl.length() > 0)
		{
			gEngfuncs.Con_DPrintf("[BetterSpray] Found image URL: %s\n", ctx->actualMediaUrl.c_str());

			std::string localFileName = std::format("{0}.jpg", m_userId);

			SprayDatabaseInternal()->BuildQueryImageFile(m_userId, localFileName, ctx->actualMediaUrl);
		}

		if (ctx->errorMessage.length() > 0)
		{
			gEngfuncs.Con_DPrintf("[BetterSpray] Error: %s\n", ctx->errorMessage.c_str());
		}
		
		m_bWorkItemCompleted = true;
	}

	bool OnProcessPayload(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) override
	{
		if (!CSprayQueryBase::OnProcessPayload(RequestInstance, ResponseInstance, data, size))
			return false;

		auto ctx = new CSprayQueryImageLinkWorkItemContext(this, (const char *)data, size);

		auto hWorkItemHandle = g_pMetaHookAPI->CreateWorkItem(g_pMetaHookAPI->GetGlobalThreadPool(), [](void* context) -> bool {

			auto ctx = (CSprayQueryTaskListWorkItemContext*)context;

			ctx->pthis->OnProcessPayloadWorkItem(ctx);

			GameThreadTaskScheduler()->QueueTask(ctx);

			return true;

		}, ctx);

		g_pMetaHookAPI->QueueWorkItem(g_pMetaHookAPI->GetGlobalThreadPool(), hWorkItemHandle);

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
	std::string m_userId;
	bool m_bWorkItemCompleted{};

public:
	CSprayQueryTaskList(const std::string& userId) :
		CSprayQueryBase(),
		m_userId(userId)
	{

	}

	bool IsFinished() const override
	{
		if (!CSprayQueryBase::IsFinished())
			return false;

		if (!m_bWorkItemCompleted)
			return false;

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

	void OnProcessPayloadWorkItem(void* context) override
	{
		auto ctx = (CSprayQueryTaskListWorkItemContext*)context;

		// 解析HTML文档
		htmlDocPtr doc = htmlReadMemory(ctx->payload.c_str(), ctx->payload.length(), nullptr, "UTF-8", HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
		if (!doc) {
			ctx->errorMessage = "htmlReadMemory: Failed to parse HTML document.";
			return;
		}
		SCOPE_EXIT() { xmlFreeDoc(doc); };

		// 创建XPath上下文
		xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
		if (!xpathCtx) {
			ctx->errorMessage = "xmlXPathNewContext: Failed to create XPath context.";
			return;
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
				if (!info->fileId.empty() &&
					(info->description.starts_with("!"))) {

					ctx->floatHelpList.push_back(info);
				}
			}
		}
	}

	void OnProcessPayloadWorkItemComplete(void *context) override
	{
		auto ctx = (CSprayQueryTaskListWorkItemContext*)context;

		// 从floatHelpList中随机抽取一个元素并为其调用BuildQueryImageLink
		if (ctx->floatHelpList.size() > 0)
		{
#if 0
			// 生成随机索引
			size_t randomIndex = rand() % ctx->floatHelpList.size();
#else
			size_t randomIndex = 0;
#endif
			// 获取随机选择的元素
			auto selectedItem = ctx->floatHelpList[randomIndex];

			// 调用BuildQueryImageLink
			gEngfuncs.Con_DPrintf("[BetterSpray] Spary selected: fileId=%s\n", selectedItem->fileId.c_str());

			SprayDatabaseInternal()->BuildQueryImageLink(m_userId, selectedItem->fileId);
		}
		else
		{
			gEngfuncs.Con_DPrintf("[BetterSpray] No sprays found for \"%s\".\n", m_userId.c_str());

			SprayDatabaseInternal()->UpdatePlayerSprayQueryStatusInternal(m_userId.c_str(), SprayQueryState_Failed);
		}

		if (ctx->errorMessage.length() > 0)
		{
			gEngfuncs.Con_DPrintf("[BetterSpray] Error: %s\n", ctx->errorMessage.c_str());
		}

		m_bWorkItemCompleted = true;
	}

	bool OnProcessPayload(IUtilHTTPRequest* RequestInstance, IUtilHTTPResponse* ResponseInstance, const void* data, size_t size) override
	{
		if (!CSprayQueryBase::OnProcessPayload(RequestInstance, ResponseInstance, data, size))
			return false;

		auto ctx = new CSprayQueryTaskListWorkItemContext(this, (const char *)data, size);

		auto hWorkItemHandle = g_pMetaHookAPI->CreateWorkItem(g_pMetaHookAPI->GetGlobalThreadPool(), [](void* context) -> bool {

			auto ctx = (CSprayQueryTaskListWorkItemContext*)context;

			ctx->pthis->OnProcessPayloadWorkItem(ctx);

			GameThreadTaskScheduler()->QueueTask(ctx);

			return true;

		}, ctx);

		g_pMetaHookAPI->QueueWorkItem(g_pMetaHookAPI->GetGlobalThreadPool(), hWorkItemHandle);

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

class CSprayDatabase : public ISprayDatabaseInternal
{
private:
	std::vector<AutoPtr<ISprayQueryInternal>> m_QueryList;
	std::vector<ISprayQueryStateChangeHandler*> m_StateChangeCallbacks;
	std::unordered_map<std::string, SprayQueryState> m_UserQueryStatus{};

public:

	void Init() override
	{
		xmlInitParser();
	}

	void Shutdown() override
	{
		m_QueryList.clear();
		m_StateChangeCallbacks.clear();
		xmlCleanupParser();
	}

	void OnConnectToServer() override
	{
		m_UserQueryStatus.clear();
		m_QueryList.clear();
	}

	void RunFrame() override
	{
		auto flCurrentAbsTime = (float)gEngfuncs.GetAbsoluteTime();

		for (auto itor = m_QueryList.begin(); itor != m_QueryList.end();)
		{
			const auto& p = (*itor);

			p->RunFrame(flCurrentAbsTime);

			if (p->IsFinished() && !p->NeedRetry())
			{
				itor = m_QueryList.erase(itor);
				continue;
			}

			itor++;
		}
	}

	/*
		Purpose: Build query based on userId (steamId64), to get screenshot list.
	*/

	bool BuildQueryTaskList(int playerindex, const char* userId)
	{
		for (const auto& p : m_QueryList)
		{
			if (!strcmp(p->GetName(), "QueryTaskList") &&
				!strcmp(p->GetIdentifier(), userId))
			{
				return false;
			}
		}

		auto QueryList = new CSprayQueryTaskList(userId);
		m_QueryList.emplace_back(QueryList);
		QueryList->StartQuery();
		QueryList->Release();  

		return true;
	}

	/*
		Purpose: Build query based on fileId, to get actual image link.
	*/

	void BuildQueryImageLink(const std::string& userId, const std::string& fileId) override
	{
		for (const auto& p : m_QueryList)
		{
			if (!strcmp(p->GetName(), "QueryImageLink") &&
				!strcmp(p->GetIdentifier(), fileId.c_str()))
			{
				return;
			}
		}

		auto QueryInstance = new CSprayQueryImageLinkTask(userId, fileId);
		QueryInstance->StartQuery();
		m_QueryList.emplace_back(QueryInstance);
		QueryInstance->Release();
	}

	/*
		Purpose: Build query based on actualMediaUrl, to get actual image file.
	*/

	void BuildQueryImageFile(const std::string& userId, const std::string& fileName, const std::string& actualMediaUrl) override
	{
		for (const auto& p : m_QueryList)
		{
			if (!strcmp(p->GetName(), "QueryImageFile") &&
				!strcmp(p->GetIdentifier(), fileName.c_str()))
			{
				return;
			}
		}

		auto QueryInstance = new CSprayQueryImageFileTask(userId, fileName, actualMediaUrl);
		QueryInstance->StartQuery();
		m_QueryList.emplace_back(QueryInstance);
		QueryInstance->Release();
	}

	void OnImageFileAcquired(const std::string& userId, const std::string& filePath) override
	{
		if (EngineIsInLevel())
		{
			int playerindex = EngineFindPlayerIndexByUserId(userId.c_str());

			int result = Draw_LoadSprayTexture(userId.c_str(), filePath.c_str(), "GAMEDOWNLOAD", [playerindex](const char* userId, FIBITMAP* fiB) -> DRAW_LOADSPRAYTEXTURE_STATUS {
				return Draw_LoadSprayTexture_AsyncLoadInGame(playerindex, fiB);
			});

			if (result == 0)
			{
				UpdatePlayerSprayQueryStatusInternal(userId, SprayQueryState_Finished);
			}
			else
			{
				UpdatePlayerSprayQueryStatusInternal(userId, SprayQueryState_Failed);
			}
		}
	}

	void QueryPlayerSpray(int playerindex, const char* userId) override
	{
		std::string userIdString = userId;

		auto it = m_UserQueryStatus.find(userIdString);

		if (it == m_UserQueryStatus.end())
		{
			gEngfuncs.Con_DPrintf("[BetterSpray] Querying spary for userId \"%s\"...\n", userId);

			UpdatePlayerSprayQueryStatusInternal(userIdString, SprayQueryState_Querying);

			BuildQueryTaskList(playerindex, userId);
		}
		else
		{
			gEngfuncs.Con_DPrintf("[BetterSpray] UserId \"%s\" already queried!\n", userId);
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

	void UpdatePlayerSprayQueryStatusInternal(const std::string& userId, SprayQueryState newQueryStatus) override
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

	void UpdatePlayerSprayQueryStatus(const char *userId, SprayQueryState newQueryStatus) override
	{
		UpdatePlayerSprayQueryStatusInternal(userId, newQueryStatus);
	}

	void EnumQueries(IEnumSprayQueryHandler* handler) override
	{
		for (const auto& p : m_QueryList)
		{
			handler->OnEnumQuery(p);
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

	void DispatchQueryStateChangeCallback(ISprayQuery* pQuery, SprayQueryState newState) override
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

ISprayDatabaseInternal* SprayDatabaseInternal()
{
	return &s_SprayDatabase;
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CSprayDatabase, ISprayDatabase, SPARY_DATABASE_INTERFACE_VERSION, s_SprayDatabase)