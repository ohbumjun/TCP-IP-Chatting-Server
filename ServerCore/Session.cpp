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

/*
void Session::Send(BYTE* buffer, int32 len)
{
	SendEvent* sendEvent = new SendEvent;
	sendEvent->owner = shared_from_this(); // ADD_REF
	sendEvent->buffer.resize(len);

	=> �Ʒ��� ���� �Ź� �����ϴ� ���� ������ �� �� �ִ�.
	������ ���, Ư�� Ŭ���̾�Ʈ ������ ������, �ٸ� �����鿡�� �ѷ��ִ� ���°� �ȴ�.
	�׷��� ��� �������� �̿� ���� ���縦 ����� �Ѵٴ� ���ε� �̴� ������ �� �� �ִ�.
	::memcpy(sendEvent->buffer.data(), buffer, len);

	// RegisterSend ���� ȣ��Ǵ� WSASend �Լ��� ��� ��Ƽ������κ��� �������� �ʴ�
	// ���� WRITE_LOCK �� ȣ�����־�� �Ѵ�.
	WRITE_LOCK;

	RegisterSend();
}
*/

void Session::Send(SendBufferRef sendBuffer)
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
	// ex) Ŭ���̾�Ʈ�� ���� ��� => ���� ������ ����ؼ� ���� ���̴�.

	// ���� RegisterSend �� �ɸ��� ���� ���¶�� �ɾ��ش�.
	// - RegisterSend => ProcessSend ȣ�� ����, ���������� �ٽ� RegisterSend ȣ��
	// - ��, WSARecv �� ������, �� ���� WSARecv �� ȣ���ϰ��� �Ѵ�.
	// - �̸� ���� �ѹ��� �����͸� ��Ƽ� ��Ŷ�� �ּ������� ���� �� �ִ�.
	// - ���� ���� Send �Լ��� �����Ϸ��� �ߴ���, ������ SendEvent �� ������ �ʾҴٸ�
	// - Queue<SendBufferRef> �� �����ϴ� ���·� ������ ���̴�.

	if (IsConnected() == false)
		return;

	bool registered = false;

	// Register �� �Ȱɸ� ���¿��� �ɾ��ش�.
	{
		// RegisterSend ���� ȣ��Ǵ� WSASend �Լ��� ��� ��Ƽ������κ��� �������� �ʴ�
	// ���� WRITE_LOCK �� ȣ�����־�� �Ѵ�.
		WRITE_LOCK;

		// sendQueue �� �����ϰ��� �ϴ� �����͸� �о�־��ش�.
		_sendQueue.push(sendBuffer);

		// ������ ���� false ���ٸ�
		if (_sendRegistered.exchange(true) == false)
		{
			registered = true;
		}
	}

	if (registered)
	{
		// RegisterSend() ������ _sendQueue �� �ִ� �����͸� ������ ���� ���̴�.
		RegisterSend();
	}
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

	/*
	�Ʒ� �Լ� : ProcessDisconnect �� �̵� => �� ? GameSessionManager::BroadCast ����  
	> OnDisconnected(); // ������ �ڵ忡�� �����ε�

	// ref Cnt ���ҽ�Ű��
	> GetService()->ReleaseSession(GetSessionRef());
	*/

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
		ProcessSend(numOfBytes);
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
	
	if (false == SocketUtils::ConnectEx(_socket,
		reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr), nullptr, 0, &numOfBytes, &_connectEvent))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_connectEvent.owner = nullptr; // RELEASE_REF
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
	BYTE* WritePos = _recvBuffer.WritePos();

	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.WritePos());

	// wsaBuf.len : ���� ���� ��ü�� �ִ�� ���� �� �ִ� ũ��
	wsaBuf.len = _recvBuffer.FreeSize();

	DWORD numOfBytes = 0;
	DWORD flags      = 0;

	// �Ʒ��� �񵿱� Recv �� �Ϸ�ǰ� �Ǹ�, ��������� ProcessRecv() �Լ� ȣ��
	if (SOCKET_ERROR == ::WSARecv(_socket, &wsaBuf, 1, OUT & numOfBytes, OUT & flags, &_recvEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_recvEvent.owner = nullptr; // RELEASE_REF
		}
	}
}

