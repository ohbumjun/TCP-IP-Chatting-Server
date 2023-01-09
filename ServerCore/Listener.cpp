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

	// 서버 소켓 생성
	_socket = SocketUtils::CreateSocket();
	if (_socket == INVALID_SOCKET)
		return false;

	// CP 에 소켓 할당 => 해당 소켓에 대한 IO 가 완료되면 CP 에 그 정보 저장된다.
	// IO 작업 완료 이후 CP 오브젝트에 할당된 쓰레드가 Dispatch 함수를 통해 처리
	if (_service->GetIocpCore()->Register(shared_from_this()) == false)
		return false;

	// 이거 안하면 주소가 겹쳐서 서버 실행 안되는 경우 있다.
	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	// Linger 는 우선 꺼준다.
	if (SocketUtils::SetLinger(_socket, 0, 0) == false)
		return false;

	// 주소 할당
	if (SocketUtils::Bind(_socket, _service->GetNetAddress()) == false)
		return false;

	// 연결 요청 대기큐 생성
	if (SocketUtils::Listen(_socket) == false)
		return false;

	// accept 함수를 호출해줌으로써 
	const int32 acceptCount = _service->GetMaxSessionCount();

	for (int32 i = 0; i < acceptCount; i++)
	{
		AcceptEvent* acceptEvent = xnew<AcceptEvent>();

		// 아래처럼 하면 Ref 가 1 짜리인 shared_ptr 을 새로 생성해버리는 문제가 된다. (주소값 바로 넘겨주기X)
		// acceptEvent->owner = std::shared_ptr<IocpObject>(this);
		// 부모가 Listner 가 된다.
		// shared_from_this : 이미 자기 자신을 가리키는 shared_ptr 을 리턴 => ref Cnt 1 증가
		// 즉, 기존에 자기 자신을 가리키는 shared_ptr 에서의 제어블록 정보를 활용하기 위함

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
	// IOCP 의 CP 에 등록하는 과정도 포함
	SessionRef session = _service->CreateSession(); // Register IOCP

	// Accept Event 에 Session 정보 세팅 => 이후 Listenr::Dispatch 를 통해서
	// iocpEvent 정보를 뽑아왔을 때 , 어떤 세션을 넘겨줬는지 알 수 있기 때문이다.
	acceptEvent->Init();
	acceptEvent->session = session;

	DWORD bytesReceived = 0;

	// 비동기 accept 
	// 처음에 session->_socket 은 nullptr 이다. 다만, 해당 비동기 함수를 호출하여
	// 정상적으로 클라이언트 연결이 이루어지면, session->_socket 은 클라이언트 소켓 정보로 채워지게 된다.
	if (false == SocketUtils::AcceptEx(_socket, session->GetSocket(), 
		session->_recvBuffer.WritePos(), 0, sizeof(SOCKADDR_IN) + 16, 
		sizeof(SOCKADDR_IN) + 16, OUT & bytesReceived, 
		// AcceptEvent 가 overlapped 를 상속하고 있으므로, 아래의 코드가 가능
		static_cast<LPOVERLAPPED>(acceptEvent)))
	{
		const int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			// 일단 다시 Accept 걸어준다
			RegisterAccept(acceptEvent);
		}
	}
}

// Accept 요청이 성공하면, 즉, 클라이언트 요청이 실제로 있으면
// 아래 함수 호출
// 자. Listner 에서 한번 만들어준 acceptEvent 는 이후 계속해서 재사용하게 된다.
void Listener::ProcessAccept(AcceptEvent* acceptEvent)
{
	// Event 로 부터 session 정보 추출
	SessionRef session = acceptEvent->session;

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
	if (SOCKET_ERROR == ::getpeername(session->GetSocket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr))
	{
		RegisterAccept(acceptEvent);
		return;
	}

	// ServerSession에 클라이언트 소켓 주소 정보 세팅 ?
	session->SetNetAddress(NetAddress(sockAddress));

	// 해당 클라이언트로부터 데이터를 비동기로 Recv 하는 절차 진행
	session->ProcessConnect();

	// 또 다른 클라이언트 요청 받아들여주기 
	RegisterAccept(acceptEvent);
}