#include "pch.h"
#include "ThreadManager.h"
#include "CoreTLS.h"
#include "CoreGlobal.h"

ThreadManager::ThreadManager()
{
	InitTLS();
}

ThreadManager::~ThreadManager()
{
	Join();
}

void ThreadManager::Launch(function<void(void)> callback)
{
	// std::lock_guard<std::mutex> guard(_lock);

	_lock.lock();

	_threads.push_back(thread([=]()
		{
			InitTLS();
			callback();
			DestroyTLS();
		}));

	_lock.unlock();
}

void ThreadManager::Join()
{
	if (_threads.size() == 0)
		return;

	for (thread& t : _threads)
	{
		if (t.joinable())
			t.join();
	}

	_threads.clear();
}

void ThreadManager::InitTLS()
{
	// main thread 가 1번이 된다.

	static Atomic<uint32> SThreadId = 1;
	// fetch_add : 값 더하기
	LThreadID = SThreadId.fetch_add(1);
};

void ThreadManager::DestroyTLS()
{

};
