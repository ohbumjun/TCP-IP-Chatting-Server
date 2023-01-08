#include "pch.h"
#include <iostream>
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"

// Service    : � ������ �� ���ΰ� ex) ���� ? Ŭ���̾�Ʈ ?
//            - ���� ������ ���� �°� Session ���� �� ���� ���
// Session    : �������� ����� ����ϴ� ��
//            - ���� ����, Recv, Send
// Listener   : ���� ���� => �ʱ�ȭ �� AcceptEx �Լ� �� ȣ��
// IocpObject : CreateIOCompletionPort, WSARecv ���� ���� key ������ �Ѱ��ֱ� ���� Object 
//            - Listener, Service �� ��ӹ޾Ƽ� ó���Ѵ�.
// IocpEvent  : Accept, Recv, Send �� �پ��� �̺�Ʈ�� Ŭ����ȭ ��Ų �� -> OVERLAPPED �� ���

char sendBuffer[] = "Hello World";

// Server ���� ��ǥ�ϴ� ����
class ServerSession : public Session
{
public:
	// Client : -> Connect ȣ�� -> ProcessConnect -> WSARecv ȣ�� -> ���� �ٷ� SEnd
	// -> Server ������ ���� ��������. -> OnSend ȣ�� -> ���ڼ����� �ٽ� ������
	// -> �� ������ WSARecv ȣ���صξ����Ƿ� �������� Recv �� ȣ��ȴ�.
	virtual void OnConnected() override
	{
		cout << "Connected To Server" << endl;
		Send((BYTE*)sendBuffer, sizeof(sendBuffer));
	}

	virtual void OnDisconnected() override
	{
		cout << "Client Disconnected" << endl;
	}

	virtual int32 OnRecv(BYTE* buffer, int32 len)
	{
		// Echo
		cout << "OnRecv Len Dummy = " << len << endl;

		this_thread::sleep_for(1s);

		// Server ������ ���ڼ��� ������� ������ �ٽ� �����ָ�
		// �ٽ� �� ������ 
		Send((BYTE*)sendBuffer, sizeof(sendBuffer));

		return len;
	};

	virtual void		OnSend(int32 len)
	{
		cout << "OnSend Len Dummy : " << len << endl;
	}
};

int main()
{
	this_thread::sleep_for(2s);

	// server �߱� ���� �������� �ʵ��� ���
	ClientServiceRef service = std::make_shared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		std::make_shared<IocpCore>(),
		[]()->SessionRef {return std::make_shared<ServerSession>(); },
		// 100 ? => Test ������ 100�� �����ϴ� ��
		1);
	
	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 2; ++i)
	{
		GThreadManager->Launch([=]()
			{
				while (true)
				{
					service->GetIocpCore()->Dispatch();
				}
			});
	}

	GThreadManager->Join();

	return 0;
}