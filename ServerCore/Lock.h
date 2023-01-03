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
public :
	void WriteLock();
	void WriteUnlock();
	void ReadLock();
	void ReadUnlock();

private :
	Atomic<uint32> _lockFlag = EMPTY_FLAG;

	// WriteLock �� lock �� ��������� ȣ������ ���� �ִ�.
	// �̶����� lock �� �� ������ �����ϱ� ����
	// _writeCount ��� ������ ����� ���̴�.
	uint16 _writeCount = 0;
};

/*--- Lock Guard ---*/
class ReadLockGuard
{
public :
	ReadLockGuard(Lock& lock) : _lock(lock) { _lock.ReadLock(); };
	~ReadLockGuard() { _lock.ReadUnlock(); };

private :
	Lock& _lock;
};

class WriteLockGuard
{
public:
	WriteLockGuard(Lock& lock) : _lock(lock) { _lock.WriteLock(); };
	~WriteLockGuard() { _lock.WriteUnlock(); };

private:
	Lock& _lock;
};