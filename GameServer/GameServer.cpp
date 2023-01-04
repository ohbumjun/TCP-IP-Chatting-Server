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

// Select �� 
// - ���� : ������, ������ ����
// - ���� : ���� ���� (�Ź� fd_set �� ���), 64�� ����

// WSAEventSelect ��
// - ���� : ���� �پ��
// - ���� : 64�� ����

// Overlapped (�̺�Ʈ ���)
// - ���� : ����
// - ���� : 64�� ����, �̺�Ʈ ������Ʈ ����

// Overlapped (�ݹ� ���)
// - ���� : ����
// - ���� : ��� �񵿱� ���� �Լ����� ��밡�� X
//          ����� Alertable Wait ���� ���� ���� ����
//          ACP Queue �� ������ ���� �ִ� => �ٸ� �����尡 , ���� �������� �ݹ� ���� ó���� �� ����.

// IOCP
// - ���� :
// - ���� : 

const int32 BUFSIZE = 1000;

struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUFSIZE];
	int32 recvBytes = 0;
};

enum IO_TYPE
{
	READ,
	WRITE,
	ACCEPT,
	CONNECT
};

struct OverlappedEx
{
	WSAOVERLAPPED overlapped = {};
	int type = 0; // io_type
};

void HandleError(const char* cause)
{
	int32 errCode = ::WSAGetLastError();
	cout << cause << "ErrorCode : " << errCode << endl;
};

// �ϰ� ���� �Լ� 
void WorkerThreadMain(HANDLE iocpHandle)
{
	// CP ��� Ȯ�� -> �����¿� �����ٰ�, �ϰ��� ����� 
	// �ü���� ������ �ϳ��� ������ ���� ó���ϰ� �ϴ� ���
	while (true)
	{
		DWORD byteTransferred = 0;
		Session* session = nullptr;
		OverlappedEx* overlappedEx = nullptr;

		// iocp �� ������ �Ҵ�
		// 1) &session : CreateIoCompletionPort Ű��
		// 2) overlappedEx : WSARecv �Լ� ȣ��� �� ������
		BOOL ret = ::GetQueuedCompletionStatus(iocpHandle, &byteTransferred,
			(ULONG_PTR*)&session, (LPOVERLAPPED*)&overlappedEx, INFINITE);

		// �ϰ��� ���ͼ� �ü���� �����带 ����
		if (ret == FALSE || byteTransferred == 0)
		{
			// ���� ����
			continue;
		}

		ASSERT_CRASH(overlappedEx->type == IO_TYPE::READ);

		cout << "New Data IOCP : " << byteTransferred << endl;

		// �� ó���ϰ�, �̾���� recv �� �ϰ� �ʹٸ� �Ʒ� �κ� ȣ��
		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUFSIZE;

		DWORD recvLen = 0;
		DWORD flags = 0;

		// �ش� socket �� ���� recv, send �� �Ϸ�Ǿ��ٴ� ������
		// CP �� ��ϵ� ���̴�.
		::WSARecv(session->socket, &wsaBuf, 1, &recvLen, &flags,
			&overlappedEx->overlapped, NULL);
	}
}

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


	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);

	if (listenSocket == INVALID_SOCKET)
		return 0;

	u_long on = 1;

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

	// Overlapped IO (Completion Routing �ݹ� ���)
	// - �񵿱� ����� �Լ� �Ϸ�Ǹ�, �����帶�� �ִ� APC ť�� �ϰ��� ����
	// - Alertable Wait ���·� ���� APC ť ���� (�ݹ� �Լ�)
	// ����) APC ť�� �����帶�� �ִ� !. Alertable Wait ��ü�� �δ�

	// IOCP(Completion Port ��)
	// - 1) APC ���, Completion Port �� �ϰ��� ó���ϴ� ���̴�. �����帶�� �ִ� ���� �ƴϰ�, 1������ �����Ѵ�. 
	//   �߾ӿ��� �����ϴ� APC Queue ó��, �ټ��� �����尡 1���� Completion Port �� ���� �ϰ��� �޾� ó���Ѵ�.
	// - 2) Aleratable Wait => CP ��� ó���� GetQueuedCompletionStatus

	vector<Session*> sessionManager;

	// CP ����
	HANDLE iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// ���ϴ� ������� ���� => CP �����ϸ鼭, �Ϸ�� IO �� ���� ������ �޾� ó��
	for (int32 i = 0; i < 5; ++i)
		GThreadManager->Launch([=]() {WorkerThreadMain(iocpHandle); });

	while (true)
	{
		SOCKADDR_IN clientAddr;
		int32 addrLen = sizeof(clientAddr);

		SOCKET clientSocket;
		clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

		if (clientSocket == INVALID_SOCKET)
			return 0;

		Session* session = new Session;
		session->socket = clientSocket;
		sessionManager.push_back(session);

		cout << "Client Connected" << endl;

		// Completion Port �� ���� ���
		// - Key �� => �� �����忡�� ���� ����
		CreateIoCompletionPort((HANDLE)clientSocket, iocpHandle, /*key*/(ULONG_PTR)session, 0);

		// ���� Recv �� ȣ������� �Ѵ�.
		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUFSIZE;

		OverlappedEx* overlappedEx = new OverlappedEx;
		overlappedEx->type = IO_TYPE::READ;

		DWORD recvLen = 0;
		DWORD flags = 0;

		// �ش� socket �� ���� recv, send �� �Ϸ�Ǿ��ٴ� ������
		// CP �� ��ϵ� ���̴�.
		::WSARecv(clientSocket, &wsaBuf, 1, &recvLen, &flags, 
			&overlappedEx->overlapped, NULL);

		// ::closesocket(session.socket);
		// ::WSACloseEvent(wsaEvent);
	}

	GThreadManager->Join();

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