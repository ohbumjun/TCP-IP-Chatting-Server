#include <iostream>
#include <atomic>
#include <mutex>
#include <windows.h>
#include <future>

#include "pch.h"
#include "CorePch.h"
#include "CoreMacro.h"
#include "ThreadManager.h"

#include "PlayerManager.h"
#include "AccountManager.h"

#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "BufferWriter.h"

// Service    : 어떤 역할을 할 것인가 ex) 서버 ? 클라이언트 ?
//            - 동시 접속자 수에 맞게 Session 생성 및 관리 기능
// Session    : 실질적인 기능을 담당하는 곳
//            - 소켓 소유, Recv, Send
// Listener   : 서버 소켓 => 초기화 및 AcceptEx 함수 등 호출
// IocpObject : CreateIOCompletionPort, WSARecv 등을 통해 key 값으로 넘겨주기 위한 Object 
//            - Listener, Service 가 상속받아서 처리한다.
// IocpEvent  : Accept, Recv, Send 등 다양한 이벤트를 클래스화 시킨 것 -> OVERLAPPED 를 상속


int main()
{
	ServerServiceRef service = std::make_shared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		std::make_shared<IocpCore>()   ,
		[]()->SessionRef {return std::make_shared<GameSession>(); },
		100);

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

	char sendData[] = "Hello From Server";
	
	while (true)
	{
		// Echo
		SendBufferRef sendBuffer = GSendBufferManager->Open(4096);

		/*
		BYTE* buffer = sendBuffer->Buffer();

		((PacketHeader*)buffer)->size = sizeof(sendData) + sizeof(PacketHeader);
		((PacketHeader*)buffer)->id   = 1; // 1. Hello Msg

		::memcpy(&buffer[4], sendData, sizeof(sendData));

		sendBuffer->Close(sizeof(sendData) + sizeof(PacketHeader));
		*/

		// Helper 함수 사용하기
		BufferWriter bw(sendBuffer->Buffer(), 4096);
		
		// PacketHeader 크기만큼 데이터 쓰기
		PacketHeader* header = bw.Reserve<PacketHeader>();

		// id(uint64), 체력(uint32), 공격력(uint16)
		bw << (uint64)1001 << (uint32)100 << (uint16)10;

		// 데이터 또 쓰기 
		bw.Write(sendData, sizeof(sendData));

		header->size = bw.WriteSize();
		header->id   = 1;

		sendBuffer->Close(bw.WriteSize());

		GSessionManager.Broadcast(sendBuffer);

		this_thread::sleep_for(250ms);
	}

	GThreadManager->Join();
	
}
