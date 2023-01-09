#pragma once

// 99.99 % 는 변화가 없는데
// 0.01 % 만 변화한다. 그런데 이때마다 lock 을 걸어주면
// 아깝다.
// 따라서 read 할때는 lock X
// write 할때만 lock 하게 할 것이다.

/* ---------------
상위 16비트, 하위 16비트 => 32 비트 활용
- 상위 16 비트 : Exclusive Lock Owner ThreadID => lock 을 획득한 thread ID
- 하위 16 비트 : 공유중인 Read Cnt
-----------------*/

/*--- RW SpinLock ---*/
// W -> W (o)
// W -> R (o) : 동일한 쓰레드가 Write Lock을 잡는 상황에서 Read Lock 잡기 Ok 
// R -> W (x) : 동일한 쓰레드가 Read Lock을 잡은 상황에서 Write Lock 잡기 X
class Lock
{
	enum : uint32
	{
		ACQUIRE_TIMEOUT_TICK = 10000,
		MAX_SPIN_COUNT = 5000,
		WRITE_THREAD_MASK = 0xFFFF'0000,
		READ_COUNT_MAX = 0x0000'FFFF,
		EMPTY_FLAG = 0x0000'0000
	};
public:
	void WriteLock(const char* name);
	void WriteUnlock(const char* name);
	void ReadLock(const char* name);
	void ReadUnlock(const char* name);

private:
	Atomic<uint32> _lockFlag = EMPTY_FLAG;

	// WriteLock 등 lock 을 재귀적으로 호출해줄 수도 있다.
	// 이때마다 lock 을 건 개수를 추적하기 위해
	// _writeCount 라는 변수를 사용할 것이다.
	uint16 _writeCount = 0;
};

/*--- Lock Guard ---*/
class ReadLockGuard
{
public:
	ReadLockGuard(Lock& lock, const char* name) :
		_lock(lock), _name(name) {
		_lock.ReadLock(_name);
	};
	~ReadLockGuard() { _lock.ReadUnlock(_name); };

private:
	Lock& _lock;
	const char* _name;
};

class WriteLockGuard
{
public:
	WriteLockGuard(Lock& lock, const char* name) :
		_lock(lock), _name(name) {
		_lock.WriteLock(_name);
	};
	~WriteLockGuard() { _lock.WriteUnlock(_name); };

private:
	Lock& _lock;
	const char* _name;
};