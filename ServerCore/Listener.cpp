#include "pch.h"
#include "Listener.h"
#include "SocketUtils.h"
#include "IocpEvent.h"
#include "Session.h"

/*--------------
	Listener
---------------*/

// 사실 보통 listener가 소멸자 까지 가게 되는 경우는 드물다.
Listener::~Listener()
{
	SocketUtils::Close(_socket);

	for (AcceptEvent* acceptEvent : _acceptEvents)
	{
		// TODO

		if (acceptEvent)
			delete acceptEvent;
	}
}

bool Listener::StartAccept(NetAddress netAddress)
{
	// 서버 소켓 생성
	_socket = SocketUtils::CreateSocket();

	if (_socket == INVALID_SOCKET)
		return false;

	// CP 에 소켓 할당 => 해당 소켓에 대한 IO 가 완료되면 CP 에 그 정보 저장된다.
	// IO 작업 완료 이후 CP 오브젝트에 할당된 쓰레드가 Dispatch 함수를 통해 처리
	if (GIocpCore.Register(this) == false)
		return false;

	// 이거 안하면 주소가 겹쳐서 서버 실행 안되는 경우 있다.
	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	// Linger 는 우선 꺼준다.
	if (SocketUtils::SetLinger(_socket, 0, 0) == false)
		return false;

	// 주소 할당
	if (SocketUtils::Bind(_socket, netAddress) == false)
		return false;
	
	// 연결 요청 대기큐 생성
	if (SocketUtils::Listen(_socket) == false)
		return false;

	// accept 함수를 호출해줌으로써 
	const int32 acceptCount = 1;
	for (int32 i = 0; i < acceptCount; i++)
	{
		AcceptEvent* acceptEvent = new AcceptEvent;

		_acceptEvents.push_back(acceptEvent);

		RegisterAccept(acceptEvent);
	}

	return false;
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
	// iocpEvent 는 accept event 여야만 한다.
	ASSERT_CRASH(iocpEvent->GetType() == EventType::Accept);

	AcceptEvent* acceptEvent = static_cast<AcceptEvent*>(iocpEvent);

	ProcessAccept(acceptEvent);
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent)
{
	Session* session = new Session;
		
	// Accept Event 에 Session 정보 세팅 => 이후 Listenr::Dispatch 를 통해서
	// iocpEvent 정보를 뽑아왔을 때 , 어떤 세션을 넘겨줬는지 알 수 있기 때문이다.
	acceptEvent->Init();
	acceptEvent->SetSession(session);

	DWORD bytesReceived = 0;

	// 비동기 accept 
	// 처음에 session->_socket 은 nullptr 이다. 다만, 해당 비동기 함수를 호출하여
	// 정상적으로 클라이언트 연결이 이루어지면, session->_socket 은 클라이언트 소켓 정보로 채워지게 된다.
	if (false == SocketUtils::AcceptEx(_socket, session->GetSocket(), 
		session->_recvBuffer, 0, sizeof(SOCKADDR_IN) + 16, 
		sizeof(SOCKADDR_IN) + 16, OUT & bytesReceived, 
		// AcceptEvent 가 overlapped 를 상속하고 있으므로, 아래의 코드가 가능
		static_cast<LPOVERLAPPED>(acceptEvent)))
	{
		const int32 errorCode = ::WSAGetLastError();
		
		if (errorCode != WSA_IO_PENDING)
		{
			// 실패 => 다시 시도
			RegisterAccept(acceptEvent);
		}
	}
}

// 자. Listner 에서 한번 만들어준 acceptEvent 는 이후 계속해서 재사용하게 된다.
void Listener::ProcessAccept(AcceptEvent* acceptEvent)
{
	// 연결 요청이 되었다면, 해당 요청에 대한 처리를 진행한다.
	Session* session = acceptEvent->GetSession();

	// Listener 소켓과 옵션을 똑같이 맞춰주는 부분
	if (false == SocketUtils::SetUpdateAcceptSocket(session->GetSocket(), _socket))
	{
		// 문제가 있다면, 다시 연결 요청을 받아들이기 
		RegisterAccept(acceptEvent);
		return;
	}

	SOCKADDR_IN sockAddress;
	int32 sizeOfSockAddr = sizeof(sockAddress);

	// 방금 접속한 클라이언트의 정보 추출 
	// ex) 주소 정보
	if (SOCKET_ERROR == ::getpeername(session->GetSocket(), 
		OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr))
	{
		RegisterAccept(acceptEvent);
		return;
	}

	// session의 클라이언트 소켓 주소 정보 세팅
	session->SetNetAddress(NetAddress(sockAddress));

	cout << "Client Connected!" << endl;

	// TODO

	// 또 다른 클라이언트 요청 받아들여주기 
	RegisterAccept(acceptEvent);
}