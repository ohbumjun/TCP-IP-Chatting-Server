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

// Select 모델 
// - 장점 : 윈도우, 리눅스 공통
// - 단점 : 성능 최하 (매번 fd_set 에 등록), 64개 제한

// WSAEventSelect 모델
// - 장점 : 비교적 뛰어난다
// - 단점 : 64개 제한

// Overlapped (이벤트 기반)
// - 장점 : 성능
// - 단점 : 64개 제한, 이벤트 오브젝트 생성

// Overlapped (콜백 기반)
// - 장점 : 성능
// - 단점 : 모든 비동기 소켓 함수에서 사용가능 X
//          빈번한 Alertable Wait 으로 인한 성능 저하
//          ACP Queue 가 쓰레드 별로 있다 => 다른 쓰레드가 , 현재 쓰레드의 콜백 등을 처리할 수 없다.

// IOCP
// - 장점 :
// - 단점 : 

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

// 일감 실행 함수 
void WorkerThreadMain(HANDLE iocpHandle)
{
	// CP 계속 확인 -> 대기상태에 놓였다가, 일감이 생기면 
	// 운영체제가 쓰레드 하나를 깨워서 일을 처리하게 하는 방식
	while (true)
	{
		DWORD byteTransferred = 0;
		Session* session = nullptr;
		OverlappedEx* overlappedEx = nullptr;

		// iocp 에 쓰레드 할당
		// 1) &session : CreateIoCompletionPort 키값
		// 2) overlappedEx : WSARecv 함수 호출시 얻어낸 데이터
		BOOL ret = ::GetQueuedCompletionStatus(iocpHandle, &byteTransferred,
			(ULONG_PTR*)&session, (LPOVERLAPPED*)&overlappedEx, INFINITE);

		// 일감이 들어와서 운영체제가 쓰레드를 깨움
		if (ret == FALSE || byteTransferred == 0)
		{
			// 연결 끊김
			continue;
		}

		ASSERT_CRASH(overlappedEx->type == IO_TYPE::READ);

		cout << "New Data IOCP : " << byteTransferred << endl;

		// 일 처리하고, 이어나가서 recv 를 하고 싶다면 아래 부분 호출
		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUFSIZE;

		DWORD recvLen = 0;
		DWORD flags = 0;

		// 해당 socket 에 대한 recv, send 가 완료되었다는 통지가
		// CP 에 등록될 것이다.
		::WSARecv(session->socket, &wsaBuf, 1, &recvLen, &flags,
			&overlappedEx->overlapped, NULL);
	}
}

int main()
{
	WSADATA wsaData;

	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	// 블로킹 소켓 : 무언가 완료가 될 때까지 대기
	// accept -> 접속한 클라가 있을 때
	// connect -> 서버 접속 성공했을 때
	// send, sendTo -> 요청한 데이터를 송신 버퍼에 복사했을 때
	// recv, recvFrom -> 수신 버퍼에 도착한 데이터가 있고, 이를 정상적으로 읽어들였을 때


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

	// Overlapped IO (Completion Routing 콜백 기반)
	// - 비동기 입출력 함수 완료되면, 쓰레드마다 있는 APC 큐에 일감이 생김
	// - Alertable Wait 상태로 들어가서 APC 큐 비우기 (콜백 함수)
	// 단점) APC 큐가 쓰레드마다 있다 !. Alertable Wait 자체도 부담

	// IOCP(Completion Port 모델)
	// - 1) APC 대신, Completion Port 가 일감을 처리하는 것이다. 쓰레드마다 있는 것이 아니고, 1개만이 존재한다. 
	//   중앙에서 관리하는 APC Queue 처럼, 다수의 쓰레드가 1개의 Completion Port 를 통해 일감을 받아 처리한다.
	// - 2) Aleratable Wait => CP 결과 처리를 GetQueuedCompletionStatus

	vector<Session*> sessionManager;

	// CP 생성
	HANDLE iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// 일하는 쓰레드들 생성 => CP 관찰하면서, 완료된 IO 에 대한 정보를 받아 처리
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

		// Completion Port 에 소켓 등록
		// - Key 값 => 각 쓰레드에서 받을 정보
		CreateIoCompletionPort((HANDLE)clientSocket, iocpHandle, /*key*/(ULONG_PTR)session, 0);

		// 최초 Recv 는 호출해줘야 한다.
		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUFSIZE;

		OverlappedEx* overlappedEx = new OverlappedEx;
		overlappedEx->type = IO_TYPE::READ;

		DWORD recvLen = 0;
		DWORD flags = 0;

		// 해당 socket 에 대한 recv, send 가 완료되었다는 통지가
		// CP 에 등록될 것이다.
		::WSARecv(clientSocket, &wsaBuf, 1, &recvLen, &flags, 
			&overlappedEx->overlapped, NULL);

		// ::closesocket(session.socket);
		// ::WSACloseEvent(wsaEvent);
	}

	GThreadManager->Join();

	// 윈속 종료
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