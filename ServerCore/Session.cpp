#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"

/*--------------
	Session
---------------*/

Session::Session()
{
	// tcp 소켓 생성
	_socket = SocketUtils::CreateSocket();
}

Session::~Session()
{
	SocketUtils::Close(_socket);
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

// iocpEvent 가 이후 recv, 혹은 send 같은 event 를 발생시킨다.
// 이것을 처리할 때 Dispatch 로 처리한다는 것이다.
void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	// TODO
}