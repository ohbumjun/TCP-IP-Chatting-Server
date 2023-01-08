#include "pch.h"
#include <iostream>
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "BufferReader.h"
#include "ClientPacketHandler.h"

// Service    : � ������ �� ���ΰ� ex) ���� ? Ŭ���̾�Ʈ ?
//            - ���� ������ ���� �°� Session ���� �� ���� ���
// Session    : �������� ����� ����ϴ� ��
//            - ���� ����, Recv, Send
// Listener   : ���� ���� => �ʱ�ȭ �� AcceptEx �Լ� �� ȣ��
// IocpObject : CreateIOCompletionPort, WSARecv ���� ���� key ������ �Ѱ��ֱ� ���� Object 
//            - Listener, Service �� ��ӹ޾Ƽ� ó���Ѵ�.
// IocpEvent  : Accept, Recv, Send �� �پ��� �̺�Ʈ�� Ŭ����ȭ ��Ų �� -> OVERLAPPED �� ���

char sendData[] = "Hello World";

// Server ���� ��ǥ�ϴ� ����
class ServerSession : public PacketSession
{
public:
	// Client : -> Connect ȣ�� -> ProcessConnect -> WSARecv ȣ�� -> ���� �ٷ� SEnd
	// -> Server ������ ���� ��������. -> OnSend ȣ�� -> ���ڼ����� �ٽ� ������
	// -> �� ������ WSARecv ȣ���صξ����Ƿ� �������� Recv �� ȣ��ȴ�.
	virtual void OnConnected() override
	{
		// cout << "Connected To Server" << endl;

		// SendBuffer �� Ref �� �����ϴ� ���� ?
		// - Send �Լ� ȣ�� => RegisterSend ȣ�� => WSASend ȣ��
		// - �� �������ȿ� �������� SendBuffer �޸𸮰� �����Ǿ�� �Ѵ�.
		SendBufferRef sendBuffer = GSendBufferManager->Open(4096);
		::memcpy(sendBuffer->Buffer(), sendData, sizeof(sendData));

		// Echo Server ���
		// sendBuffer->CopyData(buffer, len);
		sendBuffer->Close(sizeof(sendData));

		// Send(sendBuffer);
	}

	virtual void OnDisconnected() override
	{
		cout << "Client Disconnected" << endl;
	}

	virtual void OnRecvPacket(BYTE* buffer, int32 len)
	{
		ClientPacketHandler::HandlePacket(buffer, len);
	};

	virtual void		OnSend(int32 len)
	{
		// cout << "OnSend Len Dummy : " << len << endl;
	}
};

int main()
{
	// server �߱� ���� �������� �ʵ��� ���
	this_thread::sleep_for(2s);

	// ������ ���� �Ѱ��� ������ŭ�� Session ���� => Connect �õ�
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