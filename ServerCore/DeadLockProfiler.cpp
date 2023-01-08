#include "pch.h"
#include "DeadLockProfiler.h"

void DeadLockProfiler::PushLock(const char* name)
{
	// ��Ƽ������ ȯ�濡�� ���ư��� ���� Lock
	LockGuard guard(_lock);

	// ���̵� ã�ų� �߱��Ѵ�
	int32 lockId = 0;

	auto foundId = _nameToId.find(name);

	// �߰ߵ� �� ���� �����̶��
	if (foundId == _nameToId.end())
	{
		// id �߱����ֱ�
		lockId = static_cast<int32>(_nameToId.size());
		_nameToId[name] = lockId;
		_idToName[lockId] = name;
	}
	else
	{
		lockId = foundId->second;
	};

	// ��� �ִ� ���� �־��ٸ�
	if (LLockStack.empty() == false)
	{
		// ������ �߰ߵ��� ���� ���̽���� ����� ���� �ٽ� Ȯ��
		const int32 prevId = LLockStack.top();

		if (lockId != prevId)
		{
			// history �� ���ؼ� ����Ͽ� �׷��� ������ �׷��ִ� ���̴�.
			set<int32>& history = _lockHistory[prevId];

			if (history.find(lockId) == history.end())
			{
				history.insert(lockId);

				// ���ο� ������ �߰ߵ� ���� Cycle �˻縦 �Ѵٴ� ���� �ٽ�
				CheckCycle();
			}
		}
	}

	LLockStack.push(lockId);
};

// lockStack ���� lock �� �����ֱ�
void DeadLockProfiler::PopLock(const char* name)
{
	LockGuard guard(_lock);

	if (LLockStack.empty())
		CRASH("MULTIPLE UNLOCK");

	int32 lockId = _nameToId[name];

	// lock �Ŵ� ������ �߸��Ǿ��ٴ� �ǹ��̴�.
	if (LLockStack.top() != lockId)
		CRASH("MULTIPLE UNLOCK");

	LLockStack.pop();
}

void DeadLockProfiler::CheckCycle()
{
	// ���ݱ��� �Ͼ lock cnt �����ϱ�
	const int32 lockCount = static_cast<int32>(_nameToId.size());
	_discoveredOrder = vector<int32>(lockCount, -1);
	_discoveredCount = 0;
	_finished = vector<bool>(lockCount, false);
	_parent = vector<int32>(lockCount, -1);

	for (int32 lockId = 0; lockId < lockCount; ++lockId)
		DFS(lockId);

	// ������ �����ٸ� �����Ѵ�.
	_discoveredOrder.clear();
	_finished.clear();
	_parent.clear();
}

void DeadLockProfiler::DFS(int32 here)
{
	// �̹� �ش� ��带 �湮�� �� ����
	if (_discoveredOrder[here] != -1)
		return;

	_discoveredOrder[here] = _discoveredCount++;

	// ��� ������ ������ ��ȸ�Ѵ�.
	auto findIt = _lockHistory.find(here);

	// �ش� ������ �������� �ϴ� ���� ������ ���� ?
	// �� !
	if (findIt == _lockHistory.end())
	{
		_finished[here] = true;
		return;
	}

	set<int32>& nextSet = findIt->second;

	for (int32 there : nextSet)
	{
		// ���� �湮�� ���� ���ٸ� �湮
		if (_discoveredOrder[there] == -1)
		{
			_parent[there] = here;
			DFS(there);
			continue;
		}

		// �̹� �湮�� ���� �ִٸ�
		// ����������, ���������� Ȯ���� ���̴�.
		// here �� there ���� ���� �߰ߵǾ��ٸ�, there �� here ��
		// �ļ��̴�.
		if (_discoveredOrder[here] < _discoveredOrder[there])
			continue;

		// �������� �ƴϰ�, DFS(there)�� ���� �������� �ʾҴٸ�
		// there �� here �� �����̴� (������ ����)
		if (_finished[there] == false)
		{
			printf("%s->%s\n", _idToName[here], _idToName[there]);

			int32 now = here;

			while(true)
			{
				printf("%s->%s\n", _idToName[_parent[now]], _idToName[now]);

				now = _parent[now];

				// there : Cycle �� ������
				if (now == there)
					break;
			}

			// Cycle �� �Ͼ�� ����
			CRASH("DEAD_LOCL Detected");
		}
	}

	_finished[here] = true;
}
