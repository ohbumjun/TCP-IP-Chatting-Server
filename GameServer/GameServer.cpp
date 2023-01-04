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

	// Select
	// ���� �Լ� ȣ���� ������ ������ �̸� �� �� �ִ�.
	// (���ŷ, ����ŷ ��� ���밡��)
	
	// ���� �Լ� ȣ���� �����Ѵ� ? 
	// ������Ȳ 1) ���� ���ۿ� �����Ͱ� ���µ� read �Ѵٰų�
	// ������Ȳ 2) �۽� ���۰� �� á�µ� write �Ѵٰų�
	
	// - ���ŷ ����   : ������ �������� �ʾƼ� ���ŷ �Ǵ� ��Ȳ ����
	// - ����ŷ ���� : ������ �������� �ʾƼ� ���ʿ��ϰ� �ݺ� üũ�ϴ� ��Ȳ ����
	// ��, send, recv �Լ� ȣ�� ������ ���� send, recv �� �� �ִ��� üũ

	// socket set
	// 1) �б�(���� �غ� �� �༮�� �ִ°�, ������ ���� �༮�� �ִ°�) [], 
	// 2) ����(�����͸� ���� �غ� �� �༮�� �ִ°�) [], 
	// 3) ����(ex. Out of Band) [] ���� ��� ���

	// > select(readSet, writeSet, exceptSet) 
	// > ��� �ϳ��� ������ �غ�Ǹ� ���� => �����ڴ� �˾Ƽ� ���� (��Ʈ ���� 0���� �����. ��ȭ �ִ� �༮�� 1�� ���´�)
	// > ���� ���� üũ�ؼ� ����

	// fd_set read
	// FD_ZERO : ����.
	// ex) FD_ZERO(set)
	// FD_SET : ���� s �� �ִ´� (������� �߰�)
	// ex) FD_SET(s, &set);
	// FD_CLR : ���� s ����
	// ex) FD_CLR(s, &set);
	// FD_ISSET : ���� s �� set �� ��������� 0�� �ƴ� ���� �����Ѵ�.
	// ex)

	vector<Session> sessions;
	sessions.reserve(100);

	fd_set reads, writes;

	while (true)
	{
		// ���� �� �ʱ�ȭ (�� loop ����)
		FD_ZERO(&reads);
		FD_ZERO(&writes);

		// serverSocket ��� => Ŭ���̾�Ʈ ��û�̶�� ������ �����ϴ� �༮�̹Ƿ�
		// reads �� ���
		FD_SET(listenSocket, &reads);

		// ���� ���
		for (Session& s : sessions)
		{
			// �� �̷��� ? (�Ʒ� send �Լ� ����)
			if (s.recvBytes <= s.sendBytes)
				FD_SET(s.socket, &reads);
			else 
				FD_SET(s.socket, &writes);
		}

		// [�ɼ�] ������ timeout ���� ���� ����
		// timeval timeout;
		// timeout.tv_sec;
		// timeout.tv_usec;
		int32 retVal = ::select(0, &reads, &writes, nullptr, nullptr);

		// Error
		if (retVal == SOCKET_ERROR)
			break;

		// Listener ���� üũ
		if (FD_ISSET(listenSocket, &reads))
		{
			SOCKADDR_IN clientAddr;
			int32 addrLen = sizeof(clientAddr);
			SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
			
			if (clientSocket != SOCKET_ERROR)
			{
				cout << "Cient connected" << endl;
				sessions.push_back(Session({ clientSocket }));
			}
		}

		// ������ ������ read or write
		for (Session& s : sessions)
		{
			// Read
			if (FD_ISSET(s.socket, &reads))
			{
				int32 recvLen = ::recv(s.socket, s.recvBuffer, BUFSIZE, 0);
				
				// ���� ���� ��Ȳ => ���� session ���� 
				if (recvLen <= 0)
					continue;

				s.recvBytes = recvLen;
			}

			// Write
			if (FD_ISSET(s.socket, &writes))
			{
				// �۽� ���۰� ��������Ƿ�, �װ��� ������ �غ� �� ��
				// send �Լ��� ���ϰ� : ���� �������� ũ�⸦ ����
				// ���ŷ ���   => ��� ������ �� ����
				// ����ŷ ��� => �Ϻθ� ���� �� �ִ� (���� ���� ���� ��Ȳ�� ����)
				// ex) ���濡�� 100 ����Ʈ�� ������ ������, 10����Ʈ�� ������ ��Ȳ�� �� ���� �ִ�.
				//     ��, ó�� ��������, ��û�� ������ ���� ���� ũ���� �����͸� ���� ���� �ִٴ� �ǹ��̴�.
				int32 sendLen = ::send(s.socket, &s.recvBuffer[s.sendBytes], s.recvBytes - s.sendBytes, 0);
				
				// Error : session ����
				if (sendLen == SOCKET_ERROR)
					continue;
				
				s.sendBytes += sendLen;

				// ��� �����͸� �� ���� ����
				if (s.sendBytes == s.recvBytes)
				{
					s.recvBytes = 0;
					s.sendBytes = 0;
				}
			}

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