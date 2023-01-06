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

void Session::RegisterSend()
{
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

	// 수신 등록 => 즉, 다시 한번 계속해서 Recv
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
