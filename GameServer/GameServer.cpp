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

#include "SocketUtils.h"
#include "Listener.h"

int main()
{
	Listener listener;
	listener.StartAccept(NetAddress(L"127.0.0.1",7777));

	for (int32 i = 0; i < 5; ++i)
	{
		GThreadManager->Launch([=]()
			{
				while (true)
				{
					GIocpCore.Dispatch();
				}
			});
	}

	GThreadManager->Join();

	cout << "Client Connected" << endl;
}
