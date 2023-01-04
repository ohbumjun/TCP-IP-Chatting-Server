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

	// WSAEventSelect
	// - 소켓과 관련된 네트워크 이벤트를 [이벤트 객체]를 통해 감지
	// - 즉, 이벤트를 통해 통지를 받는다.

	// 이벤트 객체 관련 함수들
	// 생성 : WSACreateEvent (manual reset + non-signaled)
	// 삭제 : WSACloseEvent
	// 신호 상태 감지 : WSAWaitForMultipleEvents
	// 구체적인 네트워크 이벤트 알아내기 : WSAEnumNetworkdEvents

	// WSAEventSelect : 소켓 ~ 이벤트 연동시켜주는 함수 
	// - 즉, 소켓 개수 만큼 이벤트를 만들어줘야 한다.
	// WSAEventSelect(socket, event, networkEvents)
	// 관찰대상은 socket, 이벤트를 통해 감지 , 어떤 이벤트를 관찰할 것 ?
	
	// 관심있는 네트워크 이벤트
	// FD_ACCEPT  : 접속한 클라 있음 ? (accept)
	// FD_READ    : 데이터 수신 가능 ? (recv, recvfrom)
	// FD_WRITE   : 데이터 송신 가능 ? (send)
	// FD_CLOSE   : 상대가 접속 종료
	// FD_CONNECT : 통신을 위한 연결 절차 완료
	// FD_OOB

	// 주의 사항
	// - WSAEventSelect 함수를 호출하면, 해당 소켓은 자동으로 넌블로킹 모드로 전환
	//   즉, 처음에 블로킹 모드로 만들어두었다고 해도 자동으로 넌블로킹 모드로 !
	// - accept() 함수가 리턴하는 소켓은 listenSocket 과 동일한 속성을 갖는다. 즉, 넌블로킹 모드
	//   따라서 clientSocket 은 FD_READ, FD_WRITE 등을 다시 등록해야 한다.
	//   드물게 WSAEWOULDBLOCK 오류가 뜰 수 있으니 예외 처리 필요
	// - 이벤트 발생 시 , 적절한 소켓 함수 호출해야 한다.
	//   아니면 다음 번에는 동일 네트워크 이벤트가 발생 X
	//   ex) FD_READ 이벤트가 발생하면, recv 함수를 호출해야만
	//       다음 번에도 FD_READ 이벤트를 관찰할 수 있게 되는 것이다.
	//       즉, 감지를 해줬으니 꺼내써라, 꺼낼 쓸 때까지 감지 안해줌
	
	// WSAEventSelect, Select 함수 차이점 ?
	// - select 함수의 경우, 결과물 있을 때까지 기다렸다가 결과물을 뱉어준다.
	// - 반면 WSAEventSelect 는 소켓과 이벤트를 연동시켜주기만 한다.
	//   결과물을 얻는 것은, WSAWaitForMultipleEvents 를 통해 이벤트를 감지하여 얻는다.

	// WSAWaitForMultipleEvents
	// 1) count (이벤트 개수) , event(이벤트 포인터)
	// 2) waitAll : 모두 기다림 > 하나만 완료되어도 ok ?
	// 3) timeout : 타임아웃
	// 4) 지금은 false
	// return : 완료된 첫번째 Event 객체(오브젝트) idx

	// WSAEnumNetworkEvents : 완료된 이벤트 오브젝트는 알겠다. 이제 어떤 이벤트가 완료되었는지 확인
	// 1) socket
	// 2) eventObject :socket 과 연동된 이벤트 객체 핸들을 넘겨주면
	//    - 이벤트 객체를 non-signaled 상태로 만들어준다.
	// 3) networkEvents : 네트워크 이벤트 / 오류 정보가 저장

	vector<WSAEVENT> wsaEvents;
	vector<Session> sessions;
	wsaEvents.reserve(100);
	sessions.reserve(100);

	// listenSocket 을 위한 이벤트 객체
	WSAEVENT listenEvent = ::WSACreateEvent();
	wsaEvents.push_back(listenEvent);
	sessions.push_back(Session{ listenSocket });

	// socket, 이벤트 객체 연동 시키기
	if (::WSAEventSelect(listenSocket, listenEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
		return 0;

	while (true)
	{
		// 3번째 : false => 하나라도 완료되면 빠져나오기
		// 즉, signaled 상태가 된 EventObject 가 있다면 빠져나오는 것이다.
		int32 index = ::WSAWaitForMultipleEvents(wsaEvents.size(), &wsaEvents[0], FALSE, WSA_INFINITE, FALSE);
		
		// Error
		if (index == WSA_WAIT_FAILED)
			continue;
		
		index -= WSA_WAIT_EVENT_0;

		// 결과물 받아오기 
		WSANETWORKEVENTS networkEvents;

		// WSAEnumNetworkEvents 함수 호출 이후, 자동으로 wsaEvents[index] 는 non-signaled 로 바뀐다.
		if (::WSAEnumNetworkEvents(sessions[index].socket, wsaEvents[index], &networkEvents) == SOCKET_ERROR)
			continue;

		// Listener 소켓 체크
		if (networkEvents.lNetworkEvents & FD_ACCEPT)
		{
			// Error Check
			if (networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
				continue;

			// Accept 이벤트가 발생한 것이므로 accept 호출
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

		// Client Session 소켓 체크
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
				// 아무것도 받지 않은 상태
				int32 recvLen = ::recv(s.socket, s.recvBuffer, BUFSIZE, 0);

				if (recvLen == SOCKET_ERROR)
				{
					// 원래 블록했어야 했는데... 너가 논블로킹으로 하라며
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
				// 보낼 데이터가 남아있따면
				int32 sendLen = ::send(s.socket, &s.recvBuffer[s.sendBytes], s.recvBytes - s.sendBytes, 0);

				if (sendLen == SOCKET_ERROR && ::WSAGetLastError() != WSAEWOULDBLOCK)
				{
					// 실제 Error
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