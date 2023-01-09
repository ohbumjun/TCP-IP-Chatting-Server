#include "pch.h"
#include "IocpCore.h"
#include "IocpEvent.h"

/*--------------
	IocpCore
---------------*/

IocpCore::IocpCore()
{
	// CP ����
	_iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	ASSERT_CRASH(_iocpHandle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore()
{
	::CloseHandle(_iocpHandle);
}

bool IocpCore::Register(IocpObjectRef iocpObject)
{
	// CP �� locpObject ���

	// 1��° : iocpObject->GetHandle() : ����
	// 3��° : Key �� = > �� �����忡�� ���� ����
	return ::CreateIoCompletionPort(iocpObject->GetHandle(), _iocpHandle, 
		/*key �� : ���� reinterpret_cast<ULONG_PTR>(iocpObject)*/
		/*key*/0, 0);
}

// Woker Thread ���� �ش� �Լ��� ����Ͽ� ȣ���ϸ鼭
// �ϰ��� �ִ��� Ȯ���Ѵ� (������� �Ϸ�� ������ �ִ��� Ȯ���Ѵ�)
bool IocpCore::Dispatch(uint32 timeoutMs)
{
	DWORD numOfBytes = 0;
	ULONG_PTR key = 0;	
	IocpEvent* iocpEvent = nullptr;

	// GetQueuedCompletionStatus : �ش� �Լ��� �ϰ��� ������, �ٷ� �ش� ������ ���ŷ ���¿� ���δ�.
	//                             �ϰ��� ������, ������Ǯ���� Ư�� �����带 ����� ���
	// numOfBytes : ���޵� ���� ũ��
	// iocpObject : CreateIoCompletionPort �� ���� Register �� �Ѱ��� Key ��
	// iocpEvent == overlappedEx : WSARecv �� �Լ� ȣ��� �� ������
	if (::GetQueuedCompletionStatus(_iocpHandle, OUT &numOfBytes, 
		/*IocpCore::Register ���� �Ѱ��� Key �� => ���� ���ʿ� Key �� �ȳѰ�������, ���⼭�� ���� X*/
		// ����ҽ� : OUT reinterpret_cast<PULONG_PTR>(&iocpObject), 
		// �׷��� ���⼭�� ������Ų Event ��ü�� iocpObject �� �θ�� �����ϰ�
		// Event ��ü�� ���ؼ� iocpObject ������ ������ ������� ������ ���̴�.
		OUT &key, OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), timeoutMs))
	{
		IocpObjectRef iocpObject = iocpEvent->owner;

		// Event �� ���� iocpObject ���� ����
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
			// TODO : �α� ���
			IocpObjectRef iocpObject = iocpEvent->owner;
			iocpObject->Dispatch(iocpEvent, numOfBytes);
			break;
		}
	}

	return true;
}
