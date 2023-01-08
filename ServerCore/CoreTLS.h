#pragma once
#include <stack>
// TLS : Thread Local Storage

extern thread_local uint32 LThreadID;
extern thread_local std::stack<int32> LLockStack;
// SendBuffer 를 크게 관리하는 녀석
extern thread_local SendBufferChunkRef LSendBufferChunk;
