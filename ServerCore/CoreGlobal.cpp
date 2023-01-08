#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"
#include "DeadLockProfiler.h"
#include "SocketUtils.h"
#include "SendBuffer.h"

SendBufferManager*	GSendBufferManager = nullptr;
ThreadManager*		GThreadManager = nullptr;
DeadLockProfiler*	GDeadLockProfiler = nullptr;

// 차후 여러개의 Manager Class 가 만들어질 수 있다.
// 그리고 Manager Class 들간의 생성 및 해제 순서가 정해져 있을 수 있다.
// 이를 별도로 관리해주기 위함이다.
class CoreGlobal
{
public:
	CoreGlobal();
	~CoreGlobal();
} GCoreGlobal; // 선언과 동시에 인스턴스 1개 생성

CoreGlobal::CoreGlobal()
{
	GThreadManager = new ThreadManager();
	GDeadLockProfiler = new DeadLockProfiler();
	GSendBufferManager = new SendBufferManager();
	SocketUtils::Init();
}

CoreGlobal::~CoreGlobal()
{
	delete GThreadManager;
	delete GDeadLockProfiler;
	delete GSendBufferManager;
	SocketUtils::Clear();

}
