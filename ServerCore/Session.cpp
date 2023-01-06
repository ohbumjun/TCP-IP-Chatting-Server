#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "Service.h"

/*--------------
	Session
---------------*/

Session::Session() : 
	_recvBuffer{}
{
	// tcp ���� ����
	_socket = SocketUtils::CreateSocket();
}

Session::~Session()
{
	SocketUtils::Close(_socket);
}

void Session::Disconnect(const WCHAR* cause)
{
	// false �� �����ϱ� ������ ���� �̹� false ���
	// �̹� ������ ���� �����̹Ƿ�, return
	if (_connected.exchange(false) == false)
		return;

	// TEMP
	wcout << "Disconnect : " << cause << endl;

	OnDisconnected(); // ������ �ڵ忡�� �����ε�
	SocketUtils::Close(_socket);

	// ref Cnt ���ҽ�Ű��
	GetService()->ReleaseSession(GetSessionRef());
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

// iocpEvent �� ���� recv, Ȥ�� send ���� event �� �߻���Ų��.
// �̰��� ó���� �� Dispatch �� ó���Ѵٴ� ���̴�.
// ��, Thread ���� iocpCore �� Dispatch �� ȣ���ϸ鼭 �ϰ��� �����
// iocpObject->Dispatch ȣ�� => Session->Dispatch ȣ��
void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	// TODO
	switch (iocpEvent->eventType)
	{
	case EventType::Connect:
		ProcessConnect();
		break;
	case EventType::Recv:
		ProcessRecv(numOfBytes);
		break;
	case EventType::Send:
		ProcessSend(numOfBytes);
		break;
	default:
		break;
	}
}

void Session::RegisterConnect()
{
}

void Session::RegisterRecv()
{
	// ��¥ ������ ����ٰų�
	// ���������� �ش� Ŭ���̾�Ʈ�� ��ŷ�ǽɵǾ� �����������
	if (IsConnected() == false)
		return;

	// RecvEvent �� ������ش�.
	_recvEvent.Init();
	// - ref cnt �� 1 �� ���ش�.
	_recvEvent.owner = shared_from_this();

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer);
	wsaBuf.len = len32(_recvBuffer);

	DWORD numOfBytes = 0;
	DWORD flags      = 0;

	// �Ʒ��� �񵿱� Recv �� �Ϸ�ǰ� �Ǹ�, ��������� ProcessRecv() �Լ� ȣ��
	if (SOCKET_ERROR == ::WSARecv(_socket, &wsaBuf, 1,
		OUT & numOfBytes, OUT &flags, &_recvEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();

		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);

			// Release Ref (shared ptr --)
			_recvEvent.owner = nullptr;
		}
	}
}

void Session::RegisterSend()
{
}

void Session::ProcessConnect()
{
	// ��Ƽ������ ȯ��
	_connected.store(true);

	// �ڽ��� ������ �ִ� ���񽺿� �ڱ� �ڽ� (����) ���
	GetService()->AddSession(GetSessionRef());

	// ������ �ڵ忡�� �����ε�
	OnConnected();

	// ���� ��� => �񵿱� �Լ� WSARecv() ȣ�� -> IO �� �Ϸ�Ǹ�
	// iocpCore->Dispatch => iocpObject->Dispatch => session->Dispatch
	// => ProcessRecv() �Լ� ȣ��
	RegisterRecv();
}

void Session::ProcessRecv(int32 numOfBytes)
{
	_recvEvent.owner = nullptr; 

	// ���ŵ� �����Ͱ� ���ٴ� �ǹ̴�, ������ ����ٴ� �ǹ��̴�.
	if (numOfBytes == 0)
	{
		Disconnect(L"Recv 0");
		return;
	};

	// TODO
	cout << "Recv Data Len = " << numOfBytes << endl;

	// ���� ��� => ��, �ٽ� �ѹ� ����ؼ� Recv
	RegisterRecv();
}

void Session::ProcessSend(int32 numOfBytes)
{
}

void Session::HandleError(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
		Disconnect(L"HandleError");
		break;
	default:
		// TODO : Log
		cout << "Handle Error : " << errorCode << endl;
		break;
	}
}
