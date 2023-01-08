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
	// tcp 소켓 생성
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

	=> 아래와 같이 매번 복사하는 것은 문제가 될 수 있다.
	서버의 경우, 특정 클라이언트 유저의 정보를, 다른 유저들에게 뿌려주는 형태가 된다.
	그러면 모든 유저마다 이와 같이 복사를 해줘야 한다는 것인데 이는 문제가 될 수 있다.
	::memcpy(sendEvent->buffer.data(), buffer, len);

	// RegisterSend 에서 호출되는 WSASend 함수의 경우 멀티쓰레드로부터 안전하지 않다
	// 따라서 WRITE_LOCK 을 호출해주어야 한다.
	WRITE_LOCK;

	RegisterSend();
}
*/

void Session::Send(SendBufferRef sendBuffer)
{
	// 생각할 문제
	// 1) 버퍼 관리 
	//    => 각 데이터 마다 Send 함수를 호출하여 Write_LOCK을 걸고
	//    => RegisterSend 를 호출하여 WSASend 를 호출하는 것은 비효율적이다
	//    => 데이터를 한번에 모아두고, 빵 한번에 다 보내는 방식이 더 효율적 
	//       (Scatter Gather)
	// 2) sendEvent 관리? 단일? 여러개? WSASend 중첩?

	// send 는 한번에 한번씩 차례로 지키면서 호출하는 것이 아니라
	// 무작위로 호출하게 될 것이다. 중복해서 여러 번 호출될 수도 있다.
	// ex) 클라이언트가 몬스터 사냥 => 서버 측으로 계속해서 보낼 것이다.

	// 현재 RegisterSend 가 걸리지 않은 상태라면 걸어준다.
	// - RegisterSend => ProcessSend 호출 이후, 순차적으로 다시 RegisterSend 호출
	// - 즉, WSARecv 가 끝나고, 그 다음 WSARecv 를 호출하고자 한다.
	// - 이를 통해 한번에 데이터를 모아서 패킷을 최소한으로 보낼 수 있다.
	// - 따라서 현재 Send 함수를 실행하려고 했더니, 이전의 SendEvent 가 끝나지 않았다면
	// - Queue<SendBufferRef> 에 보관하는 형태로 진행할 것이다.

	if (IsConnected() == false)
		return;

	bool registered = false;

	// Register 가 안걸린 상태에서 걸어준다.
	{
		// RegisterSend 에서 호출되는 WSASend 함수의 경우 멀티쓰레드로부터 안전하지 않다
	// 따라서 WRITE_LOCK 을 호출해주어야 한다.
		WRITE_LOCK;

		// sendQueue 에 전송하고자 하는 데이터를 밀어넣어준다.
		_sendQueue.push(sendBuffer);

		// 이전의 값이 false 였다면
		if (_sendRegistered.exchange(true) == false)
		{
			registered = true;
		}
	}

	if (registered)
	{
		// RegisterSend() 에서는 _sendQueue 에 있는 데이터를 꺼내다 보낼 것이다.
		RegisterSend();
	}
}

// 서버 끼리 연결하는 경우가 있을 수 있다.
// 그러면 Session 을 이용해서 상대방 서버로 붙는 작업을 해야 할 때가 있다.
bool Session::Connect()
{
	return RegisterConnect();
}

