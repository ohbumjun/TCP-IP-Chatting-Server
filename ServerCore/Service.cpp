#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "Listener.h"

Service::Service(ServiceType type, NetAddress address, 
	IocpCoreRef core, SessionFactory factory, int32 maxSessionCnt) :
	_type(type), _netAddress(address), _iocpCore(core),
	_sessionFactory(factory), _maxSessionCount(maxSessionCnt)
{
}

Service::~Service()
{
}

bool Service::Start()
{
	return false;
}

void Service::CloneService()
{
	// Todo 
}

SessionRef Service::CreateSession()
{
	// ���� ����
	SessionRef session = _sessionFactory();

	// ���� ����
	session->SetService(shared_from_this());

	// IOCP �� ���� ��� (��Ȯ���� Session �� ���� ���)
	if (_iocpCore->Register(session) == false)
		return nullptr;

	return session;
}

void Service::AddSession(SessionRef session)
{
	WRITE_LOCK;
	_sessionCount += 1;
	_sessions.insert(session);
}

void Service::ReleaseSession(SessionRef session)
{
	WRITE_LOCK;
	ASSERT_CRASH(_sessions.erase(session) != 0);
	_sessionCount--;
};

ClientService::ClientService(NetAddress targetAddress,
	IocpCoreRef core, SessionFactory factory, int32 maxSessionCount) 
: Service(ServiceType::Client, targetAddress, core, factory, maxSessionCount)
{
}

bool ClientService::Start()
{
	if (CanStart() == false)
		return false;

	const int32 sessionCount = GetMaxSessionCount();

	for (int32 i = 0; i < sessionCount; ++i)
	{
		SessionRef session = CreateSession();

		if (session->Connect() == false)
			return false;
	}

	return true;
}


// address : �ڱ� �ڽ��� �ּ� 
ServerService::ServerService(NetAddress address, 
	IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: Service(ServiceType::Server, address, core, factory, maxSessionCount)
{
}

bool ServerService::Start()
{
	if (CanStart() == false)
		return false;

	_listener = std::make_shared<Listener>();

	if (_listener == nullptr)
		return false;

	ServerServiceRef service = static_pointer_cast<ServerService>(shared_from_this());
	
	if (_listener->StartAccept(service) == false)
	{
		return false;
	}

	return true;
}

void ServerService::CloneService()
{
	Service::CloneService();
}

