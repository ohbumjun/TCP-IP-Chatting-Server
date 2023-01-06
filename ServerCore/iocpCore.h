#pragma once

/*----------------
	IocpObject : iocp 에 등록할 수 있는 대상
-----------------*/

class IocpObject
{
public:
	// GetHandle() : 소켓 핸들 반환
	// abstract    : 반드시 상속을 통해서 해당 함수를 구현하도록 하기
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
	// CP 오브젝트 핸들
	HANDLE		_iocpHandle;
};

// TEMP
extern IocpCore GIocpCore;