// �Ʒ� WSASend ��� �񵿱� �Լ��� �Ϸ�Ǹ�
// ProcessSend ��� �Լ� ������ �Ѿ�� �� ���̴�.
// - ���� �츮�� ���� ��Ŀ����� �ѹ��� �� �����常 �����ϰ� �� ���̴�.
void Session::RegisterSend()
{
	if (IsConnected() == false)
		return;

	// IOCP �� WSASend �Լ��� ȣ���� �غ� ���ش�.
	_sendEvent.Init();
	_sendEvent.owner = shared_from_this();

	// Send() �Լ����� _sendQueud �� �Ѱ��� �����͸� ������ ����� ���̴�.
	// ���� �����͸� sendEvent �� ���
	// ��, sendEvent Ŭ������ �ִ� sendBuffer �� �Ű��شٴ� ���̴�.
	// �� ? WSASend �� �ϴ� ���� �ش� sendBuffer �� �������� �ʰ�
	// Ref Cnt �� ����������� �Ѵ�.
	// �� Queue �� ���� ���� Ref Cnt �� 1 �����Ͽ� ����� �� �ֱ� ������
	// �ѹ� �� ���⼭ �������ִ� ���̴�.
	{
		WRITE_LOCK;

		int32 writeSize = 0;

		while (_sendQueue.empty() == false)
		{
			SendBufferRef sendBuffer = _sendQueue.front();

			writeSize += sendBuffer->WriteSize();

			// TODO 
			// SendBuffer �� �ʱ⿡ ���� �ִ� ũ�Ⱑ �ִ�.
			// ���� �ʹ� ���� �����͸� �ѹ��� ������ �ʰ� �� ���̴�.

			_sendQueue.pop();
			_sendEvent.sendBuffers.push_back(sendBuffer);
		}
	}

	// Scatter Gather (����� �ִ� �����͵��� ��Ƽ� �ѹ濡 ������.)
	// ex) 10���� �׿��־��ٸ�, 10���� �ѹ��� ������
	// ��, ����� �ִ� �����͸� �ѹ��� ������ 
	// Send ���� _sendRegistered �� true ���, ��, ������ ���� �����Ϳ� ����
	//      ������ �Ϸ���� ���� ��쿡��, _sendQueue �� �����Ͱ� ���̰� �ȴ�
	//      �� ������ ������ ���� _sendQueue �� �ִ� �����͸� �ѹ��� ������.
	vector<WSABUF> wsaBufs;
	wsaBufs.reserve(_sendEvent.sendBuffers.size());

	for (SendBufferRef sendBuffer : _sendEvent.sendBuffers)
	{
		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->Buffer());
		wsaBuf.len = static_cast<LONG>(sendBuffer->WriteSize());
		wsaBufs.push_back(wsaBuf);
	}

	DWORD numOfBytes = 0;

	if (SOCKET_ERROR == ::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()),
		OUT & numOfBytes, 0, &_sendEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();

		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);

			_sendEvent.owner = nullptr; // Release Ref

			_sendEvent.sendBuffers.clear(); // Release Ref

			_sendRegistered.store(false);
		}
	}
}

void Session::ProcessConnect()
{
	_connectEvent.owner = nullptr; // RELEASE_REF

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

	OnDisconnected(); // ������ �ڵ忡�� �����ε�

	// ref Cnt ���ҽ�Ű��
	GetService()->ReleaseSession(GetSessionRef());
}

void Session::ProcessRecv(int32 numOfBytes)
{
	_recvEvent.owner = nullptr; 

	// ���ŵ� �����Ͱ� ���ٴ� �ǹ̴�, ������ ����ٴ� �ǹ��̴�.
	if (numOfBytes == 0)
	{
		Disconnect(L"Recv 0");
		return;
	}

	// ���� ���ۿ� ������ ����
	if (_recvBuffer.OnWrite(numOfBytes) == false)
	{
		Disconnect(L"OnWrite Overflow");
		return;
	}

	// ���ݱ��� ���� ������ ũ�� ���� (���� ������ ũ��)
	int32 dataSize = _recvBuffer.DataSize();

	/* Ŭ���̾�Ʈ �����ε� �ڵ� */
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
void Session::ProcessSend(int32 numOfBytes)
{
	// ���� �Ϸ� 
	_sendEvent.owner = nullptr; // Release Ref
	_sendEvent.sendBuffers.clear(); // Release Ref

	// ������ �÷����� ref �� 1 ���� (���� �� �ô�)
	_sendEvent.owner = nullptr; // RELEASE_REF

	if (numOfBytes == 0)
	{
		Disconnect(L"Send 0");
		return;
	}

	// ������ �ڵ忡�� ������
	OnSend(numOfBytes);

	WRITE_LOCK;

	if (_sendQueue.empty())
		_sendRegistered.store(false);
	else
	{
		// �񵿱Ⱑ �Ϸ�Ǳ� ������, �� �߰� ������
		// ������ sendQueue �� �����͸� �о�ְ� �� ���̴�.
		// �ٽ� �ѹ� RegisterSend. ��, ProcessSend �� ȣ������ �༮��
		// ����ؼ� ������ ����ϰ� �Ѵٴ� ���̴�.
		RegisterSend();
	}
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



/*-----------------
	PacketSession
------------------*/

PacketSession::PacketSession()
{
}

PacketSession::~PacketSession()
{
}

// TCP ��ſ��� �����Ͱ� ���ŵǸ�, �Ʒ� �Լ��� �����ϰ� �� ���̴�.
// [size(2)][id(2)][data....][size(2)][id(2)][data....]
int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
{
	// ���� ó���� ������ ũ�� 
	int32 processLen = 0;

	// ��, �Ʒ��� ���� ���·� ���� ��Ŷ�� �Ѳ����� ���� �ű��� !
	// �׸��� while ���� ���ؼ� �̸� ���� �� ������ ó���ϰ��� �ϴ� ���̴�.
	// [size(2)] [id(2)] [data....] [size(2)] [id(2)] [data....] [size(2)][id(2)][data....][size(2)][id(2)][data....]
	while (true)
	{
		int32 dataSize = len - processLen;

		// �ּ��� ����� �Ľ��� �� �־�� �Ѵ� (4byte �� ������ �Ѵٴ� ������)
		if (dataSize < sizeof(PacketHeader))
			break;

		// �ʱ� 4byte ������ ���ο� PacketHeader ��ü�� �������ֱ� 
		PacketHeader header = *(reinterpret_cast<PacketHeader*>(&buffer[processLen]));
		
		// ����� ��ϵ� ��Ŷ ũ�⸦ �Ľ��� �� �־�� �Ѵ�
		// dataSize    : ���ݱ��� �Ѱܹ��� ������ ũ�� 
		// header.size : size, id, data ũ�� ��ü ũ�� 
		if (dataSize < header.size)
			break;

		// ��Ŷ ���� ����
		OnRecvPacket(&buffer[processLen], header.size);

		// ���� ��Ŷ���� �Ѿ�� �ȴ�.
		// [size(2)][id(2)][data....] ==> [size(2)][id(2)][data....]
		processLen += header.size;
	}

	return processLen;
}
