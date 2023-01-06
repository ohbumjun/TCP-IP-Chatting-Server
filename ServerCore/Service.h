#pragma once
#include "NetAddress.h"
#include "Listener.h"
#include "iocpCore.h"
#include <functional>

// Server ������ �� ������, Client������ �� ������
enum class ServiceType : uint8
{
	Server, 
	Client
};

using SessionFactory = function<SessionRef(void)>;

// Client ����, Server ������ �ϴ� �� �پ��� ������ Service �� ���� �� �ִ�.
// Server �߿����� �پ��� ���°� ���� �� �ִ�.
class Service : public enable_shared_from_this<Service>
{
public :
	//maxSessionCnt : �ִ� ���������ڼ� 
	Service(ServiceType type, NetAddress address, IocpCoreRef core, 
		SessionFactory factory, int32 maxSessionCnt = 1);
	virtual ~Service();

	virtual bool Start() abstract;
	bool CanStart() { return _sessionFactory != nullptr; }; 
	virtual void CloneService();
	void SetSessionFactory(SessionFactory func) { _sessionFactory = func; };
	// Session ���� �� IocpCore �� ���
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
	IocpCoreRef _iocpCore; // � iocpCore �� �ϰ��� �Ҵ��� ���ΰ�

	set<SessionRef> _sessions; // ����ϰ� �ִ� Sesion ��ϵ�
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
	// address : �ڱ� �ڽ��� �ּ� 
	ServerService(NetAddress address, IocpCoreRef core, SessionFactory factory,
		int32 maxSessionCount = 1);
	virtual ~ServerService() {}

	virtual bool Start() override;
	virtual void CloneService() override;

private :
	ListenerRef _listener = nullptr;
};