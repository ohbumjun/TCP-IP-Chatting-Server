#pragma once
#include "NetAddress.h"
#include "Listener.h"
#include "iocpCore.h"
#include <functional>

// Server 역할을 할 것인지, Client역할을 할 것인지
enum class ServiceType : uint8
{
	Server, 
	Client
};

using SessionFactory = function<SessionRef(void)>;

// Client 역할, Server 역할을 하는 등 다양한 형태의 Service 가 있을 수 있다.
// Server 중에서도 다양한 형태가 있을 수 있다.
class Service : public enable_shared_from_this<Service>
{
public :
	//maxSessionCnt : 최대 동시접속자수 
	Service(ServiceType type, NetAddress address, IocpCoreRef core, 
		SessionFactory factory, int32 maxSessionCnt = 1);
	virtual ~Service();

	virtual bool Start() abstract;
	bool CanStart() { return _sessionFactory != nullptr; }; 
	virtual void CloneService();
	void SetSessionFactory(SessionFactory func) { _sessionFactory = func; };
	// Session 생성 및 IocpCore 에 등록
	SessionRef CreateSession();
	void AddSession(SessionRef session);
	void ReleaseSession(SessionRef session);
	int32 GetCurrentSessionCount() { return _sessionCount; }
	int32 GetMaxSessionCount() { return _maxSessionCount; }
public :
	ServiceType GetSeviceType() { return _type; };
	NetAddress GetNetAddress() { return _netAddress; };
	IocpCoreRef& GetIocpCore() { return _iocpCore; }
protected:
	USE_LOCK;
	ServiceType _type;
	NetAddress _netAddress = {};
	IocpCoreRef _iocpCore; // 어떤 iocpCore 에 일감을 할당할 것인가

	set<SessionRef> _sessions; // 사용하고 있는 Sesion 목록들
	int32 _sessionCount = 0;
	int32 _maxSessionCount = 0;
	SessionFactory _sessionFactory;
};


/* Client Service */
class ClientService : public Service
{
public :
	ClientService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory,
		int32 maxSessionCount = 1);
	virtual ~ClientService(){}

	virtual bool Start() override;
};

class ServerService : public Service
{
public:
	// address : 자기 자신의 주소 
	ServerService(NetAddress address, IocpCoreRef core, SessionFactory factory,
		int32 maxSessionCount = 1);
	virtual ~ServerService() {}

	virtual bool Start() override;
	virtual void CloneService() override;

private :
	ListenerRef _listener = nullptr;
};