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
	// read, write ��ġ Ȯ�� �Ǹ�
	// ex) [][][r][][w]...

	// 1) r,w ���� ��ġ ? == ������ ��� ó��
	// - �� Ŀ���� �� ������ ���� ��� ���� �̵�

	// 2) �Ѵ� ���ʿ� ��ġ�ϸ�, ���ʿ� ��ġ�� ��ȿ�� �����͸�
	// - ó�� ��ġ�� �̵���Ų��. (������ ����)
	int32 dataSize = DataSize();
	if (dataSize == 0)
	{
		// �� ��ħ �б�+���� Ŀ���� ������ ��ġ���, �� �� ����.
		_readPos = _writePos = 0;
	}
	else
	{
		// ��� �̺κ��� �ǵ��� ���ϸ� ����.
		// �׷��ٸ� ��� �ؾ� �ϴ°� ?
		// 4byte ��� �Ѵٸ�
		// [][][][],[][][][], [][][][], [][][][]
		// �� ���� buffer ũ�⸦ �ξ� ũ�� ������
		// r == w �� ������ �� Ȯ���� Ŀ����.
		// ���� ���� ������ �� ũ��� _capacity
		// ���� �ۼ����� ������ ũ��� _bufferSize

		// ���� ������ ���� 1�� ũ�� �̸��̸�, �����͸� ������ �����. (����)
		if (FreeSize() < _bufferSize)
		{
			::memcpy(&_buffer[0], &_buffer[_readPos], dataSize);
			_readPos = 0;
			_writePos = dataSize;
		}
	}
}

// ���������� �����͸� �о�鿴�ٸ�
// Ŀ���� ������ ����� �۾��� ������ ���̴�.
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