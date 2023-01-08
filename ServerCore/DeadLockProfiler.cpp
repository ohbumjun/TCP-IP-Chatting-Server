#include "pch.h"
#include "DeadLockProfiler.h"

void DeadLockProfiler::PushLock(const char* name)
{
	// 멀티쓰레드 환경에서 돌아가기 위해 Lock
	LockGuard guard(_lock);

	// 아이디를 찾거나 발급한다
	int32 lockId = 0;

	auto foundId = _nameToId.find(name);

	// 발견된 적 없는 정점이라면
	if (foundId == _nameToId.end())
	{
		// id 발급해주기
		lockId = static_cast<int32>(_nameToId.size());
		_nameToId[name] = lockId;
		_idToName[lockId] = name;
	}
	else
	{
		lockId = foundId->second;
	};

	// 잡고 있는 락이 있었다면
	if (LLockStack.empty() == false)
	{
		// 기존에 발견되지 않은 케이스라면 데드락 여부 다시 확인
		const int32 prevId = LLockStack.top();

		if (lockId != prevId)
		{
			// history 를 통해서 계속하여 그래프 구조를 그려주는 중이다.
			set<int32>& history = _lockHistory[prevId];

			if (history.find(lockId) == history.end())
			{
				history.insert(lockId);

				// 새로운 간선이 발견될 때만 Cycle 검사를 한다는 것이 핵심
				CheckCycle();
			}
		}
	}

	LLockStack.push(lockId);
};

// lockStack 에서 lock 을 꺼내주기
void DeadLockProfiler::PopLock(const char* name)
{
	LockGuard guard(_lock);

	if (LLockStack.empty())
		CRASH("MULTIPLE UNLOCK");

	int32 lockId = _nameToId[name];

	// lock 거는 순서가 잘못되었다는 의미이다.
	if (LLockStack.top() != lockId)
		CRASH("MULTIPLE UNLOCK");

	LLockStack.pop();
}

void DeadLockProfiler::CheckCycle()
{
	// 지금까지 일어난 lock cnt 조사하기
	const int32 lockCount = static_cast<int32>(_nameToId.size());
	_discoveredOrder = vector<int32>(lockCount, -1);
	_discoveredCount = 0;
	_finished = vector<bool>(lockCount, false);
	_parent = vector<int32>(lockCount, -1);

	for (int32 lockId = 0; lockId < lockCount; ++lockId)
		DFS(lockId);

	// 연산이 끝났다면 정리한다.
	_discoveredOrder.clear();
	_finished.clear();
	_parent.clear();
}

void DeadLockProfiler::DFS(int32 here)
{
	// 이미 해당 노드를 방문을 한 상태
	if (_discoveredOrder[here] != -1)
		return;

	_discoveredOrder[here] = _discoveredCount++;

	// 모든 인접한 정점을 순회한다.
	auto findIt = _lockHistory.find(here);

	// 해당 정점을 시작으로 하는 간선 정보가 없다 ?
	// 끝 !
	if (findIt == _lockHistory.end())
	{
		_finished[here] = true;
		return;
	}

	set<int32>& nextSet = findIt->second;

	for (int32 there : nextSet)
	{
		// 아직 방문한 적이 없다면 방문
		if (_discoveredOrder[there] == -1)
		{
			_parent[there] = here;
			DFS(there);
			continue;
		}

		// 이미 방문한 적이 있다면
		// 순방향인지, 역방향인지 확인할 것이다.
		// here 가 there 보다 먼저 발견되었다면, there 는 here 의
		// 후손이다.
		if (_discoveredOrder[here] < _discoveredOrder[there])
			continue;

		// 순방향이 아니고, DFS(there)가 아직 종료하지 않았다면
		// there 는 here 의 선조이다 (역방향 간선)
		if (_finished[there] == false)
		{
			printf("%s->%s\n", _idToName[here], _idToName[there]);

			int32 now = here;

			while(true)
			{
				printf("%s->%s\n", _idToName[_parent[now]], _idToName[now]);

				now = _parent[now];

				// there : Cycle 의 시작점
				if (now == there)
					break;
			}

			// Cycle 이 일어나는 상태
			CRASH("DEAD_LOCL Detected");
		}
	}

	_finished[here] = true;
}
