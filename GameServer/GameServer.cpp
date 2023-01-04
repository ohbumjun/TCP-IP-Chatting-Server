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

	// 블로킹 소켓 : 무언가 완료가 될 때까지 대기
	// accept -> 접속한 클라가 있을 때
	// connect -> 서버 접속 성공했을 때
	// send, sendTo -> 요청한 데이터를 송신 버퍼에 복사했을 때
	// recv, recvFrom -> 수신 버퍼에 도착한 데이터가 있고, 이를 정상적으로 읽어들였을 때

	// 논블로킹 소켓(Non-Blocking)

	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);

	if (listenSocket == INVALID_SOCKET)
		return 0;

	u_long on = 1;

	// 입출력 모드 변경 => 넌블로킹 방식
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
	// 소켓 함수 호출이 성공할 시점을 미리 알 수 있다.
	// (블로킹, 논블로킹 모두 적용가능)
	
	// 소켓 함수 호출이 성공한다 ? 
	// 문제상황 1) 수신 버퍼에 데이터가 없는데 read 한다거나
	// 문제상황 2) 송신 버퍼가 꽉 찼는데 write 한다거나
	
	// - 블로킹 소켓   : 조건이 만족되지 않아서 블로킹 되는 상황 예방
	// - 논블로킹 소켓 : 조건이 만족되지 않아서 불필요하게 반복 체크하는 상황 예방
	// 즉, send, recv 함수 호출 이전에 실제 send, recv 할 수 있는지 체크

	// socket set
	// 1) 읽기(읽을 준비 된 녀석이 있는가, 데이터 받은 녀석이 있는가) [], 
	// 2) 쓰기(데이터를 보낼 준비가 된 녀석이 있는가) [], 
	// 3) 예외(ex. Out of Band) [] 관찰 대상 등록

	// > select(readSet, writeSet, exceptSet) 
	// > 적어도 하나의 소켓이 준비되면 리턴 => 낙오자는 알아서 제거 (비트 정보 0으로 만든다. 변화 있는 녀석만 1로 남는다)
	// > 남은 소켓 체크해서 진행

	// fd_set read
	// FD_ZERO : 비운다.
	// ex) FD_ZERO(set)
	// FD_SET : 소켓 s 를 넣는다 (관찰대상에 추가)
	// ex) FD_SET(s, &set);
	// FD_CLR : 소켓 s 제거
	// ex) FD_CLR(s, &set);
	// FD_ISSET : 소켓 s 가 set 에 들어있으면 0이 아닌 값을 리턴한다.
	// ex)

	vector<Session> sessions;
	sessions.reserve(100);

	fd_set reads, writes;

	while (true)
	{
		// 소켓 셋 초기화 (매 loop 마다)
		FD_ZERO(&reads);
		FD_ZERO(&writes);

		// serverSocket 등록 => 클라이언트 요청이라는 데이터 수신하는 녀석이므로
		// reads 에 등록
		FD_SET(listenSocket, &reads);

		// 소켓 등록
		for (Session& s : sessions)
		{
			// 왜 이렇게 ? (아래 send 함수 참조)
			if (s.recvBytes <= s.sendBytes)
				FD_SET(s.socket, &reads);
			else 
				FD_SET(s.socket, &writes);
		}

		// [옵션] 마지막 timeout 인자 설정 가능
		// timeval timeout;
		// timeout.tv_sec;
		// timeout.tv_usec;
		int32 retVal = ::select(0, &reads, &writes, nullptr, nullptr);

		// Error
		if (retVal == SOCKET_ERROR)
			break;

		// Listener 소켓 체크
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

		// 나머지 소켓은 read or write
		for (Session& s : sessions)
		{
			// Read
			if (FD_ISSET(s.socket, &reads))
			{
				int32 recvLen = ::recv(s.socket, s.recvBuffer, BUFSIZE, 0);
				
				// 연결 끊긴 상황 => 차후 session 제거 
				if (recvLen <= 0)
					continue;

				s.recvBytes = recvLen;
			}

			// Write
			if (FD_ISSET(s.socket, &writes))
			{
				// 송신 버퍼가 비어있으므로, 그곳에 복사할 준비가 된 것
				// send 함수의 리턴값 : 보낸 데이터의 크기를 리턴
				// 블로킹 모드   => 모든 데이터 다 보냄
				// 논블로킹 모드 => 일부만 보낼 수 있다 (상대방 수신 버퍼 상황에 따라)
				// ex) 상대방에게 100 바이트를 보내야 하지만, 10바이트만 보내는 상황이 될 수도 있다.
				//     즉, 처음 보내야할, 요청한 데이터 보다 작은 크기의 데이터를 보낼 수도 있다는 의미이다.
				int32 sendLen = ::send(s.socket, &s.recvBuffer[s.sendBytes], s.recvBytes - s.sendBytes, 0);
				
				// Error : session 제거
				if (sendLen == SOCKET_ERROR)
					continue;
				
				s.sendBytes += sendLen;

				// 모든 데이터를 다 보낸 상태
				if (s.sendBytes == s.recvBytes)
				{
					s.recvBytes = 0;
					s.sendBytes = 0;
				}
			}

		}


	}

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