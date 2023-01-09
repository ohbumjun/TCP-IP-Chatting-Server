#pragma once

// 99.99 % �� ��ȭ�� ���µ�
// 0.01 % �� ��ȭ�Ѵ�. �׷��� �̶����� lock �� �ɾ��ָ�
// �Ʊ���.
// ���� read �Ҷ��� lock X
// write �Ҷ��� lock �ϰ� �� ���̴�.

/* ---------------
���� 16��Ʈ, ���� 16��Ʈ => 32 ��Ʈ Ȱ��
- ���� 16 ��Ʈ : Exclusive Lock Owner ThreadID => lock �� ȹ���� thread ID
- ���� 16 ��Ʈ : �������� Read Cnt
-----------------*/

/*--- RW SpinLock ---*/
// W -> W (o)
// W -> R (o) : ������ �����尡 Write Lock�� ��� ��Ȳ���� Read Lock ��� Ok 
// R -> W (x) : ������ �����尡 Read Lock�� ���� ��Ȳ���� Write Lock ��� X
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

	// WriteLock �� lock �� ��������� ȣ������ ���� �ִ�.
	// �̶����� lock �� �� ������ �����ϱ� ����
	// _writeCount ��� ������ ����� ���̴�.
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