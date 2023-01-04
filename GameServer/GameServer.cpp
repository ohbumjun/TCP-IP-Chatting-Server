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

#include <WinSock2.h>
#include <winsock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

void HandleError(const char* cause)
{
	int32 errCode = ::WSAGetLastError();
	cout << cause << "ErrorCode : " << errCode << endl;
};

const int32 BUFSIZE = 1000;

struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUFSIZE];
	int32 recvBytes = 0;
	int32 sendBytes = 0;
};

int main()
{
	WSADATA wsaData;

	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	// ���ŷ ���� : ���� �Ϸᰡ �� ������ ���
	// accept -> ������ Ŭ�� ���� ��
	// connect -> ���� ���� �������� ��
	// send, sendTo -> ��û�� �����͸� �۽� ���ۿ� �������� ��
	// recv, recvFrom -> ���� ���ۿ� ������ �����Ͱ� �ְ�, �̸� ���������� �о�鿴�� ��

	// ����ŷ ����(Non-Blocking)

	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);

	if (listenSocket == INVALID_SOCKET)
		return 0;

	u_long on = 1;

	// ����� ��� ���� => �ͺ��ŷ ���
	if (::ioctlsocket(listenSocket, FIONBIO, &on) == INVALID_SOCKET)
		return 0;

	SOCKADDR_IN serverAddr;

	::memset(&serverAddr, 0, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);
	serverAddr.sin_port = ::htons(7777);

	if (::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return 0;

	if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
		return 0;

	cout << "Accept" << endl;

	// WSAEventSelect
	// - ���ϰ� ���õ� ��Ʈ��ũ �̺�Ʈ�� [�̺�Ʈ ��ü]�� ���� ����
	// - ��, �̺�Ʈ�� ���� ������ �޴´�.

	// �̺�Ʈ ��ü ���� �Լ���
	// ���� : WSACreateEvent (manual reset + non-signaled)
	// ���� : WSACloseEvent
	// ��ȣ ���� ���� : WSAWaitForMultipleEvents
	// ��ü���� ��Ʈ��ũ �̺�Ʈ �˾Ƴ��� : WSAEnumNetworkdEvents

	// WSAEventSelect : ���� ~ �̺�Ʈ ���������ִ� �Լ� 
	// - ��, ���� ���� ��ŭ �̺�Ʈ�� �������� �Ѵ�.
	// WSAEventSelect(socket, event, networkEvents)
	// ��������� socket, �̺�Ʈ�� ���� ���� , � �̺�Ʈ�� ������ �� ?
	
	// �����ִ� ��Ʈ��ũ �̺�Ʈ
	// FD_ACCEPT  : ������ Ŭ�� ���� ? (accept)
	// FD_READ    : ������ ���� ���� ? (recv, recvfrom)
	// FD_WRITE   : ������ �۽� ���� ? (send)
	// FD_CLOSE   : ��밡 ���� ����
	// FD_CONNECT : ����� ���� ���� ���� �Ϸ�
	// FD_OOB

	// ���� ����
	// - WSAEventSelect �Լ��� ȣ���ϸ�, �ش� ������ �ڵ����� �ͺ��ŷ ���� ��ȯ
	//   ��, ó���� ���ŷ ���� �����ξ��ٰ� �ص� �ڵ����� �ͺ��ŷ ���� !
	// - accept() �Լ��� �����ϴ� ������ listenSocket �� ������ �Ӽ��� ���´�. ��, �ͺ��ŷ ���
	//   ���� clientSocket �� FD_READ, FD_WRITE ���� �ٽ� ����ؾ� �Ѵ�.
	//   �幰�� WSAEWOULDBLOCK ������ �� �� ������ ���� ó�� �ʿ�
	// - �̺�Ʈ �߻� �� , ������ ���� �Լ� ȣ���ؾ� �Ѵ�.
	//   �ƴϸ� ���� ������ ���� ��Ʈ��ũ �̺�Ʈ�� �߻� X
	//   ex) FD_READ �̺�Ʈ�� �߻��ϸ�, recv �Լ��� ȣ���ؾ߸�
	//       ���� ������ FD_READ �̺�Ʈ�� ������ �� �ְ� �Ǵ� ���̴�.
	//       ��, ������ �������� �������, ���� �� ������ ���� ������
	
	// WSAEventSelect, Select �Լ� ������ ?
	// - select �Լ��� ���, ����� ���� ������ ��ٷȴٰ� ������� ����ش�.
	// - �ݸ� WSAEventSelect �� ���ϰ� �̺�Ʈ�� ���������ֱ⸸ �Ѵ�.
	//   ������� ��� ����, WSAWaitForMultipleEvents �� ���� �̺�Ʈ�� �����Ͽ� ��´�.

	// WSAWaitForMultipleEvents
	// 1) count (�̺�Ʈ ����) , event(�̺�Ʈ ������)
	// 2) waitAll : ��� ��ٸ� > �ϳ��� �Ϸ�Ǿ ok ?
	// 3) timeout : Ÿ�Ӿƿ�
	// 4) ������ false
	// return : �Ϸ�� ù��° Event ��ü(������Ʈ) idx

	// WSAEnumNetworkEvents : �Ϸ�� �̺�Ʈ ������Ʈ�� �˰ڴ�. ���� � �̺�Ʈ�� �Ϸ�Ǿ����� Ȯ��
	// 1) socket
	// 2) eventObject :socket �� ������ �̺�Ʈ ��ü �ڵ��� �Ѱ��ָ�
	//    - �̺�Ʈ ��ü�� non-signaled ���·� ������ش�.
	// 3) networkEvents : ��Ʈ��ũ �̺�Ʈ / ���� ������ ����

	vector<WSAEVENT> wsaEvents;
	vector<Session> sessions;
	wsaEvents.reserve(100);
	sessions.reserve(100);

	// listenSocket �� ���� �̺�Ʈ ��ü
	WSAEVENT listenEvent = ::WSACreateEvent();
	wsaEvents.push_back(listenEvent);
	sessions.push_back(Session{ listenSocket });

	// socket, �̺�Ʈ ��ü ���� ��Ű��
	if (::WSAEventSelect(listenSocket, listenEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
		return 0;

	while (true)
	{
		// 3��° : false => �ϳ��� �Ϸ�Ǹ� ����������
		// ��, signaled ���°� �� EventObject �� �ִٸ� ���������� ���̴�.
		int32 index = ::WSAWaitForMultipleEvents(wsaEvents.size(), &wsaEvents[0], FALSE, WSA_INFINITE, FALSE);
		
		// Error
		if (index == WSA_WAIT_FAILED)
			continue;
		
		index -= WSA_WAIT_EVENT_0;

		// ����� �޾ƿ��� 
		WSANETWORKEVENTS networkEvents;

		// WSAEnumNetworkEvents �Լ� ȣ�� ����, �ڵ����� wsaEvents[index] �� non-signaled �� �ٲ��.
		if (::WSAEnumNetworkEvents(sessions[index].socket, wsaEvents[index], &networkEvents) == SOCKET_ERROR)
			continue;

		// Listener ���� üũ
		if (networkEvents.lNetworkEvents & FD_ACCEPT)
		{
			// Error Check
			if (networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
				continue;

			// Accept �̺�Ʈ�� �߻��� ���̹Ƿ� accept ȣ��
			SOCKADDR_IN clientAddr;
			int32 addrLen = sizeof(clientAddr);
			SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

			if (clientSocket != SOCKET_ERROR)
			{
				cout << "Cient connected" << endl;

				WSAEVENT clientEvent = ::WSACreateEvent();
				wsaEvents.push_back(clientEvent);
				sessions.push_back(Session({ clientSocket }));

				if (::WSAEventSelect(clientSocket, clientEvent, FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR)
					return 0;
			}
		}

		// Client Session ���� üũ
		if (networkEvents.lNetworkEvents & FD_READ || networkEvents.lNetworkEvents & FD_WRITE)
		{
			// Error - Check
			if ((networkEvents.lNetworkEvents & FD_READ) && (networkEvents.iErrorCode[FD_READ_BIT] != 0))
				continue;

			if ((networkEvents.lNetworkEvents & FD_WRITE) && (networkEvents.iErrorCode[FD_WRITE_BIT] != 0))
				continue;

			Session& s = sessions[index];
			
			// Read
			if (s.recvBytes == 0)
			{
				// �ƹ��͵� ���� ���� ����
				int32 recvLen = ::recv(s.socket, s.recvBuffer, BUFSIZE, 0);

				if (recvLen == SOCKET_ERROR)
				{
					// ���� ����߾�� �ߴµ�... �ʰ� ����ŷ���� �϶��
					if (::WSAGetLastError() == WSAEWOULDBLOCK)
						continue;

					// Error
					cout << "break" << endl;
					break;
				}

				s.recvBytes = recvLen;

				cout << "Recv data :  " << recvLen << endl;
			}

			// Write
			if (s.recvBytes > s.sendBytes)
			{
				// ���� �����Ͱ� �����ֵ���
				int32 sendLen = ::send(s.socket, &s.recvBuffer[s.sendBytes], s.recvBytes - s.sendBytes, 0);

				if (sendLen == SOCKET_ERROR && ::WSAGetLastError() != WSAEWOULDBLOCK)
				{
					// ���� Error
					continue;
				}

				s.sendBytes += sendLen;

				if (s.sendBytes == s.recvBytes)
				{
					s.sendBytes = 0;
					s.recvBytes = 0;

					cout << "Send data :  " << sendLen << endl;
				}
			}
		}

		if (networkEvents.lNetworkEvents & FD_CLOSE)
		{
			// Remove Socket
		}
	}

	// ���� ����
	::WSACleanup();
}


/* DEBUG Profiler
	GThreadManager->Launch([=] {
		while (true)
		{
			cout << "PlayerThenAccount" << endl;
			GPlayerManager.PlayerThenAccount();
			this_thread::sleep_for(100ms);
		}
	});

	GThreadManager->Launch([=] {
		while (true)
		{
			cout << "AccountThenPlayer" << endl;
			GAccountManager.AccountThenPlayer();
			this_thread::sleep_for(100ms);
		}
	});

	GThreadManager->Join();
	*/