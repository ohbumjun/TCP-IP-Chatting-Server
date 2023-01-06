﻿#pragma once
#include "IocpCore.h"
#include "NetAddress.h"

class AcceptEvent;

/*--------------
	Listener
---------------*/

// 문지기 역할
// - Listener 를 만들어서 iocpCore 에 등록하고, 관심 대상으로 살피라고 할 것이다.
// - CreateIoCompletionPort 등을 통해 소켓을 CP 에 등록할 때, 넘겨주는 Key 값으로도 사용될 수 있다.
// 진행 순서
// 1) StartAccept
class Listener : public IocpObject
{
public:
	Listener() = default;
	~Listener();

public:
	/*외부에서 사용*/
	bool StartAccept(NetAddress netAddress);
	void CloseSocket();

public:
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
	void RegisterAccept(AcceptEvent* acceptEvent);
	void ProcessAccept(AcceptEvent* acceptEvent);

protected:
	// 해당 변수가 listener 소켓이 될 것이다.
	// - session 내에 socket 을 들고 있게 된다.
	SOCKET _socket = INVALID_SOCKET;
	vector<AcceptEvent*> _acceptEvents;
};
