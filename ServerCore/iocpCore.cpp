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

bool IocpCore::Register(IocpObjectRef iocpObject)
{
	// CP 에 locpObject 등록
	
	// 1번째 : iocpObject->GetHandle() : 소켓
	// 3번째 : Key 값 = > 각 쓰레드에서 받을 정보
	return ::CreateIoCompletionPort(iocpObject->GetHandle(), 
		_iocpHandle, 
		/*key 값 : 원래 reinterpret_cast<ULONG_PTR>(iocpObject)*/
		0, 
		0);
}

// Woker Thread 들이 해당 함수를 계속하여 호출하면서
// 일감이 있는지 확인한다 (입출력이 완료된 정보가 있는지 확인한다)
bool IocpCore::Dispatch(uint32 timeoutMs)
{
	DWORD numOfBytes = 0;
	ULONG_PTR key = 0;
	// IocpObject* iocpObject = nullptr;
	IocpEvent* iocpEvent = nullptr;

	// GetQueuedCompletionStatus : 해당 함수는 일감이 없으면, 바로 해당 쓰레드 블로킹 상태에 놓인다.
	//                             일감이 들어오면, 쓰레드풀에서 특정 쓰레드를 깨우는 방식

	// numOfBytes : 전달된 정보 크기
	// iocpObject : CreateIoCompletionPort 을 통해 Register 시 넘겨준 Key 값
	// iocpEvent == overlappedEx : WSARecv 등 함수 호출시 얻어낸 데이터
	if (::GetQueuedCompletionStatus(_iocpHandle, OUT & numOfBytes, 
		/*IocpCore::Register 에서 넘겨준 Key 값 => 만약 애초에 Key 값 안넘겨줬으면, 여기서도 복원 X*/
		// 사용할시 : OUT reinterpret_cast<PULONG_PTR>(&iocpObject), 
		// 그런데 여기서는 복원시킨 Event 객체에 iocpObject 를 부모로 세팅하고
		// Event 객체를 통해서 iocpObject 정보를 얻어오는 방식으로 진행할 것이다.
		&key,
		OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), timeoutMs))
	{
		// Event 에 대한 iocpObject 주인 복원
		IocpObjectRef iocpObject = iocpEvent->owner;
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
			IocpObjectRef iocpObject = iocpEvent->owner;
			iocpObject->Dispatch(iocpEvent, numOfBytes);
			break;
		}
	}

	return true;
}
