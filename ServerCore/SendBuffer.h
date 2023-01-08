#pragma once

/*----------------
	SendBuffer
-----------------*/

class SendBuffer : public enable_shared_from_this<SendBuffer>
{
public:
	// SendBuffer(int32 bufferSize);
	SendBuffer(SendBufferChunkRef owner, BYTE* buffer, int32 allocSize);
	~SendBuffer();

	BYTE* Buffer() { return _buffer; }
	int32 WriteSize() { return _writeSize; }

	// int32 Capacity() { return static_cast<int32>(_buffer.size()); }
	void Close(uint32 writeSize);
	// void CopyData(uint32 writeSize);

private:
	// SendBuffer 가 자체적으로 buffer 를 들고 있을 경우
	// vector<BYTE>	_buffer;
	
	// (아래 변수들 : SendBufferChunk::Open 함수 참고)
	// SendBufferChunk 내 일부 메모리 공간을 가리키는 포인터
	BYTE* _buffer;
	// SendBuffer 가 사용하는 전체 메모리 크기 
	uint32 _allocSize = 0;
	// SendBuffer 가 사용하는 전체 메모리 중에서 실제 사용하는 메모리 크기
	uint32 _writeSize = 0;
	SendBufferChunkRef _owner;
};

/*--------------------
	SendBufferChunk
--------------------*/

// SendBufferManager 에서 TLS 를 사용해서 BufferChunk 를 꺼내오기 때문에
// 기본적으로 싱글 쓰레드 환경에서 SendBufferChunk 에 접근하고 있는 것이다.
class SendBufferChunk : public enable_shared_from_this<SendBufferChunk>
{
	enum
	{
		SEND_BUFFER_CHUNK_SIZE = 6000
	};

public:
	SendBufferChunk();
	~SendBufferChunk();

	void				Reset();
	// 할당할 공간
	SendBufferRef		Open(uint32 allocSize);
	// 실질적으로 사용할 공간
	void				Close(uint32 writeSize);

	// 누군나 나를 열어서 데이터를 기입하고 있는가.
	bool				IsOpen() { return _open; }
	// &_buffer[_usedSize] : _usedSize 이전위치는 사용했으므로, 그 다음 위치부터
	BYTE* Buffer() { return &_buffer[_usedSize]; }
	// 사용가능한 공간
	uint32				FreeSize() { return static_cast<uint32>(_buffer.size()) - _usedSize; }

private:
	array<BYTE, SEND_BUFFER_CHUNK_SIZE>		_buffer = {};
	bool									_open = false;
	// SEND_BUFFER_CHUNK_SIZE 중엣서 내가 얼맘만큼 사용했는가.
	uint32									_usedSize = 0;
};

/*---------------------
	SendBufferManager
----------------------*/

class SendBufferManager
{
public:
	// 큼지막한 SendBuffer 덩어리에서 사용할만큼을 쪼개서 사용하겠다.
	// 쪼개가겠다. => Open 에 해당하는 영역을 예약하는 것
	// Close 함수를 호출학 되면, 해당 영역이 진짜 사용하는 크기
	// [(Open)         ]
	SendBufferRef		Open(uint32 size);

private:
	// SendBufferChunk 를 하나 꺼내 쓰겠다.
	SendBufferChunkRef	Pop();

	// SendBufferChunk 라는 것을 다 소진했다, 메모리가 없다면
	// 이것을 풀에 반납
	void				Push(SendBufferChunkRef buffer);

	static void			PushGlobal(SendBufferChunk* buffer);

private:
	USE_LOCK;
	vector<SendBufferChunkRef> _sendBufferChunks;
};
