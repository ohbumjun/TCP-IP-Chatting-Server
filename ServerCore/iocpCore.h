#pragma once

/*----------------
	IocpObject : iocp �� ����� �� �ִ� ���
-----------------*/

class IocpObject
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

	bool		Register(class IocpObject* iocpObject);
	bool		Dispatch(uint32 timeoutMs = INFINITE);

private:
	// CP ������Ʈ �ڵ�
	HANDLE		_iocpHandle;
};

// TEMP
extern IocpCore GIocpCore;