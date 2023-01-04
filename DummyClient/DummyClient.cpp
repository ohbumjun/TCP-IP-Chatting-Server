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
	// server �߱� ���� �������� �ʵ��� ���
	this_thread::sleep_for(2s);

	WSAData wsaData;

	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	SOCKET clientSocket = ::socket(AF_INET, SOCK_STREAM, 0);

	if (clientSocket == INVALID_SOCKET)
		return 0;

	u_long on = 1;

	// ����� ��� ���� => �ͺ��ŷ ���
	if (::ioctlsocket(clientSocket, FIONBIO, &on) == INVALID_SOCKET)
		return 0;

	SOCKADDR_IN serverAddr;

	::memset(&serverAddr, 0, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	// ������ : serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);
	::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
	serverAddr.sin_port = ::htons(7777);

	// connect
	while(true)
	{
		if (::connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		{
			// ���� ����߾�� �ߴµ�... �ʰ� ����ŷ���� �϶��
			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;

			// �̹� ����� ����
			if (::WSAGetLastError() == WSAEISCONN)
				break;

			// Error : ����� ���� ��¥ ������ ���ٴ� �ǹ�
			break;
		}
	}

	cout << "connected To server " << endl;



	// Send
	while (true)
	{
		char sendBuffer[100] = "Hello World";
		WSAEVENT wsaEvent = ::WSACreateEvent();
		WSAOVERLAPPED overlapped = {};
		overlapped.hEvent = wsaEvent;

		WSABUF wsaBuf;
		wsaBuf.buf = sendBuffer;
		wsaBuf.len = 100;

		DWORD sendLen = 0;
		DWORD flags = 0;

		if (::WSASend(clientSocket, &wsaBuf, 1, &sendLen, flags, &overlapped, nullptr) == SOCKET_ERROR)
		{
			if (::WSAGetLastError() == WSA_IO_PENDING)
			{
				// Pending
				::WSAWaitForMultipleEvents(1, &wsaEvent, TRUE, WSA_INFINITE, FALSE);
				::WSAGetOverlappedResult(clientSocket, &overlapped, &sendLen, FALSE, &flags);
			}
			else
			{
				// ERROR
				break;
			}
		}

		cout << "Send Data : " << sendLen << endl;

		this_thread::sleep_for(1s);
	}

	cout << "END" << endl;

	// ���� ���ҽ� �ݱ�
	::closesocket(clientSocket);

	// ����
	::WSACleanup();

	return 0;
}