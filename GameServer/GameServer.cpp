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

// Service    : � ������ �� ���ΰ� ex) ���� ? Ŭ���̾�Ʈ ?
//            - ���� ������ ���� �°� Session ���� �� ���� ���
// Session    : �������� ����� ����ϴ� ��
//            - ���� ����, Recv, Send
// Listener   : ���� ���� => �ʱ�ȭ �� AcceptEx �Լ� �� ȣ��
// IocpObject : CreateIOCompletionPort, WSARecv ���� ���� key ������ �Ѱ��ֱ� ���� Object 
//            - Listener, Service �� ��ӹ޾Ƽ� ó���Ѵ�.
// IocpEvent  : Accept, Recv, Send �� �پ��� �̺�Ʈ�� Ŭ����ȭ ��Ų �� -> OVERLAPPED �� ���


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
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_TEST(1001, 100, 10, buffs, L"�ȳ��ϼ���");
		GSessionManager.Broadcast(sendBuffer);

		this_thread::sleep_for(250ms);
	}

	GThreadManager->Join();
}
