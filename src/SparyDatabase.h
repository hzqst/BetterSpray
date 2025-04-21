#pragma once

#include <interface.h>

#define CUSTOM_SPRAY_DIRECTORY "custom_sprays"

enum SparyQueryState
{
	SparyQueryState_Querying = 0,
	SparyQueryState_Failed,
	SparyQueryState_Finished,
};

class IBaseSparyQuery : public IBaseInterface
{
public:
	virtual const char* GetName() const = 0;
	virtual const char* GetIdentifier() const = 0;
	virtual const char* GetUrl() const = 0;
	virtual bool IsFailed() const = 0;
	virtual bool IsFinished() const = 0;
	virtual SparyQueryState GetState() const = 0; 
	virtual unsigned int GetTaskId() const = 0;

	virtual void OnFinish() = 0;
	virtual void OnFailure() = 0;
	virtual bool OnProcessPayload(const char* data, size_t size) = 0;

	virtual void RunFrame(float flCurrentAbsTime) = 0;
	virtual void StartQuery() = 0;
};

class IEnumSparyQueryHandler : public IBaseInterface
{
public:
	virtual void OnEnumQuery(IBaseSparyQuery* pQuery) = 0;
};

class ISparyQueryStateChangeHandler : public IBaseInterface
{
public:
	virtual void OnQueryStateChanged(IBaseSparyQuery* pQuery, SparyQueryState newState) = 0;
};

class ISparyDatabase : public IBaseInterface
{
public:
	virtual void Init() = 0;
	virtual void Shutdown() = 0;
	virtual void RunFrame() = 0;
	virtual void QueryPlayerSpary(int playerindex, const char* userId) = 0;
	virtual void EnumQueries(IEnumSparyQueryHandler *handler) = 0;
	virtual void RegisterQueryStateChangeCallback(ISparyQueryStateChangeHandler* handler) = 0;
	virtual void UnregisterQueryStateChangeCallback(ISparyQueryStateChangeHandler* handler) = 0;
	virtual void DispatchQueryStateChangeCallback(IBaseSparyQuery* pQuery, SparyQueryState newState) = 0;
};

ISparyDatabase* SparyDatabase();

#define SPARY_DATABASE_INTERFACE_VERSION "SparyDatabase_API_001"