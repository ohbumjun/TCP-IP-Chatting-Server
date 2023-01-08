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


// ��Ŷ ����ȭ (Serialization)
/*
class Player
{
public :
	// ��. hp, attack �� ���� �����ʹ� �������.
	int32 hp = 0;
	int32 attack = 0;

	// ������
	// Player�� �����Ҵ�Ǵ� �ּҶ�, ���� �ּ��̴�.
	// ���α׷��� ������ ������ ��� �ٲ��.
	// ���� target �̶�� �ּҰ��� ���Ͽ� �����ص�
	// ������ ���α׷��� ��� ���� �ش� ������ ��ȿ���� �ʰ� �ȴ�.
	// ���̰��� ������ �� ���� ���̴�.
	// vector �� ��������. ���������� ���� �迭�� ��� �����Ƿ�
	
	// ��Ʈ��ũ ��ſ����� ���������̴�.
	// �� �޸𸮿����� �ּҰ� ������ ��뿡�� �Ѱܺ���
	// �ƹ��� ���� ���� ���̴�.

	// ����ȭ �Ѵٴ� ���� [byte �迭 Ȥ�� string �迭]�� ����
	// ���濡�� ���� �� �ְ� ������شٴ� ���̴�.
	Player* target = nullptr;
	vector<int32> buffer;
};
*/

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

	WCHAR sendData3[1000] = L"��"; // UTF16 = Unicode (�ѱ�/�θ� 2����Ʈ)

	while (true)
	{
		/*
		( PKT_S_TEST_WRITE ��� ���� ���� )
		vector<BuffData> buffs{ BuffData {100, 1.5f}, BuffData{200, 2.3f}, BuffData {300, 0.7f } };
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_TEST(1001, 100, 10, buffs, L"�ȳ��ϼ���");
		*/

		// [ PKT_S_TEST ]
		PKT_S_TEST_WRITE pktWriter(1001, 100, 10);

		// [ PKT_S_TEST ][BuffsListItem BuffsListItem BuffsListItem]
		PKT_S_TEST_WRITE::BuffsList buffList = pktWriter.ReserveBuffsList(3);
		buffList[0] = { 100, 1.5f };
		buffList[1] = { 200, 2.3f };
		buffList[2] = { 300, 0.7f };

		PKT_S_TEST_WRITE::BuffsVictimsList vic0 = pktWriter.ReserveBuffsVictimsList(&buffList[0], 3);
		{
			vic0[0] = 1000;
			vic0[1] = 2000;
			vic0[2] = 3000;
		}

		PKT_S_TEST_WRITE::BuffsVictimsList vic1 = pktWriter.ReserveBuffsVictimsList(&buffList[1], 1);
		{
			vic1[0] = 1000;
		}

		PKT_S_TEST_WRITE::BuffsVictimsList vic2 = pktWriter.ReserveBuffsVictimsList(&buffList[2], 2);
		{
			vic2[0] = 3000;
			vic2[1] = 5000;
		}

		SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

		GSessionManager.Broadcast(sendBuffer);

		this_thread::sleep_for(250ms);
	}

	GThreadManager->Join();
}
