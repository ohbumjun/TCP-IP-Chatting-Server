#include "pch.h"
#include "SendBuffer.h"

/*----------------
	SendBuffer
-----------------*/

SendBuffer::SendBuffer(SendBufferChunkRef owner, BYTE* buffer, uint32 allocSize)
	: _owner(owner), _buffer(buffer), _allocSize(allocSize)
{
}

SendBuffer::~SendBuffer()
{
	// SendBufferChunkRef 의 ref cnt 를 1 감소
	// 결과적으로 0이 되면 SendBufferManager 의 PushGlobal 함수를 호출
}

// SendBuffer 자신이 할당받은 메모리 중에서 실제 사용하기 
void SendBuffer::Close(uint32 writeSize)
{
	ASSERT_CRASH(_allocSize >= writeSize);
	_writeSize = writeSize;
	_owner->Close(writeSize);
}

/*
아래 함수 대신에 Close 함수를 호출하고자 한다.
void SendBuffer::CopyData(uint32 writeSize)
{
	ASSERT_CRASH(_allocSize >= writeSize);
	_writeSize = writeSize;
	_owner->Close(writeSize);
}
*/

/*--------------------
	SendBufferChunk
--------------------*/

SendBufferChunk::SendBufferChunk()
{
}

SendBufferChunk::~SendBufferChunk()
{
}

void SendBufferChunk::Reset()
{
	_open = false;

	_usedSize = 0;
}

SendBufferRef SendBufferChunk::Open(uint32 allocSize)
{
	ASSERT_CRASH(allocSize <= SEND_BUFFER_CHUNK_SIZE);
	ASSERT_CRASH(_open == false);

	if (allocSize > FreeSize())
		return nullptr;

	_open = true;

	// SendBuffer 역할 : SendBufferChunk 를 잘라서 사용
	// [(   )      ] => 즉, 전체 SendBufferChunk [] 중에서 () 부분을 SendBuffer 가 사용
	// 해당 ( ) 영역을 SendBuffer 가 관리하는 것이다.
	// 그런데 SendBufferChunk 도, 내부에 어떤 SendBuffer 가 ( ) 를 사용하고 있다면
	// 절대로 [    ] 라는 SendBufferChunk 도 삭제되면 안된다.
	// 따라서 ref Cnt 를 통해 관리할 것이다.
	// 또한 SendBuffer 또한 자기가 사용하고 있는 SendBufferChunk 공간도 알고 있게 할 것이다.
	return ObjectPool<SendBuffer>::MakeShared(shared_from_this(), Buffer(), allocSize);
}

// 해당 영역만큼 실제 SendBuffer에서 사용하고 있다는 의미
void SendBufferChunk::Close(uint32 writeSize)
{
	ASSERT_CRASH(_open == true);
	_open = false;
	_usedSize += writeSize;
}

/*---------------------
	SendBufferManager
----------------------*/

// 어느 정도 메모리를 할당해줘 !
// - 쓰레드 로컬로 자신만의 SendBuffer 를 하나씩 가지고 있을 것이다.
SendBufferRef SendBufferManager::Open(uint32 size)
{
	// 참고 : Thread Local 변수는 별도로 Lock 을 걸어줄 필요가 없는 것이다.
	// 즉, 각 쓰레드 별로 서로 다른 SendBufferChunk 를 사용한다.
	// 그리고 매번 데이터를 Send 할 때마다 , 자기가 할당한 SendBufferChunk 에서 메모리를 나눠서 pooling
	if (LSendBufferChunk == nullptr)
	{
		LSendBufferChunk = Pop(); // WRITE_LOCK
		LSendBufferChunk->Reset();
	}		

	// 혹시나 Open 을 여러번 하게 될까봐 체크해주는 부분이다.
	ASSERT_CRASH(LSendBufferChunk->IsOpen() == false);

	// - SendBufferChunk 라는 것은 어마하게 큰 메모리 공간 [            ]
	// - 그리고 왼쪽부터 해서 차례대로 메모리를 사용하게 될 것이다.
	// ex) [(   )  ] => 그런데 (  ) 부분과 같이 내가 특정 메모리 공간을 사용했다면
	//   해당 공간은 재사용하지 않고, 앞으로만 진행하게 될 것이다.
	//   이러다 보면 Chunk 맨 끝부분에 도달할 것이다. 그렇다면 Chunk 전체를 날리고
	//   다른 애를 할당해서 사용할 것이다.
	//   그런데 날린다는 것은, 메모리 해제가 아니다. 우리가 아래에서 보면
	//   shared_ptr 2번째 인자로 pushGlobal 이 되므로, ref cnt 가 0이 되면
	//   다시 sendBufferVector 에 들어가게 될 것이다.

	// 다 썼으면 버리고 새거로 교체
	// (여유 공간이, 우리가 할당해야 하는 Size 보다 작다면)
	if (LSendBufferChunk->FreeSize() < size)
	{
		// Pop() 하는 순간 기존에 LSendBufferChunk 는 ref cnt 가 0이 되어, push Global 된다.
		LSendBufferChunk = Pop(); // WRITE_LOCK
		LSendBufferChunk->Reset();
	}

	cout << "FREE : " << LSendBufferChunk->FreeSize() << endl;

	return LSendBufferChunk->Open(size);
}

SendBufferChunkRef SendBufferManager::Pop()
{
	cout << "Pop SENDBUFFERCHUNK" << endl;

	{
		WRITE_LOCK;
		if (_sendBufferChunks.empty() == false)
		{
			SendBufferChunkRef sendBufferChunk = _sendBufferChunks.back();
			_sendBufferChunks.pop_back();
			return sendBufferChunk;
		}
	}

	// 여유분이 없다면, 새로 만들어준다. 
	// - ?? shared_ptr 에 2번째 인자라니 ..?
	// - SendBufferChunk 의 경우 ref cnt가 0 이 되면, 메모리를 날리는 것이 아니라
	// - pushGlobal 로 들어와서 _sendBufferChunks 모임에다가 다시 넣어줄 것이다.
	// - 즉, 재사용을 무한정으로 한다는 것이다.
	return SendBufferChunkRef(xnew<SendBufferChunk>(), PushGlobal);
}

void SendBufferManager::Push(SendBufferChunkRef buffer)
{
	WRITE_LOCK;
	_sendBufferChunks.push_back(buffer);
}

void SendBufferManager::PushGlobal(SendBufferChunk* buffer)
{
	// (참고 : SendBufferManager::Pop())
	// 주의사항 : SendBufferChunk 의 경우, 각 쓰레드에서 하나씩 사용되는데 
	// 쓰레드가 종료될 때, 즉, 클라이언트가 종료될 때 계속해서 메모리 해제되지 않고
	// 재사용되므로 메모리 해제가 되지 않을 수도 있다.
	// 따라서 정상적으로 메모리 해제가 이루어지는지 확인하기 위해
	// 로그를 찍어줄 것이다.
	cout << "PushGlobal SENDBUFFERCHUNK" << endl;

	GSendBufferManager->Push(SendBufferChunkRef(buffer, PushGlobal));
}