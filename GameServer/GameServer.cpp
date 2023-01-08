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
#include "ServerPacketHandler.h"

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
		vector<BuffData> buffs{ BuffData {100, 1.5f}, BuffData{200, 2.3f}, BuffData {300, 0.7f } };
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_TEST(1001, 100, 10, buffs, L"안녕하세요");
		GSessionManager.Broadcast(sendBuffer);

		this_thread::sleep_for(250ms);
	}

	GThreadManager->Join();
}
