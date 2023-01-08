#pragma once

class RecvBuffer
{
	// ���� �ʿ��� buffer Size ���� 10 �� ũ�� ���� ���̴�
	// OnRecv �Լ� ���� 
	enum { BUFFER_COUNT = 10 };
public :
	RecvBuffer(int32 bufferSize);
	~RecvBuffer();

	// 1���� �迭�� �̿��� ť��� �����ϸ� �ȴ�.
	// [r][w][][]
	// - rd, write �� ���ٴ� ���� ��� �о���� ���̹Ƿ� �Ѵ� ù��° ��ġ��
	void			Clean();
	bool			OnRead(int32 numOfBytes);
	bool			OnWrite(int32 numOfBytes);

	BYTE*			ReadPos() { return &_buffer[_readPos]; }
	BYTE*			WritePos() { return &_buffer[_writePos]; }
	int32			DataSize() { return _writePos - _readPos; }
	int32			FreeSize() { return _capacity - _writePos; }
private :
	// ������ �������
	// - ����Ʈ �迭 ���� : [][][] 
	// ���� ���� ũ��
	int32		_capacity = 0;
	// ���������� �޾Ƶ��� ������ ũ��
	int32		_bufferSize = 0;
	// �о���� ������ ù��° ��ġ
	int32		_readPos = 0;
	// ���� �� ������ ��ġ
	// ��, readPos ~ writePos ���̿� �ִ� ����Ʈ�� �������� ������
	int32		 _writePos = 0;
	vector<BYTE> _buffer;
};	




