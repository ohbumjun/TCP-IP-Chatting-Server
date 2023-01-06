#pragma once

// 이벤트 Data 종류 2가지 
// 1. 등록시 키값으로 넣어줄 때 (CreateIoCompletionPort)
// 2. GetQueuedCompletionStatus 에서 overlapped 구조체로 넘겨주기

#pragma once

class Session;

enum class EventType : uint8
{
	Connect,
	Disconnect,
	Accept,
	//PreRecv,
	Recv,
	Send
};

/*--------------
	IocpEvent
---------------*/

// OVERLAPPED 를 상속
// 즉, iocpEvent 의 메모리 맨 앞쪽에는 OVERLAPPED 메모리가 들어있을 것
// *IocpEvent 는 곧 *OVERLAPPED 로도 사용될 수 있다는 의미
// 이때 중요한 것은 가상함수를 사용하면 안된다.
// 가상함수테이블 관련 포인터가 메모리상 맨 앞에 위치하게 되기 때문에
// *iocpEvent 를 *OVERLAPPED 로 바꿀 수 없게 되기 때문이다.
class IocpEvent : public OVERLAPPED
{
public:
	IocpEvent(EventType type);

	void		Init();
	EventType	GetType() { return eventType; }

public :
	EventType	eventType;

	// 자신의 주인 기억하기 
	IocpObjectRef owner;
};

// 아래부터는 각각의 구체적인 Event 에 대한 클래스 생성

/*----------------
	ConnectEvent
-----------------*/

class ConnectEvent : public IocpEvent
{
public:
	ConnectEvent() : IocpEvent(EventType::Connect) { }
};

/*----------------
	DisconnectEvent
-----------------*/

class DisconnectEvent : public IocpEvent
{
public:
	DisconnectEvent() : IocpEvent(EventType::Disconnect) { }
};

/*----------------
	AcceptEvent
-----------------*/

class AcceptEvent : public IocpEvent
{
public:
	AcceptEvent() : IocpEvent(EventType::Accept) { }

public:
	SessionRef _session = nullptr;
};

/*----------------
	RecvEvent
-----------------*/

class RecvEvent : public IocpEvent
{
public:
	RecvEvent() : IocpEvent(EventType::Recv) { }
};

/*----------------
	SendEvent
-----------------*/

class SendEvent : public IocpEvent
{
public:
	SendEvent() : IocpEvent(EventType::Send) { }

	// 임시로 Send 시에 보내는 데이터를 저장해둘 것이다
	vector<BYTE> buffer;
};