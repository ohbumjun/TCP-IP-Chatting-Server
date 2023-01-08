#pragma once

/*----------------
	BufferWriter
-----------------*/

class BufferWriter
{
public:
	BufferWriter();
	BufferWriter(BYTE* buffer, uint32 size, uint32 pos = 0);
	~BufferWriter();

	BYTE* Buffer() { return _buffer; }
	uint32			Size() { return _size; }
	uint32			WriteSize() { return _pos; }
	uint32			FreeSize() { return _size - _pos; }

	template<typename T>
	bool			Write(T* src) { return Write(src, sizeof(T)); }
	bool			Write(void* src, uint32 len);

	template<typename T>
	T* Reserve();

	// 실질적으로 데이터를 쓰는 작업 (l-value, r-value 버전 둘다 만들기)
	template<typename T>
	BufferWriter& operator<<(T&& src);

private:
	// 버퍼 시작 위치
	BYTE* _buffer = nullptr;
	// 버퍼 크기
	uint32			_size = 0;
	// 어디까지 데이터를 썼는가 
	uint32			_pos = 0;
};

template<typename T>
T* BufferWriter::Reserve()
{
	if (FreeSize() < sizeof(T))
		return nullptr;

	T* ret = reinterpret_cast<T*>(&_buffer[_pos]);
	_pos += sizeof(T);
	return ret;
}

template<typename T>
BufferWriter& BufferWriter::operator<<(T&& src)
{
	// Reference 제거
	// ex) int& => int
	using DataType = std::remove_reference_t<T>;

	*reinterpret_cast<DataType*>(&_buffer[_pos]) = std::forward<DataType>(src);
	_pos += sizeof(T);
	return *this;
}