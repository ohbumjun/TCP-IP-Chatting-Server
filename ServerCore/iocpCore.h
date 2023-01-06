#pragma once

/*----------------
	IocpObject : iocp �� ����� �� �ִ� ���
-----------------*/

// enable_shared_from_this<IocpObject> : �ڱ� �ڽſ� ���� Weak Ptr �� ��� �ְ� ��
class IocpObject : public enable_shared_from_this<IocpObject>
{
public:
	// GetHandle() : ���� �ڵ� ��ȯ
	// abstract    : �ݵ�� ����� ���ؼ� �ش� �Լ��� �����ϵ��� �ϱ�
	virtual HANDLE GetHandle() abstract;
	virtual void Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) abstract;
};

/*--------------
	IocpCore
---------------*/

class IocpCore
{
public:
	IocpCore();
	~IocpCore();

	HANDLE		GetHandle() { return _iocpHandle; }

	// Shared Ptr �� ����ϱ�� �ߴٸ�, ��� �����ؼ� ���, raw �� ȥ�� X
	// bool		Register(class IocpObject* iocpObject);
	bool		Register(IocpObjectRef iocpObject);
	bool		Dispatch(uint32 timeoutMs = INFINITE);

private:
	// CP ������Ʈ �ڵ�
	HANDLE		_iocpHandle;
};

// TEMP
// extern IocpCore GIocpCore;