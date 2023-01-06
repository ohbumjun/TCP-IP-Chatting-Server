#include "pch.h"
#include "IocpEvent.h"

/*--------------
	IocpEvent
---------------*/

IocpEvent::IocpEvent(EventType type) : _type(type)
{
	Init();
}

void IocpEvent::Init()
{
	// ���ΰ����� �ü���� ����ϴ� ��. �츮�� �ǵ帱 ���� ����.
	OVERLAPPED::hEvent = 0;
	OVERLAPPED::Internal = 0;
	OVERLAPPED::InternalHigh = 0;
	OVERLAPPED::Offset = 0;
	OVERLAPPED::OffsetHigh = 0;
}