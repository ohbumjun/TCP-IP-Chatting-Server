#include "pch.h"
#include "SendBuffer.h"

/*----------------
	SendBuffer
-----------------*/

SendBuffer::SendBuffer(SendBufferChunkRef owner, BYTE* buffer, int32 allocSize) :
	_owner(owner), _buffer(buffer), _allocSize(allocSize)
{
}

SendBuffer::~SendBuffer()
{
}

// SendBuffer �ڽ��� �Ҵ���� �޸� �߿��� ���� ����ϱ� 
void SendBuffer::Close(uint32 writeSize)
{
	ASSERT_CRASH(_allocSize >= writeSize);
	_writeSize = writeSize;
	_owner->Close(writeSize);
}

/*
�Ʒ� �Լ� ��ſ� Close �Լ��� ȣ���ϰ��� �Ѵ�.
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

	// ������ �ش� Chunk �� ����ϴ� �༮�� �־��� ���� �����Ƿ�
	_usedSize = 0;
}

SendBufferRef SendBufferChunk::Open(uint32 allocSize)
{
	ASSERT_CRASH(allocSize <= SEND_BUFFER_CHUNK_SIZE);

	ASSERT_CRASH(_open == false);

	if (allocSize > FreeSize())
		return nullptr;

	_open = true;

	// SendBuffer ���� : SendBufferChunk �� �߶� ���
	// [(   )      ] => ��, ��ü SendBufferChunk [] �߿��� () �κ��� SendBuffer �� ���
	// �ش� ( ) ������ SendBuffer �� �����ϴ� ���̴�.
	// �׷��� SendBufferChunk ��, ���ο� � SendBuffer �� ( ) �� ����ϰ� �ִٸ�
	// ����� [    ] ��� SendBufferChunk �� �����Ǹ� �ȵȴ�.
	// ���� ref Cnt �� ���� ������ ���̴�.
	// ���� SendBuffer ���� �ڱⰡ ����ϰ� �ִ� SendBufferChunk ������ �˰� �ְ� �� ���̴�.
	return std::make_shared<SendBuffer>(shared_from_this(), Buffer(), allocSize);
}

// �ش� ������ŭ ���� SendBuffer���� ����ϰ� �ִٴ� �ǹ�
void SendBufferChunk::Close(uint32 writeSize)
{
	ASSERT_CRASH(_open == true);
	_open = false;
	_usedSize += writeSize;
}

/*---------------------
	SendBufferManager
----------------------*/

// ��� ���� �޸𸮸� �Ҵ����� !
// - ������ ���÷� �ڽŸ��� SendBuffer �� �ϳ��� ������ ���� ���̴�.
SendBufferRef SendBufferManager::Open(uint32 size)
{
	// ���� : Thread Local ������ ������ Lock �� �ɾ��� �ʿ䰡 ���� ���̴�.
	// ��, �� ������ ���� ���� �ٸ� SendBufferChunk �� ����Ѵ�.
	// �׸��� �Ź� �����͸� Send �� ������ , �ڱⰡ �Ҵ��� SendBufferChunk ���� �޸𸮸� ������ pooling
	if (LSendBufferChunk == nullptr)
	{
		LSendBufferChunk = Pop(); // WRITE_LOCK �ɾ��ְ� �ִ�.
		LSendBufferChunk->Reset();
	}

	// Ȥ�ó� Open �� ������ �ϰ� �ɱ�� üũ���ִ� �κ��̴�.
	bool IsOpen = LSendBufferChunk->IsOpen();
	if (IsOpen == false) //
		bool h = true;
	ASSERT_CRASH(IsOpen == false);

	// - SendBufferChunk ��� ���� ��ϰ� ū �޸� ���� [            ]
	// - �׸��� ���ʺ��� �ؼ� ���ʴ�� �޸𸮸� ����ϰ� �� ���̴�.
	// ex) [(   )  ] => �׷��� (  ) �κа� ���� ���� Ư�� �޸� ������ ����ߴٸ�
	//   �ش� ������ �������� �ʰ�, �����θ� �����ϰ� �� ���̴�.
	//   �̷��� ���� Chunk �� ���κп� ������ ���̴�. �׷��ٸ� Chunk ��ü�� ������
	//   �ٸ� �ָ� �Ҵ��ؼ� ����� ���̴�.
	//   �׷��� �����ٴ� ����, �޸� ������ �ƴϴ�. �츮�� �Ʒ����� ����
	//   shared_ptr 2��° ���ڷ� pushGlobal �� �ǹǷ�, ref cnt �� 0�� �Ǹ�
	//   �ٽ� sendBufferVector �� ���� �� ���̴�.
	
	// �� ������ ������ ���ŷ� ��ü
	// (���� ������, �츮�� �Ҵ��ؾ� �ϴ� Size ���� �۴ٸ�)
	if (LSendBufferChunk->FreeSize() < size)
	{
		LSendBufferChunk = Pop(); // WRITE_LOCK
		LSendBufferChunk->Reset();
	}

	cout << "FREE : " << LSendBufferChunk->FreeSize() << endl;

	return LSendBufferChunk->Open(size);
}

SendBufferChunkRef SendBufferManager::Pop()
{
	{
		WRITE_LOCK;
		if (_sendBufferChunks.empty() == false)
		{
			SendBufferChunkRef sendBufferChunk = _sendBufferChunks.back();
			_sendBufferChunks.pop_back();
			return sendBufferChunk;
		}
	}

	// �������� ���ٸ�, ���� ������ش�. 
	// - ?? shared_ptr �� 2��° ���ڶ�� ..?
	// - SendBufferChunk �� ��� ref cnt�� 0 �� �Ǹ�, �޸𸮸� ������ ���� �ƴ϶�
	// - pushGlobal �� ���ͼ� _sendBufferChunks ���ӿ��ٰ� �ٽ� �־��� ���̴�.
	// - ��, ������ ���������� �Ѵٴ� ���̴�.
	return SendBufferChunkRef(new SendBufferChunk, PushGlobal);
}

void SendBufferManager::Push(SendBufferChunkRef buffer)
{
	WRITE_LOCK;
	_sendBufferChunks.push_back(buffer);
}

void SendBufferManager::PushGlobal(SendBufferChunk* buffer)
{
	GSendBufferManager->Push(SendBufferChunkRef(buffer, PushGlobal));
}