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
	// SendBuffer �� ��ü������ buffer �� ��� ���� ���
	// vector<BYTE>	_buffer;
	
	// (�Ʒ� ������ : SendBufferChunk::Open �Լ� ����)
	// SendBufferChunk �� �Ϻ� �޸� ������ ����Ű�� ������
	BYTE* _buffer;
	// SendBuffer �� ����ϴ� ��ü �޸� ũ�� 
	uint32 _allocSize = 0;
	// SendBuffer �� ����ϴ� ��ü �޸� �߿��� ���� ����ϴ� �޸� ũ��
	uint32 _writeSize = 0;
	SendBufferChunkRef _owner;
};

/*--------------------
	SendBufferChunk
--------------------*/

// SendBufferManager ���� TLS �� ����ؼ� BufferChunk �� �������� ������
// �⺻������ �̱� ������ ȯ�濡�� SendBufferChunk �� �����ϰ� �ִ� ���̴�.
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
	// �Ҵ��� ����
	SendBufferRef		Open(uint32 allocSize);
	// ���������� ����� ����
	void				Close(uint32 writeSize);

	// ������ ���� ��� �����͸� �����ϰ� �ִ°�.
	bool				IsOpen() { return _open; }
	// &_buffer[_usedSize] : _usedSize ������ġ�� ��������Ƿ�, �� ���� ��ġ����
	BYTE* Buffer() { return &_buffer[_usedSize]; }
	// ��밡���� ����
	uint32				FreeSize() { return static_cast<uint32>(_buffer.size()) - _usedSize; }

private:
	array<BYTE, SEND_BUFFER_CHUNK_SIZE>		_buffer = {};
	bool									_open = false;
	// SEND_BUFFER_CHUNK_SIZE �߿��� ���� �󸾸�ŭ ����ߴ°�.
	uint32									_usedSize = 0;
};

/*---------------------
	SendBufferManager
----------------------*/

class SendBufferManager
{
public:
	// ŭ������ SendBuffer ������� ����Ҹ�ŭ�� �ɰ��� ����ϰڴ�.
	// �ɰ����ڴ�. => Open �� �ش��ϴ� ������ �����ϴ� ��
	// Close �Լ��� ȣ���� �Ǹ�, �ش� ������ ��¥ ����ϴ� ũ��
	// [(Open)         ]
	SendBufferRef		Open(uint32 size);

private:
	// SendBufferChunk �� �ϳ� ���� ���ڴ�.
	SendBufferChunkRef	Pop();

	// SendBufferChunk ��� ���� �� �����ߴ�, �޸𸮰� ���ٸ�
	// �̰��� Ǯ�� �ݳ�
	void				Push(SendBufferChunkRef buffer);

	static void			PushGlobal(SendBufferChunk* buffer);

private:
	USE_LOCK;
	vector<SendBufferChunkRef> _sendBufferChunks;
};
