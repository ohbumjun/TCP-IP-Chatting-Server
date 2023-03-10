#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"
#include "Memory.h"
#include "DeadLockProfiler.h"
#include "SocketUtils.h"
#include "SendBuffer.h"

ThreadManager*		GThreadManager = nullptr;
Memory*				GMemory = nullptr;
SendBufferManager*	GSendBufferManager = nullptr;

DeadLockProfiler*	GDeadLockProfiler = nullptr;

// 차후 여러개의 Manager Class 가 만들어질 수 있다.
// 그리고 Manager Class 들간의 생성 및 해제 순서가 정해져 있을 수 있다.
// 이를 별도로 관리해주기 위함이다.
class CoreGlobal
{
public:
	CoreGlobal()
	{
		GThreadManager = new ThreadManager();
		GMemory = new Memory();
		GSendBufferManager = new SendBufferManager();
		GDeadLockProfiler = new DeadLockProfiler();
		SocketUtils::Init();
	}

	~CoreGlobal()
	{
		delete GThreadManager;
		delete GMemory;
		delete GSendBufferManager;
		delete GDeadLockProfiler;
		SocketUtils::Clear();
	}
} GCoreGlobal;