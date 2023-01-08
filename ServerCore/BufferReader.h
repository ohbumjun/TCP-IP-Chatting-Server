#pragma once

/*----------------
	BufferReader
-----------------*/

class BufferReader
{
public:
	BufferReader();
	BufferReader(BYTE* buffer, uint32 size, uint32 pos = 0);
	~BufferReader();

	BYTE* Buffer() { return _buffer; }
	uint32			Size() { return _size; }
	uint32			ReadSize() { return _pos; }
	// 안 읽은 데이터가 얼마나 있는가.
	uint32			FreeSize() { return _size - _pos; }

	// 데이터를 읽기는 하되, 커서는 옮기지 않는 것.
	template<typename T>
	bool			Peek(T* dest) { return Peek(dest, sizeof(T)); }
	bool			Peek(void* dest, uint32 len);

	template<typename T>
	bool			Read(T* dest) { return Read(dest, sizeof(T)); }
	bool			Read(void* dest, uint32 len);

	// cout << 을 오버로딩 한 것
	template<typename T>
	BufferReader& operator>>(OUT T& dest);

private:
	// 시작 주소
	BYTE* _buffer = nullptr;
	// 버퍼의 크기 
	uint32			_size = 0;
	// 지금 어디까지 읽었느냐 
	uint32			_pos = 0;
};

// 실질적으로 데이터를 읽기 위한 함수 
template<typename T>
inline BufferReader& BufferReader::operator>>(OUT T& dest)
{
	// dest에 데이터를 복사해주기 위한 것
	dest = *reinterpret_cast<T*>(&_buffer[_pos]);
	// 읽은 만큼 커서를 뒤로 옮겨주기 위한 것
	_pos += sizeof(T);
	return *this;
}