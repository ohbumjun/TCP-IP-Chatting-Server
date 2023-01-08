#pragma once

#include <stack>
#include <map>
#include <vector>

// Lock 클래스 내에서 WriteLock, ReadLock 등을 할 때마다
// Debug 상에서 DeadLockProfiler 으로 정보를 주고
// DeadLockProfiler 에서 DFS 를 통해 DeadLock 여부를 판단할 것이다.

class DeadLockProfiler
{
public :
	void PushLock(const char* name);
	void PopLock(const char* name);
	void CheckCycle();
private :
	void DFS(int32 index);
private :
	unordered_map<const char*, int32> _nameToId;
	unordered_map<int32, const char*> _idToName;

	// key : 특정 정점 => 해당 정점을 시작으로 하여 타고타고 가는
	// 간선 정보들을 set<int32> 에 모아두는 것이다.
	map<int32, set<int32>>			  _lockHistory;

	Mutex _lock;

	// _lockStack 을 TLS 로 만들어줄 것이다.
	// 즉, 쓰레드마다 걸고 있는 lock 의 순서, _lockStack 이 다르게 될 것이다.
	// (CoreTLS 참고)
	// stack<int32>					  LLockStack;

private :
	// cycle 검사할 때 필요한 중간 임시 변수들
	vector<int32> _discoveredOrder; // 노드가 발견된 순서를 기록하는 배열
	int32 _discoveredCount = 0;     // 노드가 발견된 순서
	vector<bool> _finished;			// DFS(i) 가 종료되었는지 여부 
	vector<int32> _parent;			// 

};

