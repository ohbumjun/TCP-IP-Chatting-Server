#pragma once

class RecvBuffer
{
	// 실제 필요한 buffer Size 보다 10 배 크게 잡을 것이다
	// OnRecv 함수 참고 
	enum { BUFFER_COUNT = 10 };
public :
	RecvBuffer(int32 bufferSize);
	~RecvBuffer();

	// 1차원 배열을 이용한 큐라고 생각하면 된다.
	// [r][w][][]
	// - rd, write 가 같다는 것은 모두 읽어들인 것이므로 둘다 첫번째 위치로
	void			Clean();
	bool			OnRead(int32 numOfBytes);
	bool			OnWrite(int32 numOfBytes);

	BYTE*			ReadPos() { return &_buffer[_readPos]; }
	BYTE*			WritePos() { return &_buffer[_writePos]; }
	int32			DataSize() { return _writePos - _readPos; }
	int32			FreeSize() { return _capacity - _writePos; }
private :
	// 데이터 저장공간
	// - 바이트 배열 형태 : [][][] 
	// 실제 버퍼 크기
	int32		_capacity = 0;
	// 실질적으로 받아들일 데이터 크기
	int32		_bufferSize = 0;
	// 읽어들일 데이터 첫번째 위치
	int32		_readPos = 0;
	// 다음 쓸 데이터 위치
	// 즉, readPos ~ writePos 사이에 있는 바이트가 실질적인 데이터
	int32		 _writePos = 0;
	vector<BYTE> _buffer;
};	




