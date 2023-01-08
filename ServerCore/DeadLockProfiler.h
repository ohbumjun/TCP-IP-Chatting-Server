#pragma once

#include <stack>
#include <map>
#include <vector>

// Lock Ŭ���� ������ WriteLock, ReadLock ���� �� ������
// Debug �󿡼� DeadLockProfiler ���� ������ �ְ�
// DeadLockProfiler ���� DFS �� ���� DeadLock ���θ� �Ǵ��� ���̴�.

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

	// key : Ư�� ���� => �ش� ������ �������� �Ͽ� Ÿ��Ÿ�� ����
	// ���� �������� set<int32> �� ��Ƶδ� ���̴�.
	map<int32, set<int32>>			  _lockHistory;

	Mutex _lock;

	// _lockStack �� TLS �� ������� ���̴�.
	// ��, �����帶�� �ɰ� �ִ� lock �� ����, _lockStack �� �ٸ��� �� ���̴�.
	// (CoreTLS ����)
	// stack<int32>					  LLockStack;

private :
	// cycle �˻��� �� �ʿ��� �߰� �ӽ� ������
	vector<int32> _discoveredOrder; // ��尡 �߰ߵ� ������ ����ϴ� �迭
	int32 _discoveredCount = 0;     // ��尡 �߰ߵ� ����
	vector<bool> _finished;			// DFS(i) �� ����Ǿ����� ���� 
	vector<int32> _parent;			// 

};