void Session::Disconnect(const WCHAR* cause)
{
	// false 로 세팅하기 이전에 값이 이미 false 라면
	// 이미 연결이 끊긴 상태이므로, return
	if (_connected.exchange(false) == false)
		return;

	// TEMP
	wcout << "Disconnect : " << cause << endl;

	/*
	아래 함수 : ProcessDisconnect 로 이동 => 왜 ? GameSessionManager::BroadCast 참고  
	> OnDisconnected(); // 컨텐츠 코드에서 오버로딩

	// ref Cnt 감소시키기
	> GetService()->ReleaseSession(GetSessionRef());
	*/

	RegisterDisconnect();
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

// iocpEvent 가 이후 recv, 혹은 send 같은 event 를 발생시킨다.
// 이것을 처리할 때 Dispatch 로 처리한다는 것이다.
// 즉, Thread 들이 iocpCore 의 Dispatch 를 호출하면서 일감이 생기면
// iocpObject->Dispatch 호출 => Session->Dispatch 호출
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

	// 비동기로 Disconnect 요청
	if (false == SocketUtils::DisconnectEx(_socket, &_disconnectEvent, TF_REUSE_SOCKET, 0))
	{
		// TF_REUSE_SOCKET : 소켓이 다시 재사용될 수 있게 준비해준다.
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

	// 반드시 Client 여야 한다.
	// Server 의 경우, 상대방이 나한테 붙는 경우이므로 RegisterConnect 는 호출 X
	if (GetService()->GetSeviceType() != ServiceType::Client)
		return false;

	// 주소 재사용
	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	// 소켓에 주소, port 세팅 => 0 을 넘겨주면 여유있는거 아무거나 알아서 세팅해줌
	// ConnectEx 를 호출해주기 위해서는 반드시 필요하다.
	if (SocketUtils::BindAnyAddress(_socket, 0) == false)
		return false;

	_connectEvent.Init();
	_connectEvent.owner = shared_from_this(); 

	// 어디로 연결할 것인가
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
	// 진짜 연결이 끊겼다거나
	// 서버측에서 해당 클라이언트가 해킹의심되어 끊어버리던가
	if (IsConnected() == false)
		return;

	// RecvEvent 를 만들어준다.
	_recvEvent.Init();
	// - ref cnt 를 1 로 해준다.
	_recvEvent.owner = shared_from_this();

	// 패킷이 완전체로 왔는지 안왔는지 세팅해야 한다.
	// - TCP 특성상 데이터 경계가 없기 때문이다.
	// - 이를 RecvBuffer 를 통해 구현
	WSABUF wsaBuf;
	BYTE* WritePos = _recvBuffer.WritePos();

	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.WritePos());

	// wsaBuf.len : 실제 버퍼 자체가 최대로 받을 수 있는 크기
	wsaBuf.len = _recvBuffer.FreeSize();

	DWORD numOfBytes = 0;
	DWORD flags      = 0;

	// 아래의 비동기 Recv 가 완료되게 되면, 결과적으로 ProcessRecv() 함수 호출
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

// 아래 WSASend 라는 비동기 함수가 완료되면
// ProcessSend 라는 함수 측으로 넘어가게 될 것이다.
// - 현재 우리가 만든 방식에서는 한번에 한 쓰레드만 실행하게 될 것이다.
void Session::RegisterSend()
{
	if (IsConnected() == false)
		return;

	// IOCP 에 WSASend 함수를 호출할 준비를 해준다.
	_sendEvent.Init();
	_sendEvent.owner = shared_from_this();

	// Send() 함수에서 _sendQueud 에 넘겨준 데이터를 꺼내서 사용할 것이다.
	// 보낼 데이터를 sendEvent 에 등록
	// 즉, sendEvent 클래스에 있는 sendBuffer 에 옮겨준다는 것이다.
	// 왜 ? WSASend 를 하는 순간 해당 sendBuffer 가 없어지지 않게
	// Ref Cnt 를 유지시켜줘야 한다.
	// 이 Queue 에 빼는 순간 Ref Cnt 가 1 감소하여 사라질 수 있기 때문에
	// 한번 더 여기서 보관해주는 것이다.
	{
		WRITE_LOCK;

		int32 writeSize = 0;

		while (_sendQueue.empty() == false)
		{
			SendBufferRef sendBuffer = _sendQueue.front();

			writeSize += sendBuffer->WriteSize();

			// TODO 
			// SendBuffer 도 초기에 정한 최대 크기가 있다.
			// 따라서 너무 많은 데이터를 한번에 보내지 않게 할 것이다.

			_sendQueue.pop();
			_sendEvent.sendBuffers.push_back(sendBuffer);
		}
	}

	// Scatter Gather (흩어져 있는 데이터들을 모아서 한방에 보낸다.)
	// ex) 10개가 쌓여있었다면, 10개를 한번에 보내기
	// 즉, 흩어져 있는 데이터를 한번에 보내기 
	// Send 에서 _sendRegistered 가 true 라면, 즉, 이전에 보낸 데이터에 대한
	//      통지가 완료되지 않은 경우에믄, _sendQueue 에 데이터가 쌓이게 된다
	//      그 다음에 전송할 때는 _sendQueue 에 있던 데이터를 한번에 보낸다.
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

	// 멀티쓰레드 환경
	_connected.store(true);

	// 자신이 가지고 있는 서비스에 자기 자신 (세션) 등록
	GetService()->AddSession(GetSessionRef());

	// 컨텐츠 코드에서 오버로딩
	OnConnected();

	// 수신 등록 => 비동기 함수 WSARecv() 호출 -> IO 가 완료되면
	// iocpCore->Dispatch => iocpObject->Dispatch => session->Dispatch
	// => ProcessRecv() 함수 호출
	RegisterRecv();
}

void Session::ProcessDisconnect()
{
	_disconnectEvent.owner = nullptr;

	OnDisconnected(); // 컨텐츠 코드에서 오버로딩

	// ref Cnt 감소시키기
	GetService()->ReleaseSession(GetSessionRef());
}

void Session::ProcessRecv(int32 numOfBytes)
{
	_recvEvent.owner = nullptr; 

	// 수신된 데이터가 없다는 의미는, 연결이 끊겼다는 의미이다.
	if (numOfBytes == 0)
	{
		Disconnect(L"Recv 0");
		return;
	}

	// 수신 버퍼에 데이터 쓰기
	if (_recvBuffer.OnWrite(numOfBytes) == false)
	{
		Disconnect(L"OnWrite Overflow");
		return;
	}

	// 지금까지 쌓인 데이터 크기 보기 (누적 데이터 크기)
	int32 dataSize = _recvBuffer.DataSize();

	/* 클라이언트 오버로딩 코드 */
	// readPos 로부터 numOfBytes 까지가 실제 지금까지의 누적 데이터 크기
	// 물론 모든 데이터를 처리하게 될 수도 있고
	// 데이터 경계에 따라 일부 데이터만 처리하게 될 수도 있다.
	// processLen : dataSize 중에서 실제 처리한 데이터 크기 
	int32 processLen = OnRecv(_recvBuffer.ReadPos(), dataSize);

	// 받아들인 데이터 중에서, 실제 처리한 데이터 크기만큼 OnRead 실행
	if (processLen < 0 || dataSize < processLen || _recvBuffer.OnRead(processLen) == false)
	{
		Disconnect(L"On Read Overflow");
		return;
	}

	// 자 여기까지
	// 1) OnRecv 를 통해 WritePos 를 증가
	// 2) OnRead 를 통해 ReadPos 증가
	// 마지막으로 커서 정리를 해준다.
	_recvBuffer.Clean();

	// 수신 등록 => 즉, 다시 한번 계속해서 Recv
	RegisterRecv();
}

// 해당 함수는 여러 개의 쓰레드에서 순서가 보장되지 않은 채 실행될 수 있다.
// 왜냐하면 iocpCore->Dispatch 함수를 통해 아래 함수가 최종 실행되는데
// 이때 A,B,C,D 순서로 send 해도 , 이 순서대로 실행되지는 않는 것이다(send 완료 순서 보장 X)
void Session::ProcessSend(int32 numOfBytes)
{
	// 전송 완료 
	_sendEvent.owner = nullptr; // Release Ref
	_sendEvent.sendBuffers.clear(); // Release Ref

	// 기존에 늘려놨던 ref 를 1 감소 (볼일 다 봤다)
	_sendEvent.owner = nullptr; // RELEASE_REF

	if (numOfBytes == 0)
	{
		Disconnect(L"Send 0");
		return;
	}

	// 컨텐츠 코드에서 재정의
	OnSend(numOfBytes);

	WRITE_LOCK;

	if (_sendQueue.empty())
		_sendRegistered.store(false);
	else
	{
		// 비동기가 완료되기 이전에, 즉 중간 과정에
		// 누군가 sendQueue 에 데이터를 밀어넣게 된 것이다.
		// 다시 한번 RegisterSend. 즉, ProcessSend 를 호출해준 녀석이
		// 계속해서 전송을 담당하게 한다는 것이다.
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

// TCP 통신에서 데이터가 수신되면, 아래 함수를 실행하게 될 것이다.
// [size(2)][id(2)][data....][size(2)][id(2)][data....]
int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
{
	// 실제 처리한 데이터 크기 
	int32 processLen = 0;

	// 즉, 아래와 같은 형태로 여러 패킷이 한꺼번에 오는 거구나 !
	// 그리고 while 문을 통해서 이를 여러 번 나눠서 처리하고자 하는 것이다.
	// [size(2)] [id(2)] [data....] [size(2)] [id(2)] [data....] [size(2)][id(2)][data....][size(2)][id(2)][data....]
	while (true)
	{
		int32 dataSize = len - processLen;

		// 최소한 헤더는 파싱할 수 있어야 한다 (4byte 는 보내야 한다는 것이지)
		if (dataSize < sizeof(PacketHeader))
			break;

		// 초기 4byte 정보를 새로운 PacketHeader 객체에 복사해주기 
		PacketHeader header = *(reinterpret_cast<PacketHeader*>(&buffer[processLen]));
		
		// 헤더에 기록된 패킷 크기를 파싱할 수 있어야 한다
		// dataSize    : 지금까지 넘겨받은 데이터 크기 
		// header.size : size, id, data 크기 전체 크기 
		if (dataSize < header.size)
			break;

		// 패킷 조립 성공
		OnRecvPacket(&buffer[processLen], header.size);

		// 다음 패킷으로 넘어가면 된다.
		// [size(2)][id(2)][data....] ==> [size(2)][id(2)][data....]
		processLen += header.size;
	}

	return processLen;
}
