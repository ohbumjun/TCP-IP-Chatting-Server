#include "pch.h"
#include <iostream>
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "BufferReader.h"
#include "ClientPacketHandler.h"

// Service    : 어떤 역할을 할 것인가 ex) 서버 ? 클라이언트 ?
//            - 동시 접속자 수에 맞게 Session 생성 및 관리 기능
// Session    : 실질적인 기능을 담당하는 곳
//            - 소켓 소유, Recv, Send
// Listener   : 서버 소켓 => 초기화 및 AcceptEx 함수 등 호출
// IocpObject : CreateIOCompletionPort, WSARecv 등을 통해 key 값으로 넘겨주기 위한 Object 
//            - Listener, Service 가 상속받아서 처리한다.
// IocpEvent  : Accept, Recv, Send 등 다양한 이벤트를 클래스화 시킨 것 -> OVERLAPPED 를 상속

char sendData[] = "Hello World";

// Server 쪽을 대표하는 세션
class ServerSession : public PacketSession
{
public:
	// Client : -> Connect 호출 -> ProcessConnect -> WSARecv 호출 -> 직후 바로 SEnd
	// -> Server 측으로 먼저 보내진다. -> OnSend 호출 -> 에코서버가 다시 돌려줌
	// -> 저 위에서 WSARecv 호출해두었으므로 실질적인 Recv 가 호출된다.
	virtual void OnConnected() override
	{
		// cout << "Connected To Server" << endl;

		// SendBuffer 를 Ref 로 관리하는 이유 ?
		// - Send 함수 호출 => RegisterSend 호출 => WSASend 호출
		// - 이 과정동안에 실질적인 SendBuffer 메모리가 유지되어야 한다.
		SendBufferRef sendBuffer = GSendBufferManager->Open(4096);
		::memcpy(sendBuffer->Buffer(), sendData, sizeof(sendData));

		// Echo Server 기능
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
	// server 뜨기 전에 접속하지 않도록 대기
	this_thread::sleep_for(2s);

	// 마지막 인자 넘겨준 개수만큼의 Session 생성 => Connect 시도
	ClientServiceRef service = std::make_shared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		std::make_shared<IocpCore>(),
		[]()->SessionRef {return std::make_shared<ServerSession>(); },
		// 100 ? => Test 접속자 100명 설정하는 것
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