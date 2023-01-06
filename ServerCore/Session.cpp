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
	// tcp 소켓 생성
	_socket = SocketUtils::CreateSocket();
}

Session::~Session()
{
	SocketUtils::Close(_socket);
}

void Session::Send(BYTE* buffer, int32 len)
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
	// 따라서 _recvEvent 와 같이 한개를 재사용하는 형식으로 진행하면 안된다.
	// 매번 동적으로 생성해주는 방법이 있다.
	// ex) 클라이언트가 몬스터 사냥 => 서버 측으로 계속해서 보낼 것이다.

	SendEvent* sendEvent = new SendEvent;
	sendEvent->owner = shared_from_this(); // ADD_REF
	sendEvent->buffer.resize(len);
	::memcpy(sendEvent->buffer.data(), buffer, len);

	// RegisterSend 에서 호출되는 WSASend 함수의 경우 멀티쓰레드로부터 안전하지 않다
	// 따라서 WRITE_LOCK 을 호출해주어야 한다.
	WRITE_LOCK;

	RegisterSend(sendEvent);
}

void Session::Disconnect(const WCHAR* cause)
{
	// false 로 세팅하기 이전에 값이 이미 false 라면
	// 이미 연결이 끊긴 상태이므로, return
	if (_connected.exchange(false) == false)
		return;

	// TEMP
	wcout << "Disconnect : " << cause << endl;

	OnDisconnected(); // 컨텐츠 코드에서 오버로딩
	SocketUtils::Close(_socket);

	// ref Cnt 감소시키기
	GetService()->ReleaseSession(GetSessionRef());
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

void Session::RegisterConnect()
{
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

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer);
	wsaBuf.len = len32(_recvBuffer);

	DWORD numOfBytes = 0;
	DWORD flags      = 0;

	// 아래의 비동기 Recv 가 완료되게 되면, 결과적으로 ProcessRecv() 함수 호출
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

// 아래 WSASend 라는 비동기 함수가 완료되면
// ProcessSend 라는 함수 측으로 넘어가게 될 것이다.
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

void Session::ProcessRecv(int32 numOfBytes)
{
	_recvEvent.owner = nullptr; 

	// 수신된 데이터가 없다는 의미는, 연결이 끊겼다는 의미이다.
	if (numOfBytes == 0)
	{
		Disconnect(L"Recv 0");
		return;
	};

	// TODO
	cout << "Recv Data Len = " << numOfBytes << endl;

	// 클라이언트 오버로딩 코드
	OnRecv(_recvBuffer, numOfBytes);

	// 수신 등록 => 즉, 다시 한번 계속해서 Recv
	RegisterRecv();
}

// 해당 함수는 여러 개의 쓰레드에서 순서가 보장되지 않은 채 실행될 수 있다.
// 왜냐하면 iocpCore->Dispatch 함수를 통해 아래 함수가 최종 실행되는데
// 이때 A,B,C,D 순서로 send 해도 , 이 순서대로 실행되지는 않는 것이다(send 완료 순서 보장 X)
void Session::ProcessSend(SendEvent* sendEvent, int32 numOfBytes)
{
	// 기존에 늘려놨던 ref 를 1 감소 (볼일 다 봤다)
	sendEvent->owner = nullptr; // RELEASE_REF
	delete(sendEvent);

	if (numOfBytes == 0)
	{
		Disconnect(L"Send 0");
		return;
	}

	// 컨텐츠 코드에서 오버로딩
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
