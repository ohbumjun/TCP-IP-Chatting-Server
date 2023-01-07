#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "Service.h"

/*--------------
	Session
---------------*/

Session::Session() : 
	_recvBuffer(BUFFER_SIZE)
{
	// tcp ���� ����
	_socket = SocketUtils::CreateSocket();
}

Session::~Session()
{
	SocketUtils::Close(_socket);
}

void Session::Send(BYTE* buffer, int32 len)
{
	// ������ ����
	// 1) ���� ���� 
	//    => �� ������ ���� Send �Լ��� ȣ���Ͽ� Write_LOCK�� �ɰ�
	//    => RegisterSend �� ȣ���Ͽ� WSASend �� ȣ���ϴ� ���� ��ȿ�����̴�
	//    => �����͸� �ѹ��� ��Ƶΰ�, �� �ѹ��� �� ������ ����� �� ȿ���� 
	//       (Scatter Gather)
	// 2) sendEvent ����? ����? ������? WSASend ��ø?

	// send �� �ѹ��� �ѹ��� ���ʷ� ��Ű�鼭 ȣ���ϴ� ���� �ƴ϶�
	// �������� ȣ���ϰ� �� ���̴�. �ߺ��ؼ� ���� �� ȣ��� ���� �ִ�.
	// ���� _recvEvent �� ���� �Ѱ��� �����ϴ� �������� �����ϸ� �ȵȴ�.
	// �Ź� �������� �������ִ� ����� �ִ�.
	// ex) Ŭ���̾�Ʈ�� ���� ��� => ���� ������ ����ؼ� ���� ���̴�.

	SendEvent* sendEvent = new SendEvent;
	sendEvent->owner = shared_from_this(); // ADD_REF
	sendEvent->buffer.resize(len);
	::memcpy(sendEvent->buffer.data(), buffer, len);

	// RegisterSend ���� ȣ��Ǵ� WSASend �Լ��� ��� ��Ƽ������κ��� �������� �ʴ�
	// ���� WRITE_LOCK �� ȣ�����־�� �Ѵ�.
	WRITE_LOCK;

	RegisterSend(sendEvent);
}

// ���� ���� �����ϴ� ��찡 ���� �� �ִ�.
// �׷��� Session �� �̿��ؼ� ���� ������ �ٴ� �۾��� �ؾ� �� ���� �ִ�.
bool Session::Connect()
{
	return RegisterConnect();
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

	RegisterDisconnect();
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
	case EventType::Disconnect:
		ProcessDisconnect();
		break;
	case EventType::Recv:
		ProcessRecv(numOfBytes);
		break;
	case EventType::Send:
		ProcessSend(static_cast<SendEvent*>(iocpEvent), numOfBytes);
		break;
	default:
		break;
	}
}

bool Session::RegisterDisconnect()
{
	_disconnectEvent.Init();
	_disconnectEvent.owner = shared_from_this();

	// �񵿱�� Disconnect ��û
	if (false == SocketUtils::DisconnectEx(_socket, &_disconnectEvent, TF_REUSE_SOCKET, 0))
	{
		// TF_REUSE_SOCKET : ������ �ٽ� ����� �� �ְ� �غ����ش�.
		int32 errorCode = ::WSAGetLastError();

		if (errorCode != WSA_IO_PENDING)
		{
			_disconnectEvent.owner = nullptr;
			return false;
		}
	}

	return true;
}

