#pragma once
#include "IocpCore.h"
#include "NetAddress.h"

class AcceptEvent;
class ServerService;

/*--------------
	Listener
---------------*/

// 문지기 역할
// - Listener 를 만들어서 iocpCore 에 등록하고, 관심 대상으로 살피라고 할 것이다.
// - CreateIoCompletionPort 등을 통해 소켓을 CP 에 등록할 때, 넘겨주는 Key 값으로도 사용될 수 있다.
//   진행 순서
// 1) StartAccept => 서버 소켓 세팅
// 2) RegisterAccept => 세션 생성 및 클라이언트 연결 요청 받아들이기
// 3) 실제 연결 요청이 있을 때마다 iocpCore::Dispatch 함수 호출 
//    => iocpObject (= Listner)->Dispatch 함수 호출
// 4) ProcessAccept() 함수 호출 => Client 정보 세팅 및
//    - session 의 함수들을 호출해줌으로써 본격적인 작업 시작 
//      -- ProcessConnect()
class Listener : public IocpObject
{
public:
	Listener() = default;
	~Listener();

public:
	/* 외부에서 사용 */
	bool StartAccept(ServerServiceRef service);
	void CloseSocket();

public:
	/* 인터페이스 구현 */
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
	/* 수신 관련 */
	void RegisterAccept(AcceptEvent* acceptEvent);
	void ProcessAccept(AcceptEvent* acceptEvent);

protected:
	SOCKET _socket = INVALID_SOCKET;
	Vector<AcceptEvent*> _acceptEvents;
	ServerServiceRef _service;
};

