#pragma once

#include <cassert>

#define OUT

/*---Lock---*/
// typeid(this).name() : 런타임시 lock을 거는 객체의 주소값 (this)을 이용
#define USE_MANY_LOCKS(count)   Lock _locks[count];
#define USE_LOCK				USE_MANY_LOCKS(1);
#define READ_LOCK_IDX(idx)		ReadLockGuard readLockGuard_##idx(_locks[idx], typeid(this).name());
// #define READ_LOCK_IDX(1)		ReadLockGuard readLockGuard_1;
#define READ_LOCK				READ_LOCK_IDX(0);
#define WRITE_LOCK_IDX(idx)		WriteLockGuard writeLockGuard_##idx(_locks[idx], typeid(this).name());
#define WRITE_LOCK				WRITE_LOCK_IDX(0);


/*---Crash---*/

#define CRASH(cause)					\
{										\
	uint32* crash = nullptr;			\
	__analysis_assume(crash != nullptr);\
	*crash = 0xDEADBEEF;				\
};

#define ASSERT_CRASH(expr)		\
{								\
	if (!(expr))				\
	{							\
		CRASH("ASSERT_CRASH");  \
		__analysis_assume(expr);\
	};							\
};