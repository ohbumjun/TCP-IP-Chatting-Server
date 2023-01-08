#include "pch.h"
#include "Lock.h"
#include "CoreTLS.h"
#include "DeadLockProfiler.h"

// �ش� �Լ� ȣ�� ����
// 1) �ڱⰡ lock �� �� ���¸� writeCnt �� ������Ű�� 
// 2) �ڱ⸦ ����� �ٸ� ��������� read �ϰ� ���� ���� ���
void Lock::WriteLock(const char* name)
{
#if _DEBUG
	GDeadLockProfiler->PushLock(name);
#endif

	// ������ �����尡 �����ϰ� �ִٸ� ������ ����
	// - (_lockFlag.load() && WRITE_THREAD_MASK) : ���� 16��Ʈ ����
	// - >> 16 : ���� 16��Ʈ�� �ִ� ���� �������� �ȴ�.
	const uint32 lockThreadId = (_lockFlag.load() & WRITE_THREAD_MASK) >> 16;
	
	// ���� �ش� �Լ��� ȣ���ϰ� �ִ� ������� �����ϴٸ�
	// ������ �����尡 ������ ���¿��� �� lock �� �Ŵ� ���̹Ƿ�
	// writeCount �� ������Ű�� return
	if (LThreadID == lockThreadId)
	{
		_writeCount++;
		return;
	}

	const int64 beginTick = ::GetTickCount64();

	// �ƹ��� ���� �� �����ϰ� ���� ���� �� �����ؼ� �������� ��´�.
	const uint32 desired = ((LThreadID << 16) & WRITE_THREAD_MASK);

	while (true)
	{
		for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; ++spinCount)
		{
			// desired �� expected �� ���� ���� writeCount �� ����
			// �� ���� �ｼ, ���� 16��Ʈ�� 0 �� ����
			// ��, Read Count �� 0�� ���� ! �ƹ��� �����ϰ� ���� ���� ����
			// �ƹ��� �ش� �ڷḦ �а� ���� ���� ���� writeLock �� �Ǵٴ� ��
			uint32 expected = EMPTY_FLAG;

			if (_lockFlag.compare_exchange_strong(OUT expected, desired))
			{
				// �ش� ��ȣ ������ ���´ٴ� �ǹ̴�
				// ���տ��� �̰�ٴ� �ǹ̰� �ȴ�.
				_writeCount++;
				return;
			}
		}

		// ����ġ�� spinlock �ð��� �����ɷȴٴ� ����
		// ������ �߻��ߴٴ� �ǹ��̹Ƿ� 
		// CRASH
		if (::GetTickCount64() - beginTick >= ACQUIRE_TIMEOUT_TICK)
		{
			CRASH("Lock Timeout"); 
		}
		
		// 5000 �� Ȯ���ϸ� ��õ��� ���� �� ���̴�.
		// yield : ���� ���� ���� ������ �켱���� �̻���
		// �ٸ� �����忡�� �����ȸ �纸�Ѵ�.
		// (��, ���� ������ ���ŷ ���·�)
		this_thread::yield();
	}
}

void Lock::WriteUnlock(const char* name)
{
#if _DEBUG
	GDeadLockProfiler->PopLock(name);
#endif

	// ReadLock �� �� Ǯ�� ������ Write Unlock �Ұ����ϴ�
	if ((_lockFlag.load() & READ_COUNT_MAX) != 0)
		CRASH("INVALID UNLOCK OREDER");


	const int32 lockCount = --_writeCount;

	if (lockCount == 0)
	{
		// ���� 16��Ʈ�� 0 ���� �о������ν�
		// ���� lock �� �ɰ� �ִ� �����尡 ���ٴ� ���� �˸���
		_lockFlag.store(EMPTY_FLAG);
	}
}

void Lock::ReadLock(const char* name)
{
#if _DEBUG
	GDeadLockProfiler->PushLock(name);
#endif

	// 1) ������ �����尡 Write Lock �� ��� �ִٸ� ?
	// ���� read cnt 1 ����
	// ������ WriteLock �� �ش� �����尡 ��� �ִٴ� ����
	// �ٸ� �����尡 ���� Write Lock �� ��� ���� ���ϴ� ���̹Ƿ� (���� ���ϴ� ���̹Ƿ�)
	// ���ϰ� +1 ó���� ���� ���̴�.
	const uint32 lockThreadId = (_lockFlag.load() && WRITE_THREAD_MASK) >> 16;

	if (LThreadID == lockThreadId)
	{
		_lockFlag.fetch_add(1);
		return;
	}

	// 2) �ƹ��� ���� �����ϰ� ���� ���� �� �����ؼ�
	// ���� ī��Ʈ�� �ø���
	// Write Lock�� �� ������ ID �� ���� 16 ��Ʈ�� ����
	// Read Cnt �� ���� 16��Ʈ�� Read Lock �� ��� �����尡 ��� �ִ��� Ȯ���ϱ�
	const int64 beginTick = ::GetTickCount64();

	while(true)
	{
		for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; ++spinCount)
		{
			uint32 expected = (_lockFlag.load() & READ_COUNT_MAX);

			// ���������� read lock �� �ɸ��� ���� 16��Ʈ ���� 1 ������ų ���̴�.
			// _lockFlag �� expected �� ��ġ���, _lockFlag ���� expected + 1 ������ ��ü
			if (_lockFlag.compare_exchange_strong(OUT expected, expected + 1))
				return;

			// ������ ���� 2���� �̴�.
			// 1) �ٸ� �����尡 write lock �� ��� ���
			// 2) ������ ���̿� �ٸ� �����尡 expected ���� 1 ���� (��ġ��)
		};

		// ����ġ�� spinlock �ð��� �����ɷȴٴ� ����
		// ������ �߻��ߴٴ� �ǹ��̹Ƿ� 
		// CRASH
		if (::GetTickCount64() - beginTick >= ACQUIRE_TIMEOUT_TICK)
			CRASH("Lock Timeout");

		this_thread::yield();
	}
}

void Lock::ReadUnlock(const char* name)
{
#if _DEBUG
	GDeadLockProfiler->PopLock(name);
#endif

	// fetch_sub : ���� ���ҽ�Ű�� �Լ� + ���� ������ ���� ����
	if ((_lockFlag.fetch_sub(1) & READ_COUNT_MAX) == 0)
	{
		// _lockFlag �� 1�� ���� ���� ���� 0�� ���ٴ� �ǹ̰� �ȴ�.
		CRASH("MULTIPLE UNLOCK");
	}
}
