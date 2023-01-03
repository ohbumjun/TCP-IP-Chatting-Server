#pragma once

#include <mutex>

// ��Ƽ������ ȯ�濡�� �����ϴ� Stack
template<typename T>
class LockStack
{
public :
	LockStack() {};
	LockStack(const LockStack&) = delete;
	LockStack& operator = (const LockStack&) = delete;

	void Push(T value)
	{
		lock_guard<mutex> lock(_mutex);
		_stack.push(std::move(T));

		// ��� ���� �༮�� ������ ���̴�
		_condVar.notify_one();
	}

	bool Empty()
	{
		lock_guard<mutex> lock(_mutex);
		return _stack.empty();
	}

	bool TryPop(T& value)
	{
		lock_guard<mutex> lock(_mutex);

		if (_stack.empty())
			return false;

		// ��Ƽ������ ȯ�濡���� Emtpy üũ ���� pop �� �ϸ�
		// �ƹ��� �ǹ̰� ����. Empty �� üũ�ϴ� �������� ����־
		// pop �� �ϴ� ���� �ٸ� �����尡 ������ ���� �ֱ� �����̴�.
		value = std::move(_stack.top());

		_stack.pop();

		return true;
	};

	// Data �� Empty �� �ƴ� ������ ��ٸ���
	void WaitPop(T& value)
	{
		unique_lock<mutex> lock(_mutex);

		_condVar.wait(lock, [this] {return _stack.empty() = false; });

		value = std::move(_stack.top());

		_stack.pop();
	}

private :
	stack<T> _stack;
	mutex _mutex;
	condition_variable _condVar;
};

