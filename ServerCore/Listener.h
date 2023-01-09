#pragma once
#include "IocpCore.h"
#include "NetAddress.h"

class AcceptEvent;
class ServerService;

/*--------------
	Listener
---------------*/

// ������ ����
// - Listener �� ���� iocpCore �� ����ϰ�, ���� ������� ���Ƕ�� �� ���̴�.
// - CreateIoCompletionPort ���� ���� ������ CP �� ����� ��, �Ѱ��ִ� Key �����ε� ���� �� �ִ�.
//   ���� ����
// 1) StartAccept => ���� ���� ����
// 2) RegisterAccept => ���� ���� �� Ŭ���̾�Ʈ ���� ��û �޾Ƶ��̱�
// 3) ���� ���� ��û�� ���� ������ iocpCore::Dispatch �Լ� ȣ�� 
//    => iocpObject (= Listner)->Dispatch �Լ� ȣ��
// 4) ProcessAccept() �Լ� ȣ�� => Client ���� ���� ��
//    - session �� �Լ����� ȣ���������ν� �������� �۾� ���� 
//      -- ProcessConnect()
class Listener : public IocpObject
{
public:
	Listener() = default;
	~Listener();

public:
	/* �ܺο��� ��� */
	bool StartAccept(ServerServiceRef service);
	void CloseSocket();

public:
	/* �������̽� ���� */
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
	/* ���� ���� */
	void RegisterAccept(AcceptEvent* acceptEvent);
	void ProcessAccept(AcceptEvent* acceptEvent);

protected:
	SOCKET _socket = INVALID_SOCKET;
	Vector<AcceptEvent*> _acceptEvents;
	ServerServiceRef _service;
};

