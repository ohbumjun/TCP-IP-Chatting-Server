#pragma once

// Packet ID 
enum
{
	// Server에서 Client로 보내는 것
	S_TEST = 1
};

// Packet ID 에 따라서 서로 다른 처리를 진행해주기 위한 것
class ClientPacketHandler
{
public:
	static void HandlePacket(BYTE* buffer, int32 len);

	static void Handle_S_TEST(BYTE* buffer, int32 len);
};

template<typename T, typename C>
class PacketIterator
{
public :
	PacketIterator(C& container, uint16 index) :
		_container(container), _index(index){}

	bool operator != (const PacketIterator& other) const
	{
		return _index != other._index;
	}

	const T& operator * () const
	{
		return _container[_index];
	}

	T& operator* ()
	{
		return _container[_index];
	};

	T* operator->() { return &_container[_index]; }

	PacketIterator& operator ++() { _index++; return *this; }
	PacketIterator& operator ++(int32) { 
		PacketIterator ret = *this;
		_index++; 
		return ret; 
	}
private :
	C& _container;
	uint16 _index;
};

// T : 데이터 
template<typename T>
class PacketList
{
public :
	PacketList() : _data(nullptr), _count(0){}
	PacketList(T* data, uint16 count) : _data(data), _count(count){}

	T& operator [] (uint16 index)
	{
		ASSERT_CRASH(index < _count);
		return _data[index];
	}

	uint16 Count() { return _count; }

	// ranged - for 
	PacketIterator<T, PacketList<T>> begin()
	{
		return PacketIterator<T, PacketList<T>>(*this, 0);
	}
	PacketIterator<T, PacketList<T>> end()
	{
		return PacketIterator<T, PacketList<T>>(*this, _count);
	}
private :
	T* _data;
	uint16 _count;
};