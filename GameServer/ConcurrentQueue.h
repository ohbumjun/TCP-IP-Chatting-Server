#pragma once

#include <mutex>

template<typename T>
class LockQueue
{
public :
	LockQueue(){}
	LockQueue(const LockQueue&) = delete;
	LockQueue& operator = (const LockQueue&) = delete;

	void Push(T value)
	{

		lock_guard<mutex> lock(_mutex);
		_queue.push(std::move(T));

		// ��� ���� �༮�� ������ ���̴�
		_condVar.notify_one();
	}

	bool Empty()
	{
		lock_guard<mutex> lock(_mutex);
		return _queue.empty();
	}

	bool TryPop(T& value)
	{
		lock_guard<mutex> lock(_mutex);

		if (_queue.empty())
			return false;

		// ��Ƽ������ ȯ�濡���� Emtpy üũ ���� pop �� �ϸ�
		// �ƹ��� �ǹ̰� ����. Empty �� üũ�ϴ� �������� ����־
		// pop �� �ϴ� ���� �ٸ� �����尡 ������ ���� �ֱ� �����̴�.
		value = std::move(_queue.front());

		_queue.pop();

		return true;
	};

	// Data �� Empty �� �ƴ� ������ ��ٸ���
	void WaitPop(T& value)
	{
		unique_lock<mutex> lock(_mutex);

		_condVar.wait(lock, [this] {return _queue.empty() = false; });

		value = std::move(_queue.front());

		_queue.pop();
	}

private :
	queue<T> _queue;
	mutex _mutex;
	condition_variable _condVar;
};

