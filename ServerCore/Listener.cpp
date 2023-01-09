#include "pch.h"
#include "Listener.h"
#include "SocketUtils.h"
#include "IocpEvent.h"
#include "Session.h"
#include "Service.h"

/*--------------
	Listener
---------------*/

Listener::~Listener()
{
	SocketUtils::Close(_socket);

	for (AcceptEvent* acceptEvent : _acceptEvents)
	{
		// TODO

		xdelete(acceptEvent);
	}
}

bool Listener::StartAccept(ServerServiceRef service)
{
	_service = service;
	if (_service == nullptr)
		return false;

	// ���� ���� ����
	_socket = SocketUtils::CreateSocket();
	if (_socket == INVALID_SOCKET)
		return false;

	// CP �� ���� �Ҵ� => �ش� ���Ͽ� ���� IO �� �Ϸ�Ǹ� CP �� �� ���� ����ȴ�.
	// IO �۾� �Ϸ� ���� CP ������Ʈ�� �Ҵ�� �����尡 Dispatch �Լ��� ���� ó��
	if (_service->GetIocpCore()->Register(shared_from_this()) == false)
		return false;

	// �̰� ���ϸ� �ּҰ� ���ļ� ���� ���� �ȵǴ� ��� �ִ�.
	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	// Linger �� �켱 ���ش�.
	if (SocketUtils::SetLinger(_socket, 0, 0) == false)
		return false;

	// �ּ� �Ҵ�
	if (SocketUtils::Bind(_socket, _service->GetNetAddress()) == false)
		return false;

	// ���� ��û ���ť ����
	if (SocketUtils::Listen(_socket) == false)
		return false;

	// accept �Լ��� ȣ���������ν� 
	const int32 acceptCount = _service->GetMaxSessionCount();

	for (int32 i = 0; i < acceptCount; i++)
	{
		AcceptEvent* acceptEvent = xnew<AcceptEvent>();

		// �Ʒ�ó�� �ϸ� Ref �� 1 ¥���� shared_ptr �� ���� �����ع����� ������ �ȴ�. (�ּҰ� �ٷ� �Ѱ��ֱ�X)
		// acceptEvent->owner = std::shared_ptr<IocpObject>(this);
		// �θ� Listner �� �ȴ�.
		// shared_from_this : �̹� �ڱ� �ڽ��� ����Ű�� shared_ptr �� ���� => ref Cnt 1 ����
		// ��, ������ �ڱ� �ڽ��� ����Ű�� shared_ptr ������ ������ ������ Ȱ���ϱ� ����

		acceptEvent->owner = shared_from_this();
		_acceptEvents.push_back(acceptEvent);
		RegisterAccept(acceptEvent);
	}

	return true;
}

void Listener::CloseSocket()
{
	SocketUtils::Close(_socket);
}

HANDLE Listener::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Listener::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	ASSERT_CRASH(iocpEvent->eventType == EventType::Accept);
	AcceptEvent* acceptEvent = static_cast<AcceptEvent*>(iocpEvent);
	ProcessAccept(acceptEvent);
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent)
{
	// IOCP �� CP �� ����ϴ� ������ ����
	SessionRef session = _service->CreateSession(); // Register IOCP

	// Accept Event �� Session ���� ���� => ���� Listenr::Dispatch �� ���ؼ�
	// iocpEvent ������ �̾ƿ��� �� , � ������ �Ѱ������ �� �� �ֱ� �����̴�.
	acceptEvent->Init();
	acceptEvent->session = session;

	DWORD bytesReceived = 0;

	// �񵿱� accept 
	// ó���� session->_socket �� nullptr �̴�. �ٸ�, �ش� �񵿱� �Լ��� ȣ���Ͽ�
	// ���������� Ŭ���̾�Ʈ ������ �̷������, session->_socket �� Ŭ���̾�Ʈ ���� ������ ä������ �ȴ�.
	if (false == SocketUtils::AcceptEx(_socket, session->GetSocket(), 
		session->_recvBuffer.WritePos(), 0, sizeof(SOCKADDR_IN) + 16, 
		sizeof(SOCKADDR_IN) + 16, OUT & bytesReceived, 
		// AcceptEvent �� overlapped �� ����ϰ� �����Ƿ�, �Ʒ��� �ڵ尡 ����
		static_cast<LPOVERLAPPED>(acceptEvent)))
	{
		const int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			// �ϴ� �ٽ� Accept �ɾ��ش�
			RegisterAccept(acceptEvent);
		}
	}
}

// Accept ��û�� �����ϸ�, ��, Ŭ���̾�Ʈ ��û�� ������ ������
// �Ʒ� �Լ� ȣ��
// ��. Listner ���� �ѹ� ������� acceptEvent �� ���� ����ؼ� �����ϰ� �ȴ�.
void Listener::ProcessAccept(AcceptEvent* acceptEvent)
{
	// Event �� ���� session ���� ����
	SessionRef session = acceptEvent->session;

	// Listener ���ϰ� �ɼ��� �Ȱ��� �����ִ� �κ�
	if (false == SocketUtils::SetUpdateAcceptSocket(session->GetSocket(), _socket))
	{
		// ������ �ִٸ�, �ٽ� ���� ��û�� �޾Ƶ��̱� 
		RegisterAccept(acceptEvent);
		return;
	}

	SOCKADDR_IN sockAddress;
	int32 sizeOfSockAddr = sizeof(sockAddress);

	// ��� ������ Ŭ���̾�Ʈ�� ���� ���� 
	// ex) �ּ� ����
	if (SOCKET_ERROR == ::getpeername(session->GetSocket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr))
	{
		RegisterAccept(acceptEvent);
		return;
	}

	// ServerSession�� Ŭ���̾�Ʈ ���� �ּ� ���� ���� ?
	session->SetNetAddress(NetAddress(sockAddress));

	// �ش� Ŭ���̾�Ʈ�κ��� �����͸� �񵿱�� Recv �ϴ� ���� ����
	session->ProcessConnect();

	// �� �ٸ� Ŭ���̾�Ʈ ��û �޾Ƶ鿩�ֱ� 
	RegisterAccept(acceptEvent);
}