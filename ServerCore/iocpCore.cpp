#include "pch.h"
#include "IocpCore.h"
#include "IocpEvent.h"

// TEMP
IocpCore GIocpCore;

/*--------------
	IocpCore
---------------*/

IocpCore::IocpCore()
{
	// CP 생성
	_iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	ASSERT_CRASH(_iocpHandle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore()
{
	::CloseHandle(_iocpHandle);
}

bool IocpCore::Register(IocpObject* iocpObject)
{
	// CP 에 locpObject 등록
	
	// 1번째 : iocpObject->GetHandle() : 소켓
	// 3번째 : Key 값 = > 각 쓰레드에서 받을 정보
	return ::CreateIoCompletionPort(iocpObject->GetHandle(), 
		_iocpHandle, reinterpret_cast<ULONG_PTR>(iocpObject), 0);
}

// Woker Thread 들이 해당 함수를 계속하여 호출하면서
// 일감이 있는지 확인한다 (입출력이 완료된 정보가 있는지 확인한다)
bool IocpCore::Dispatch(uint32 timeoutMs)
{
	DWORD numOfBytes = 0;
	IocpObject* iocpObject = nullptr;
	IocpEvent* iocpEvent = nullptr;

	// numOfBytes : 전달된 정보 크기
	// iocpObject : CreateIoCompletionPort 을 통해 Register 시 넘겨준 Key 값
	// iocpEvent == overlappedEx : WSARecv 등 함수 호출시 얻어낸 데이터
	if (::GetQueuedCompletionStatus(_iocpHandle, OUT & numOfBytes, 
		OUT reinterpret_cast<PULONG_PTR>(&iocpObject), 
		OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), timeoutMs))
	{
		iocpObject->Dispatch(iocpEvent, numOfBytes);
	}
	else
	{
		int32 errCode = ::WSAGetLastError();
		switch (errCode)
		{
		case WAIT_TIMEOUT:
			return false;
		default:
			iocpObject->Dispatch(iocpEvent, numOfBytes);
			break;
		}
	}

	return true;
}
