#pragma once
#include "IocpCore.h"
#include "IocpEvent.h"
#include "NetAddress.h"
#include "RecvBuffer.h"

class Service;

/*--------------
	Session
---------------*/

// iocpCore �� ������� �༮�̴�.
// ��, CreateIoCompletionPort ���� ���� ������ CP �� ����� ��, �Ѱ��ִ� Key �����ε� ���� �� �ִ�.
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
						/* �ܺο��� ��� */
	void				Send(SendBufferRef sendBuffer);
	bool				Connect();
	// ��ŷ�ǽ� �� ���� ������ ���� ���, ������ ���ڷ� �޾Ƽ� ���� ����
	void				Disconnect(const WCHAR* cause);

	shared_ptr<Service>	GetService() { return _service.lock(); }
	void				SetService(shared_ptr<Service> service) { _service = service; }

public:
						/* ���� ���� */
	void				SetNetAddress(NetAddress address) { _netAddress = address; }
	NetAddress			GetAddress() { return _netAddress; }
	SOCKET				GetSocket() { return _socket; }
	bool				IsConnected() { return _connected; }
	SessionRef			GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }

private:
						/* �������̽� ���� */
	virtual HANDLE		GetHandle() override;
	virtual void		Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
						/* ���� ���� */
	bool				RegisterConnect();
	bool				RegisterDisconnect();
	void				RegisterRecv();
	void				RegisterSend();

						/*Register ������ ���� ���� ó���ϴ� �Լ���*/
	void				ProcessConnect();
	void				ProcessDisconnect();
	void				ProcessRecv(int32 numOfBytes);
	void				ProcessSend(int32 numOfBytes);

	void				HandleError(int32 errorCode);

protected:
						/* ������ �ڵ忡�� ������ */
	virtual void		OnConnected() { }
	virtual int32		OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void		OnSend(int32 len) { }
	virtual void		OnDisconnected() { }

private:
	// Service �� ���� ���縦 �˾ƾ�����
	// Service �� ����ϰų� ���ų� �ϴ� ����� ������ �� �ִ�.
	weak_ptr<Service>	_service;
	SOCKET				_socket = INVALID_SOCKET;
	NetAddress			_netAddress = {};
	Atomic<bool>		_connected = false;

private:
	// Session �� ��Ƽ ������ ���·� ���ư��� ������ Lock �� �ɾ��ش�.
	USE_LOCK;

							/* ���� ���� */
	RecvBuffer				_recvBuffer;

							/* �۽� ���� */
	Queue<SendBufferRef>	_sendQueue;

							/* ��Ƽ������ ȯ�� ����ϱ� */
	Atomic<bool>			_sendRegistered = false;

private:
						/* IocpEvent ���� */
	ConnectEvent		_connectEvent;
	DisconnectEvent		_disconnectEvent;
	RecvEvent			_recvEvent;
	SendEvent			_sendEvent;
};

/*-----------------
	PacketSession
------------------*/

// TCP ������ ��谡 ���� X
// ���� ��� �����Ͱ� ���޵Ǿ����� Ȯ���ϴ� ���� �ʿ��ϴ�.
// ��Ŷ�� ���� ������ Header �� �߰��ؼ� ���� ���̴�.
struct PacketHeader
{
	uint16 size; // �����ϰ��� �ϴ� ��Ŷ ������
	uint16 id;	 // ��������ID (ex. 1=�α��� ��û, 2=�̵���û)
	             // ��, �ش� ID �� ���� � ������ ����ִ��� �ľ��� �� �ִ�.
};

// �Ʒ��� ���� ������ ���̴�.
// ��, �տ� 4byte �� �ش��ϴ� PacketHeader ������ �����Ͽ�, ���� data Ȯ��
// [size(2byte)][id(2byte)][data...][size(2byte)][id(2byte)][data...]
class PacketSession : public Session
{
public:
	PacketSession();
	virtual ~PacketSession();

	PacketSessionRef	GetPacketSessionRef() { return static_pointer_cast<PacketSession>(shared_from_this()); }

protected:
	// sealed : ������ �ܿ��� PacketSession �� ��ӹ��� �༮�� OnRecv ��� �Ұ�
	virtual int32		OnRecv(BYTE* buffer, int32 len) sealed;
	// �Ʒ� �Լ��� ������ �ܿ��� �ݵ�� �����ϰ� �ϱ� 
	virtual void		OnRecvPacket(BYTE* buffer, int32 len) abstract;
};