bool Session::RegisterConnect()
{
	if (IsConnected())
		return false;

	// �ݵ�� Client ���� �Ѵ�.
	// Server �� ���, ������ ������ �ٴ� ����̹Ƿ� RegisterConnect �� ȣ�� X
	if (GetService()->GetSeviceType() != ServiceType::Client)
		return false;

	// �ּ� ����
	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	// ���Ͽ� �ּ�, port ���� => 0 �� �Ѱ��ָ� �����ִ°� �ƹ��ų� �˾Ƽ� ��������
	// ConnectEx �� ȣ�����ֱ� ���ؼ��� �ݵ�� �ʿ��ϴ�.
	if (SocketUtils::BindAnyAddress(_socket, 0) == false)
		return false;

	_connectEvent.Init();
	_connectEvent.owner = shared_from_this(); 

	// ���� ������ ���ΰ�
	DWORD numOfBytes = 0;
	SOCKADDR_IN sockAddr = GetService()->GetNetAddress().GetSockAddr();
	
	if (SocketUtils::ConnectEx(_socket, reinterpret_cast<SOCKADDR*>(&sockAddr),
		sizeof(sockAddr), nullptr, 0, &numOfBytes, &_connectEvent) == false)
	{
		int32 errorCode = ::WSAGetLastError();

		if (errorCode != WSA_IO_PENDING)
		{
			_connectEvent.owner = nullptr;
			return false;
		}
	}

	return true;
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

	// ��Ŷ�� ����ü�� �Դ��� �ȿԴ��� �����ؾ� �Ѵ�.
	// - TCP Ư���� ������ ��谡 ���� �����̴�.
	// - �̸� RecvBuffer �� ���� ����
	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.WritePos());

	// wsaBuf.len : ���� ���� ��ü�� �ִ�� ���� �� �ִ� ũ��
	wsaBuf.len = _recvBuffer.FreeSize();

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

// �Ʒ� WSASend ��� �񵿱� �Լ��� �Ϸ�Ǹ�
// ProcessSend ��� �Լ� ������ �Ѿ�� �� ���̴�.
void Session::RegisterSend(SendEvent* sendEvent)
{
	if (IsConnected() == false)
		return;

	WSABUF wsaBuf;
	wsaBuf.buf = (char*)sendEvent->buffer.data();
	wsaBuf.len = (ULONG)sendEvent->buffer.size();

	DWORD numOfBytes = 0;

	if (SOCKET_ERROR == ::WSASend(_socket, &wsaBuf, 1, OUT & numOfBytes, 0, sendEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();

		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);

			sendEvent->owner = nullptr;

			if (sendEvent)
				delete sendEvent;
		}
	}
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

void Session::ProcessDisconnect()
{
	_disconnectEvent.owner = nullptr;
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

	// ���� ���ۿ� ������ ����
	if (_recvBuffer.OnWrite(numOfBytes) == true)
	{
		Disconnect(L"On Write Overflow");
		return;
	}

	// ���ݱ��� ���� ������ ũ�� ���� (���� ������ ũ��)
	int32 dataSize = _recvBuffer.DataSize();

	// Ŭ���̾�Ʈ �����ε� �ڵ�
	// readPos �κ��� numOfBytes ������ ���� ���ݱ����� ���� ������ ũ��
	// ���� ��� �����͸� ó���ϰ� �� ���� �ְ�
	// ������ ��迡 ���� �Ϻ� �����͸� ó���ϰ� �� ���� �ִ�.
	// processLen : dataSize �߿��� ���� ó���� ������ ũ�� 
	int32 processLen = OnRecv(_recvBuffer.ReadPos(), dataSize);

	// �޾Ƶ��� ������ �߿���, ���� ó���� ������ ũ�⸸ŭ OnRead ����
	if (processLen < 0 || dataSize < processLen || _recvBuffer.OnRead(processLen) == false)
	{
		Disconnect(L"On Read Overflow");
		return;
	}

	// �� �������
	// 1) OnRecv �� ���� WritePos �� ����
	// 2) OnRead �� ���� ReadPos ����
	// ���������� Ŀ�� ������ ���ش�.
	_recvBuffer.Clean();

	// ���� ��� => ��, �ٽ� �ѹ� ����ؼ� Recv
	RegisterRecv();
}

// �ش� �Լ��� ���� ���� �����忡�� ������ ������� ���� ä ����� �� �ִ�.
// �ֳ��ϸ� iocpCore->Dispatch �Լ��� ���� �Ʒ� �Լ��� ���� ����Ǵµ�
// �̶� A,B,C,D ������ send �ص� , �� ������� ��������� �ʴ� ���̴�(send �Ϸ� ���� ���� X)
void Session::ProcessSend(SendEvent* sendEvent, int32 numOfBytes)
{
	// ������ �÷����� ref �� 1 ���� (���� �� �ô�)
	sendEvent->owner = nullptr; // RELEASE_REF
	delete(sendEvent);

	if (numOfBytes == 0)
	{
		Disconnect(L"Send 0");
		return;
	}

	// ������ �ڵ忡�� �����ε�
	OnSend(numOfBytes);
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
