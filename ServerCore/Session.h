#pragma once
#include "IocpCore.h"
#include "IocpEvent.h"
#include "NetAddress.h"

class Service;

/*--------------
	Session
---------------*/

// iocpCore 에 등록해줄 녀석이다.
// 즉, CreateIoCompletionPort 등을 통해 소켓을 CP 에 등록할 때, 넘겨주는 Key 값으로도 사용될 수 있다.
class Session : public IocpObject
{
	friend class Listener;
	friend class IocpCore;
	friend class Service;
public:
	Session();
	virtual ~Session();

public:
	// 실제 데이터를 보내는 함수 
	// - buffer: 데이터
	void				Send(BYTE* buffer, int32 len);
	// 해킹의심 및 상대방 연결이 끊긴 경우, 사유를 인자로 받아서 연결 끊기
	void				Disconnect(const WCHAR* cause);
	shared_ptr<Service>	GetService() { return _service.lock(); }
	void				SetService(shared_ptr<Service> service) { _service = service; }

public:
	// 정보관련 세팅
	void		SetNetAddress(NetAddress address) { _netAddress = address; }
	NetAddress	GetAddress() { return _netAddress; }
	SOCKET		GetSocket() { return _socket; }
	bool				IsConnected() { return _connected; }
	SessionRef			GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }

public:
	/*인터페이스 구현*/
	virtual HANDLE		GetHandle() override;
	virtual void		Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;
private:
	/* 전송 관련 */
	void				RegisterConnect(); // Client Service 측에서 사용 ?
	void				RegisterRecv();
	void				RegisterSend(SendEvent* sendEvent);

	/*Register 과정이 끝난 이후 처리하는 함수들*/
	void				ProcessConnect();
	// Register Event 생성 및 비동기 Recv 실행
	void				ProcessRecv(int32 numOfBytes);
	void				ProcessSend(SendEvent* sendEvent, int32 numOfBytes);

	void				HandleError(int32 errorCode);

protected:
	/* 컨텐츠 코드에서 오버로딩 */
	virtual void		OnConnected() { }
	virtual int32		OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void		OnSend(int32 len) { }
	virtual void		OnDisconnected() { }
public:
	// send, recv 와 관련된 Buffer
	BYTE _recvBuffer[1000];

private:
	// Service 에 대한 존재를 알아야지만
	// Service 에 등록하거나 끊거나 하는 기능을 구현할 수 있다.
	weak_ptr<Service>	_service;
	// 클라이언트 요청을 받아들이는 과정에서 만들어지는 클라이언트 소켓 ?
	SOCKET			_socket = INVALID_SOCKET;
	NetAddress		_netAddress = {};
	Atomic<bool>	_connected = false;

private:
	// Session 은 멀티 쓰레드 형태로 돌아가기 때문에 Lock 을 걸어준다.
	USE_LOCK;

	/* 수신 관련 */

	/* 송신 관련 */

private:
						/* IocpEvent 재사용 */
	// _recvEvent.owner : WSARecv 함수를 호출하기 이전에 + (RegisterRecv)
	//                    실패 혹은 완료 (ProcessRecv) 하고 -- 
	RecvEvent			_recvEvent;
};

