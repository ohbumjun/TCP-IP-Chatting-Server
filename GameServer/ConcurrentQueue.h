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

		// 대기 중인 녀석을 깨워줄 것이다
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

		// 멀티쓰레드 환경에서는 Emtpy 체크 이후 pop 을 하면
		// 아무런 의미가 없다. Empty 를 체크하는 순간에는 비어있어도
		// pop 을 하는 순간 다른 쓰레드가 꺼내올 수도 있기 때문이다.
		value = std::move(_queue.front());

		_queue.pop();

		return true;
	};

	// Data 가 Empty 가 아닐 때까지 기다리기
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

