#pragma once
#include "IocpCore.h"
#include "IocpEvent.h"
#include "NetAddress.h"

/*--------------
	Session
---------------*/

// iocpCore 에 등록해줄 녀석이다.
// 즉, CreateIoCompletionPort 등을 통해 소켓을 CP 에 등록할 때, 넘겨주는 Key 값으로도 사용될 수 있다.
class Session : public IocpObject
{
public:
	Session();
	virtual ~Session();

public:
	// 정보관련 세팅
	void		SetNetAddress(NetAddress address) { _netAddress = address; }
	NetAddress	GetAddress() { return _netAddress; }
	SOCKET		GetSocket() { return _socket; }

public:
	virtual HANDLE		GetHandle() override;
	virtual void		Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

public:
	// send, recv 와 관련된 Buffer
	char _recvBuffer[1000];

private:
	// 클라이언트 요청을 받아들이는 과정에서 만들어지는 클라이언트 소켓 ?
	SOCKET			_socket = INVALID_SOCKET;
	NetAddress		_netAddress = {};
	Atomic<bool>	_connected = false;
};

