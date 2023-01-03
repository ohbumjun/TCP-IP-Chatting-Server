#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"

ThreadManager* GThreadManager = nullptr;

// ���� �������� Manager Class �� ������� �� �ִ�.
// �׸��� Manager Class �鰣�� ���� �� ���� ������ ������ ���� �� �ִ�.
// �̸� ������ �������ֱ� �����̴�.
class CoreGlobal
{
public:
	CoreGlobal();
	~CoreGlobal();
};

CoreGlobal::CoreGlobal()
{
	GThreadManager = new ThreadManager();
}

CoreGlobal::~CoreGlobal()
{
	delete GThreadManager;
}
