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

// Service    : � ������ �� ���ΰ� ex) ���� ? Ŭ���̾�Ʈ ?
//            - ���� ������ ���� �°� Session ���� �� ���� ���
// Session    : �������� ����� ����ϴ� ��
//            - ���� ����, Recv, Send
// Listener   : ���� ���� => �ʱ�ȭ �� AcceptEx �Լ� �� ȣ��
// IocpObject : CreateIOCompletionPort, WSARecv ���� ���� key ������ �Ѱ��ֱ� ���� Object 
//            - Listener, Service �� ��ӹ޾Ƽ� ó���Ѵ�.
// IocpEvent  : Accept, Recv, Send �� �پ��� �̺�Ʈ�� Ŭ����ȭ ��Ų �� -> OVERLAPPED �� ���

class GameSession : public Session
{
public  :
	virtual int32 OnRecv(BYTE* buffer, int32 len)
	{
		// Echo
		cout << "OnRecv Len Server = " << len << endl;

		// Echo Server ���
		Send(buffer, len);

		return len;
	};

	virtual void		OnSend(int32 len)
	{
		cout << "OnSend Len Server : " << len << endl;
	}
};

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
	
	GThreadManager->Join();
	
}
