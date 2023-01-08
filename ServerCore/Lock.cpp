#include "pch.h"
#include "Lock.h"
#include "CoreTLS.h"
#include "DeadLockProfiler.h"

// 해당 함수 호출 조건
// 1) 자기가 lock 을 건 상태면 writeCnt 만 증가시키기 
// 2) 자기를 비롯한 다른 쓰레드들이 read 하고 있지 않을 경우
void Lock::WriteLock(const char* name)
{
#if _DEBUG
	GDeadLockProfiler->PushLock(name);
#endif

	// 동일한 쓰레드가 소유하고 있다면 무조건 성공
	// - (_lockFlag.load() && WRITE_THREAD_MASK) : 상위 16비트 추출
	// - >> 16 : 상위 16비트에 있던 것이 내려오게 된다.
	const uint32 lockThreadId = (_lockFlag.load() & WRITE_THREAD_MASK) >> 16;
	
	// 현재 해당 함수를 호출하고 있는 쓰레드와 동일하다면
	// 동일한 쓰레드가 소유한 상태에서 또 lock 을 거는 것이므로
	// writeCount 만 증가시키고 return
	if (LThreadID == lockThreadId)
	{
		_writeCount++;
		return;
	}

	const int64 beginTick = ::GetTickCount64();

	// 아무도 소유 및 공유하고 있지 않을 때 경합해서 소유권을 얻는다.
	const uint32 desired = ((LThreadID << 16) & WRITE_THREAD_MASK);

	while (true)
	{
		for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; ++spinCount)
		{
			// desired 가 expected 와 같을 때만 writeCount 를 증가
			// 이 말은 즉슨, 하위 16비트가 0 일 때만
			// 즉, Read Count 가 0일 때만 ! 아무도 공유하고 있지 않을 때만
			// 아무도 해당 자료를 읽고 있지 않을 때만 writeLock 을 건다는 것
			uint32 expected = EMPTY_FLAG;

			if (_lockFlag.compare_exchange_strong(OUT expected, desired))
			{
				// 해당 괄호 안으로 들어온다는 의미는
				// 경합에서 이겼다는 의미가 된다.
				_writeCount++;
				return;
			}
		}

		// 지나치게 spinlock 시간이 오래걸렸다는 것은
		// 문제가 발생했다는 의미이므로 
		// CRASH
		if (::GetTickCount64() - beginTick >= ACQUIRE_TIMEOUT_TICK)
		{
			CRASH("Lock Timeout"); 
		}
		
		// 5000 번 확인하면 잠시동안 쉬게 할 것이다.
		// yield : 현재 실행 중인 동등한 우선순위 이상의
		// 다른 스레드에게 실행기회 양보한다.
		// (즉, 현재 쓰레드 블로킹 상태로)
		this_thread::yield();
	}
}

void Lock::WriteUnlock(const char* name)
{
#if _DEBUG
	GDeadLockProfiler->PopLock(name);
#endif

	// ReadLock 이 다 풀기 전에는 Write Unlock 불가능하다
	if ((_lockFlag.load() & READ_COUNT_MAX) != 0)
		CRASH("INVALID UNLOCK OREDER");


	const int32 lockCount = --_writeCount;

	if (lockCount == 0)
	{
		// 상위 16비트를 0 으로 밀어줌으로써
		// 현재 lock 을 걸고 있는 쓰레드가 없다는 것을 알린다
		_lockFlag.store(EMPTY_FLAG);
	}
}

void Lock::ReadLock(const char* name)
{
#if _DEBUG
	GDeadLockProfiler->PushLock(name);
#endif

	// 1) 동일한 쓰레드가 Write Lock 을 잡고 있다면 ?
	// 그저 read cnt 1 증가
	// 어차피 WriteLock 을 해당 쓰레드가 잡고 있다는 것은
	// 다른 쓰레드가 현재 Write Lock 을 잡고 있지 못하는 것이므로 (접근 못하는 것이므로)
	// 쿨하게 +1 처리를 해줄 것이다.
	const uint32 lockThreadId = (_lockFlag.load() && WRITE_THREAD_MASK) >> 16;

	if (LThreadID == lockThreadId)
	{
		_lockFlag.fetch_add(1);
		return;
	}

	// 2) 아무도 락을 소유하고 있지 않을 때 경합해서
	// 공유 카운트를 올린다
	// Write Lock은 내 쓰레드 ID 를 상위 16 비트에 적기
	// Read Cnt 는 하위 16비트에 Read Lock 을 몇개의 쓰레드가 잡고 있는지 확인하기
	const int64 beginTick = ::GetTickCount64();

	while(true)
	{
		for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; ++spinCount)
		{
			uint32 expected = (_lockFlag.load() & READ_COUNT_MAX);

			// 정상적으로 read lock 이 걸리면 하위 16비트 값을 1 증가시킬 것이다.
			// _lockFlag 와 expected 가 동치라면, _lockFlag 값을 expected + 1 값으로 대체
			if (_lockFlag.compare_exchange_strong(OUT expected, expected + 1))
				return;

			// 실패할 경우는 2가지 이다.
			// 1) 다른 쓰레드가 write lock 을 잡는 경우
			// 2) 간발의 차이에 다른 쓰레드가 expected 값을 1 증가 (새치기)
		};

		// 지나치게 spinlock 시간이 오래걸렸다는 것은
		// 문제가 발생했다는 의미이므로 
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

	// fetch_sub : 값을 감소시키는 함수 + 감소 이전의 값을 리턴
	if ((_lockFlag.fetch_sub(1) & READ_COUNT_MAX) == 0)
	{
		// _lockFlag 가 1을 빼기 전의 값이 0이 었다는 의미가 된다.
		CRASH("MULTIPLE UNLOCK");
	}
}
