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


// 패킷 직렬화 (Serialization)
/*
class Player
{
public :
	// 자. hp, attack 과 같은 데이터는 상관없다.
	int32 hp = 0;
	int32 attack = 0;

	// 하지만
	// Player가 동적할당되는 주소란, 가상 주소이다.
	// 프로그램을 실행할 때마다 계속 바뀐다.
	// 따라서 target 이라는 주소값을 파일에 저장해도
	// 다음에 프로그램에 띄울 때는 해당 정보는 유효하지 않게 된다.
	// 곧이곧대로 복구할 수 없는 것이다.
	// vector 도 마찬가지. 내부적으로 동적 배열을 들고 있으므로
	
	// 네트워크 통신에서도 마찬가지이다.
	// 내 메모리에서의 주소값 정보를 상대에게 넘겨봤자
	// 아무런 쓸모가 없는 것이다.

	// 직렬화 한다는 것은 [byte 배열 혹은 string 배열]로 만들어서
	// 상대방에게 보낼 수 있게 만들어준다는 것이다.
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

	WCHAR sendData3[1000] = L"가"; // UTF16 = Unicode (한글/로마 2바이트)

	while (true)
	{
		/*
		( PKT_S_TEST_WRITE 사용 이전 버전 )
		vector<BuffData> buffs{ BuffData {100, 1.5f}, BuffData{200, 2.3f}, BuffData {300, 0.7f } };
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_TEST(1001, 100, 10, buffs, L"안녕하세요");
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
