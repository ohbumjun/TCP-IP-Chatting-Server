#include "pch.h"
#include "RecvBuffer.h"

/*--------------
	RecvBuffer
----------------*/

RecvBuffer::RecvBuffer(int32 bufferSize) : _bufferSize(bufferSize)
{
	_capacity = bufferSize * BUFFER_COUNT;
	_buffer.resize(_capacity);
}

RecvBuffer::~RecvBuffer()
{
}

void RecvBuffer::Clean()
{
	// read, write 위치 확정 되면
	// ex) [][][r][][w]...

	// 1) r,w 같은 위치 ? == 데이터 모두 처리
	// - 두 커서를 맨 앞으로 복사 비용 없이 이동

	// 2) 둘다 끝쪽에 위치하면, 뒤쪽에 위치한 유효한 데이터를
	// - 처음 위치로 이동시킨다. (앞으로 복사)
	int32 dataSize = DataSize();
	if (dataSize == 0)
	{
		// 딱 마침 읽기+쓰기 커서가 동일한 위치라면, 둘 다 리셋.
		_readPos = _writePos = 0;
	}
	else
	{
		// 사실 이부분은 되도록 안하면 좋다.
		// 그렇다면 어떻게 해야 하는가 ?
		// 4byte 라고 한다면
		// [][][][],[][][][], [][][][], [][][][]
		// 와 같이 buffer 크기를 훨씬 크게 잡으면
		// r == w 가 언젠가 될 확률이 커진다.
		// 따라서 실제 버퍼의 총 크기는 _capacity
		// 실제 송수신할 데이터 크기는 _bufferSize

		// 여유 공간이 버퍼 1개 크기 미만이면, 데이터를 앞으로 땅긴다. (복사)
		if (FreeSize() < _bufferSize)
		{
			::memcpy(&_buffer[0], &_buffer[_readPos], dataSize);
			_readPos = 0;
			_writePos = dataSize;
		}
	}
}

// 성공적으로 데이터를 읽어들였다면
// 커서를 앞으로 땡기는 작업을 진행할 것이다.
bool RecvBuffer::OnRead(int32 numOfBytes)
{
	if (numOfBytes > DataSize())
		return false;

	_readPos += numOfBytes;
	return true;
}

bool RecvBuffer::OnWrite(int32 numOfBytes)
{
	if (numOfBytes > FreeSize())
		return false;

	_writePos += numOfBytes;
	return true;
}