#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"
#include "DeadLockProfiler.h"
#include "SocketUtils.h"

ThreadManager* GThreadManager = nullptr;
DeadLockProfiler* GDeadLockProfiler = nullptr;

// ���� �������� Manager Class �� ������� �� �ִ�.
// �׸��� Manager Class �鰣�� ���� �� ���� ������ ������ ���� �� �ִ�.
// �̸� ������ �������ֱ� �����̴�.
class CoreGlobal
{
public:
	CoreGlobal();
	~CoreGlobal();
} GCoreGlobal; // ����� ���ÿ� �ν��Ͻ� 1�� ����

CoreGlobal::CoreGlobal()
{
	GThreadManager = new ThreadManager();
	GDeadLockProfiler = new DeadLockProfiler();
	SocketUtils::Init();
}

CoreGlobal::~CoreGlobal()
{
	delete GThreadManager;
	delete GDeadLockProfiler;
	SocketUtils::Clear();

}
