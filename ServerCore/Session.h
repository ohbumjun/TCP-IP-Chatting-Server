#pragma once
#include "IocpCore.h"
#include "IocpEvent.h"
#include "NetAddress.h"
#include "RecvBuffer.h"

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

	enum
	{
		BUFFER_SIZE = 0x10000, // 64KB
	};

public:
	Session();
	virtual ~Session();

public:
						/* 외부에서 사용 */
	void				Send(SendBufferRef sendBuffer);
	bool				Connect();
	// 해킹의심 및 상대방 연결이 끊긴 경우, 사유를 인자로 받아서 연결 끊기
	void				Disconnect(const WCHAR* cause);

	shared_ptr<Service>	GetService() { return _service.lock(); }
	void				SetService(shared_ptr<Service> service) { _service = service; }

public:
						/* 정보 관련 */
	void				SetNetAddress(NetAddress address) { _netAddress = address; }
	NetAddress			GetAddress() { return _netAddress; }
	SOCKET				GetSocket() { return _socket; }
	bool				IsConnected() { return _connected; }
	SessionRef			GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }

private:
						/* 인터페이스 구현 */
	virtual HANDLE		GetHandle() override;
	virtual void		Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
						/* 전송 관련 */
	bool				RegisterConnect();
	bool				RegisterDisconnect();
	void				RegisterRecv();
	void				RegisterSend();

						/*Register 과정이 끝난 이후 처리하는 함수들*/
	void				ProcessConnect();
	void				ProcessDisconnect();
	void				ProcessRecv(int32 numOfBytes);
	void				ProcessSend(int32 numOfBytes);

	void				HandleError(int32 errorCode);

protected:
						/* 컨텐츠 코드에서 재정의 */
	virtual void		OnConnected() { }
	virtual int32		OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void		OnSend(int32 len) { }
	virtual void		OnDisconnected() { }

private:
	// Service 에 대한 존재를 알아야지만
	// Service 에 등록하거나 끊거나 하는 기능을 구현할 수 있다.
	weak_ptr<Service>	_service;
	SOCKET				_socket = INVALID_SOCKET;
	NetAddress			_netAddress = {};
	Atomic<bool>		_connected = false;

private:
	// Session 은 멀티 쓰레드 형태로 돌아가기 때문에 Lock 을 걸어준다.
	USE_LOCK;

							/* 수신 관련 */
	RecvBuffer				_recvBuffer;

							/* 송신 관련 */
	Queue<SendBufferRef>	_sendQueue;

							/* 멀티쓰레드 환경 고려하기 */
	Atomic<bool>			_sendRegistered = false;

private:
						/* IocpEvent 재사용 */
	ConnectEvent		_connectEvent;
	DisconnectEvent		_disconnectEvent;
	RecvEvent			_recvEvent;
	SendEvent			_sendEvent;
};

/*-----------------
	PacketSession
------------------*/

// TCP 데이터 경계가 존재 X
// 따라서 모든 데이터가 전달되었는지 확인하는 절차 필요하다.
// 패킷을 보낼 때마다 Header 를 추가해서 보낼 것이다.
struct PacketHeader
{
	uint16 size; // 전송하고자 하는 패킷 사이즈
	uint16 id;	 // 프로토콜ID (ex. 1=로그인 요청, 2=이동요청)
	             // 즉, 해당 ID 를 보고 어떤 내용이 들어있는지 파악할 수 있다.
};

// 아래와 같이 진행할 것이다.
// 즉, 앞에 4byte 에 해당하는 PacketHeader 내용을 참조하여, 받은 data 확인
// [size(2byte)][id(2byte)][data...][size(2byte)][id(2byte)][data...]
class PacketSession : public Session
{
public:
	PacketSession();
	virtual ~PacketSession();

	PacketSessionRef	GetPacketSessionRef() { return static_pointer_cast<PacketSession>(shared_from_this()); }

protected:
	// sealed : 컨텐츠 단에서 PacketSession 를 상속받은 녀석은 OnRecv 사용 불가
	virtual int32		OnRecv(BYTE* buffer, int32 len) sealed;
	// 아래 함수는 컨텐츠 단에서 반드시 구현하게 하기 
	virtual void		OnRecvPacket(BYTE* buffer, int32 len) abstract;
};