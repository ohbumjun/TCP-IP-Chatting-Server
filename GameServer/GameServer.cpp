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
	WSAOVERLAPPED overlapped = {};
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUFSIZE];
	int32 recvBytes = 0;
	int32 sendBytes = 0;
};

void HandleError(const char* cause)
{
	int32 errCode = ::WSAGetLastError();
	cout << cause << "ErrorCode : " << errCode << endl;
};

// 1) 오류 발생시 0이 아닌 값
// 2) 전송 바이트 수
// 3) 비동기 입출력 함수 호출 시 넘겨준 WSAOverlapped 구조체의 주소값
void RecvCallback(DWORD error, DWORD recvLen, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	cout << "Data Recv Len Callback : " << recvLen << endl;

	// Session 의 첫번째 요소를 overlapped 로 하면
	// 메모리구조상 Session 으로 캐스팅이 가능하다
	Session* session = (Session*)overlapped;

	// TODO : 에코서버 ? WSASend 를 해주면 된다.
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

	// Overlapped IO (비동기 + 논블로킹 함수)
	// - Overlapped 함수를 건다 (WSARecv, WSASend)
	// - 함수가 성공했는지 확인 후
	//   성공하면, 결과 얻어서 처리
	//   실패했으면, 사유 확인
	//       IO 함수가 완료되지 않으면 차후 알려달라고 하기 (완료 통지 받기)
	//       1) 이벤트 이용
	//       2) Callback 사용

	// WSASend
	// WSARecv
	// AcceptEx
	// ConnectEx

	char sendBuffer[100];
	WSABUF wsaBuf;
	wsaBuf.buf = sendBuffer;
	wsaBuf.len = 100;

	// Overlapped 모델 (Completion Routine 기반)
	// - 비동기 입출력 지원하는 소켓 생성 
	// - 콜백함수 생성
	// - 비동기 입출력 함수 호출 
	// - 비동기 작업이 바로 완료되지 않으면 WSA_IO_PENDING 오류 코드
	// - 비동기 입출력 함수 호출 쓰레드를 Alertable Wait 상태로 만든다 => 완료루틴 호출
	// - 완료루틴 호출 끝나면 쓰레드는 Alertable Wait 상태에서 빠져나온다.

	// 상대적 장점 : 이벤트 기반과 달리, 콜백함수만 지정해주면 되므로
	//               클라이언트 개수만큼의 이벤트를 생성해줄 필요가 없다.

	while (true)
	{
		SOCKADDR_IN clientAddr;
		int32 addrLen = sizeof(clientAddr);

		SOCKET clientSocket;
		while (true)
		{
			// clientSocket 는 넌블로킹 소켓이 될 것이다.
		// 즉, 클라이언트 연결 요청이 없어도 return
			clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

			// Connected
			if (clientSocket != SOCKET_ERROR)
				break;

			// 아직 연결 요청 X
			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;

			// 문제 있는 상황
			return 0;
		}

		Session session = Session{ clientSocket };
		WSAEVENT wsaEvent = ::WSACreateEvent();
		session.overlapped.hEvent = wsaEvent;

		cout << "Client Connected" << endl;

		while(true)
		{
			// WSABUF wsaBuf 는 메모리 해제되어도 상관 없다
			// 핵심은 recvBuffer 이다. 이것은 건드리면 안된다.
			WSABUF wsaBuf;
			wsaBuf.buf = session.recvBuffer;
			wsaBuf.len = BUFSIZE;

			DWORD recvLen = 0;
			DWORD flags = 0;

			if (::WSARecv(clientSocket, &wsaBuf, 1, &recvLen, &flags, &session.overlapped, RecvCallback) == SOCKET_ERROR)
			{
				if (::WSAGetLastError() == WSA_IO_PENDING)
				{
					// Pending (recv 가 완료되지 않았다면)
					// Alertable Wait 상태로 바꿔주기
					// 1) 마지막 인자 TRUE ? Alertable Wait 으로 ! => ::WSAWaitForMultipleEvents(1, &wsaEvent, TRUE, WSA_INFINITE, TRUE);
					// 2) SleepEx(INFINITE, TRUE) => SleepEx : 지정된 조건이 충족될 때까지 현재 쓰레드 중단
					//            INFINITE, TRUE => 제한시간(INFINIT)가 경과하거나, I/O 완료 콜백함수가 발생할 때 함수 반환

					// (참고) 쓰레드마다 APC Queue 라 하여, Aleratable Wait 상태로 바꿀 때
					//        호출할 함수를 모아둔다.
					// 
					SleepEx(INFINITE, TRUE);

					cout << "After SleepEx : " << recvLen << endl;
				}
				else
				{
					// 문제 있는 상황
					break;
				}
			}
			else
			{
				cout << "Data Recv Len : " << recvLen << endl;
			}

		}

		::closesocket(session.socket);
		::WSACloseEvent(wsaEvent);
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