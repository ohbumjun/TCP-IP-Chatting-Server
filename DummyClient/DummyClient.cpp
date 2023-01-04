#include "pch.h"
#include <iostream>
#include <WinSock2.h>
#include <winsock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

void HandleError(const char* cause)
{
	int32 errCode = ::WSAGetLastError();
	cout << cause << "ErrorCode : " << errCode << endl;
}

int main()
{
	WSAData wsaData;

	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	SOCKET clientSocket = ::socket(AF_INET, SOCK_STREAM, 0);

	if (clientSocket == INVALID_SOCKET)
		return 0;

	u_long on = 1;

	// 입출력 모드 변경 => 넌블로킹 방식
	if (::ioctlsocket(clientSocket, FIONBIO, &on) == INVALID_SOCKET)
		return 0;

	SOCKADDR_IN serverAddr;

	::memset(&serverAddr, 0, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	// 서버용 : serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);
	::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
	serverAddr.sin_port = ::htons(7777);

	// connect
	while(true)
	{
		if (::connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		{
			// 원래 블록했어야 했는데... 너가 논블로킹으로 하라며
			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;

			// 이미 연결된 상태
			if (::WSAGetLastError() == WSAEISCONN)
				break;

			// Error : 여기로 오면 진짜 에러가 났다는 의미
			break;
		}
	}

	cout << "connected To server " << endl;

	char sendBuffer[100] = "Hello World";

	// Send
	while (true)
	{
		if (::send(clientSocket, sendBuffer, sizeof(sendBuffer), 0) == SOCKET_ERROR)
		{
			// 원래 블록했어야 했는데... 너가 논블로킹으로 하라며
			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;

			// Error
			break;
		}

		cout << "Send Data Len : " << sizeof(sendBuffer) << endl;

		// 에코이므로 상대방이 다시 돌려준 데이터를 받기
		while (true)
		{
			char recvBuffer[1000];

			int32 recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);

			if (recvLen == SOCKET_ERROR)
			{
				// 원래 블록했어야 했는데... 너가 논블로킹으로 하라며
				if (::WSAGetLastError() == WSAEWOULDBLOCK)
					continue;

				// Error
				break;
			}

			// 연결 끊김
			else if (recvLen == 0)
			{
				break;
			}

			cout << "Recv Data Len = " << recvLen << endl;

			break;
		}

		this_thread::sleep_for(1s);
	}

	// 소켓 리소스 닫기
	::closesocket(clientSocket);

	// 종료
	::WSACleanup();

	return 0;
}