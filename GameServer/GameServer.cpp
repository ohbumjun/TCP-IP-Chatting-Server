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
		// Echo
		SendBufferRef sendBuffer = GSendBufferManager->Open(4096);

		/*
		BYTE* buffer = sendBuffer->Buffer();

		((PacketHeader*)buffer)->size = sizeof(sendData) + sizeof(PacketHeader);
		((PacketHeader*)buffer)->id   = 1; // 1. Hello Msg

		::memcpy(&buffer[4], sendData, sizeof(sendData));

		sendBuffer->Close(sizeof(sendData) + sizeof(PacketHeader));
		*/

		// Helper �Լ� ����ϱ�
		BufferWriter bw(sendBuffer->Buffer(), 4096);
		
		// PacketHeader ũ�⸸ŭ ������ ����
		PacketHeader* header = bw.Reserve<PacketHeader>();

		// id(uint64), ü��(uint32), ���ݷ�(uint16)
		bw << (uint64)1001 << (uint32)100 << (uint16)10;

		// ������ �� ���� 
		bw.Write(sendData, sizeof(sendData));

		header->size = bw.WriteSize();
		header->id   = 1;

		sendBuffer->Close(bw.WriteSize());

		GSessionManager.Broadcast(sendBuffer);

		this_thread::sleep_for(250ms);
	}

	GThreadManager->Join();
	
}
