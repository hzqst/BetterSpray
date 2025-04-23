#pragma once

#include <interface.h>

#define CUSTOM_SPRAY_DIRECTORY "custom_sprays"

enum SprayQueryState
{
	SprayQueryState_Unknown = 0,
	SprayQueryState_Querying,
	SprayQueryState_Failed,
	SprayQueryState_Finished,
};

class ISprayQuery : public IBaseInterface
{
public:
	virtual const char* GetName() const = 0;
	virtual const char* GetIdentifier() const = 0;
	virtual const char* GetUrl() const = 0;
	virtual bool IsFailed() const = 0;
	virtual bool IsFinished() const = 0;
	virtual SprayQueryState GetState() const = 0; 
	virtual unsigned int GetTaskId() const = 0;
};

class IEnumSprayQueryHandler : public IBaseInterface
{
public:
	virtual void OnEnumQuery(ISprayQuery* pQuery) = 0;
};

class ISprayQueryStateChangeHandler : public IBaseInterface
{
public:
	virtual void OnQueryStateChanged(ISprayQuery* pQuery, SprayQueryState newState) = 0;
};

class ISprayDatabase : public IBaseInterface
{
public:
	virtual void Init() = 0;
	virtual void Shutdown() = 0;
	virtual void RunFrame() = 0;
	virtual void UpdatePlayerSprayQueryStatus(const char* userId, SprayQueryState newQueryStatus) = 0;
	virtual SprayQueryState GetPlayerSprayQueryStatus(const char* userId) const = 0;
	virtual void QueryPlayerSpray(int playerindex, const char* userId) = 0;
	virtual void EnumQueries(IEnumSprayQueryHandler *handler) = 0;
	virtual void RegisterQueryStateChangeCallback(ISprayQueryStateChangeHandler* handler) = 0;
	virtual void UnregisterQueryStateChangeCallback(ISprayQueryStateChangeHandler* handler) = 0;
	virtual void DispatchQueryStateChangeCallback(ISprayQuery* pQuery, SprayQueryState newState) = 0;
};

ISprayDatabase* SprayDatabase();

#define SPARY_DATABASE_INTERFACE_VERSION "SprayDatabase_API_001"