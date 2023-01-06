#pragma once

// �̺�Ʈ Data ���� 2���� 
// 1. ��Ͻ� Ű������ �־��� �� (CreateIoCompletionPort)
// 2. GetQueuedCompletionStatus ���� overlapped ����ü�� �Ѱ��ֱ�

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

// OVERLAPPED �� ���
// ��, iocpEvent �� �޸� �� ���ʿ��� OVERLAPPED �޸𸮰� ������� ��
// *IocpEvent �� �� *OVERLAPPED �ε� ���� �� �ִٴ� �ǹ�
// �̶� �߿��� ���� �����Լ��� ����ϸ� �ȵȴ�.
// �����Լ����̺� ���� �����Ͱ� �޸𸮻� �� �տ� ��ġ�ϰ� �Ǳ� ������
// *iocpEvent �� *OVERLAPPED �� �ٲ� �� ���� �Ǳ� �����̴�.
class IocpEvent : public OVERLAPPED
{
public:
	IocpEvent(EventType type);

	void		Init();
	EventType	GetType() { return eventType; }

public :
	EventType	eventType;

	// �ڽ��� ���� ����ϱ� 
	IocpObjectRef owner;
};

// �Ʒ����ʹ� ������ ��ü���� Event �� ���� Ŭ���� ����

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

	// �ӽ÷� Send �ÿ� ������ �����͸� �����ص� ���̴�
	vector<BYTE> buffer;
};