#pragma once
#include "BufferReader.h"
#include "BufferWriter.h"
#include "Protocol.pb.h"

enum
{
	S_TEST = 1
};

class ServerPacketHandler
{
public:
	static void HandlePacket(BYTE* buffer, int32 len);

	static SendBufferRef MakeSendBuffer(Protocol::S_TEST& pkt);
};

// 여러 형태의 packet 이 있을 수 있으므로 template 으로 세팅할 것이다.
template<typename T>
SendBufferRef _MakeSendBuffer(T& pkt, uint16 pktId)
{
	// 데이터의 크기 세팅 
	const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
	// 패킷에 들어있는 전체 크기 (Packet Header 를 만들어서 크기, Protocol ID 세팅)
	const uint16 packetSize = dataSize + sizeof(PacketHeader);

	// 필요한 만큼 Open 한다.
	SendBufferRef sendBuffer = GSendBufferManager->Open(packetSize);

	// 시작 주소 가져오기 => packet Header 값에 접근하기 
	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	header->size = packetSize;
	header->id = pktId;

	// Serialize 할 것이다
	// 1) 데이터의 시작 주소를 넘겨줘야 한다. => Packet Header 만큼 건너 뛰기
	//    header[1] ? => header 가 4byte => header[1] 은 곧 시작 주소로 부터 4byte 이동
	ASSERT_CRASH(pkt.SerializeToArray(&header[1], dataSize));

	// 끝날 때 Close
	sendBuffer->Close(packetSize);

	return sendBuffer;
}

/*자체 패킷 사용시 코드*/
/*

template<typename T, typename C>
class PacketIterator
{
public:
	PacketIterator(C& container, uint16 index) :
		_container(container), _index(index) {}

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
private:
	C& _container;
	uint16 _index;
};

// T : 데이터
template<typename T>
class PacketList
{
public:
	PacketList() : _data(nullptr), _count(0) {}
	// data 의 시작 주소값
	PacketList(T* data, uint16 count) : _data(data), _count(count) {}

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
private:
	T* _data;
	uint16 _count;
};


#pragma pack(1)
struct PKT_S_TEST
{
	struct BuffListItem
	{
		uint64 buffId;
		float remainTime;

		// 가변 데이터 내에도 추가적인 가변 데이터가 들어가는 형태
		// ex) 2차원 배열 형태라고 생각하면 된다.
		// ex) 피해를 입은 ? 사용자 목록 정보를 넘겨준다고 가정하자.
		uint16 victimsOffset;
		uint16 victimsCount;
	};

	// packet Header 까지 포함시키기
	uint16 packetSize;
	uint16 packetId;

	uint64 id;
	uint32 hp;
	uint16 attack;

	// 가변 데이터 묘사하는 Header
	// [PKT_S_TEST][BufferData, BufferData, BufferData]
	//			중에서 BufferData 가 시작하는 위치
	uint16 buffsOffset;

	// 가변데이터 총 개수
	uint16 buffsCount;

	// 아래와 같이 내부에 데이터를 받아들일 필요 없이
	// 1) 문자열 (ex. name)
	// 2) 그냥 바이트 배열 (ex. 길드 이미지)
	// 3) 일반 리스트
	// vector<BuffListItem> buffs;
	// wstring name;

	// 보내는 측에서는 Validata 할 필요 X
	// bool Validate();

	// 가변 데이터들의 배열 형태
	using BuffsList = PacketList<PKT_S_TEST::BuffListItem>;

	BuffsList GetBuffsList()
	{
		// 시작 주소 + offset == 가변 데이터 있는 시작 위치
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += buffsOffset;
		return BuffsList(reinterpret_cast<PKT_S_TEST::BuffListItem*>(data), buffsCount);
	}
};


// [ PKT_S_TEST ][BuffsListItem BuffsListItem BuffsListItem][victim victim]
class PKT_S_TEST_WRITE
{
public:
	using BuffsListItem = PKT_S_TEST::BuffListItem;
	using BuffsList = PacketList<PKT_S_TEST::BuffListItem>;
	using BuffsVictimsList = PacketList<uint64>;

	// 고정 부분을 먼저 받아준다.
	PKT_S_TEST_WRITE(uint64 id, uint32 hp, uint16 attack)
	{
		// Send Buffer 를 열어준다.
		// 닫아주는 함수는 CloseAndReturn 를 통해 별도로 진행
		_sendBuffer = GSendBufferManager->Open(4096);
		_bw = BufferWriter(_sendBuffer->Buffer(), _sendBuffer->AllocSize());

		// 고정 부분까지는 채운다. 즉, [ PKT_S_TEST ] 맨 앞 이 부분을 채운 것
		_pkt = _bw.Reserve<PKT_S_TEST>();
		_pkt->packetSize = 0;  // To Fill
		_pkt->packetId = S_TEST;
		_pkt->id = id;
		_pkt->hp = hp;
		_pkt->attack = attack;
		_pkt->buffsOffset = 0; // To Fill
		_pkt->buffsCount = 0;  // To Fill
	}

	// 가변 데이터를 추가하기
	// [BuffsListItem BuffsListItem BuffsListItem] 와 같이
	// 같은 타입의 가변 데이터를 배열 형태로 한꺼번에 추가하기
	// 왜 ? 중간에 다른 타입의 데이터가 들어가길 원하지 않는다
	// 같은 타입이면 배열 처럼 쭉 ~ 이어진 형태가 나오기를 기대한다.
	BuffsList ReserveBuffsList(uint16 buffCount)
	{
		// 가변 데이터의 첫번째 offset 위치 반환
		BuffsListItem* firstBuffsListItem = _bw.Reserve<BuffsListItem>(buffCount);

		// 패킷에서 가변 데이터 부분 정보들 채워주기
		_pkt->buffsOffset = (uint64)firstBuffsListItem - (uint64)_pkt;
		_pkt->buffsCount = buffCount;

		return BuffsList(firstBuffsListItem, buffCount);
	}

	BuffsVictimsList ReserveBuffsVictimsList(BuffsListItem* buffsItem, uint16 victimsCount)
	{
		uint64* firstVictimsListItem = _bw.Reserve<uint64>(victimsCount);

		// Victim 에 대한 정보의 시작 Offset 정보
		buffsItem->victimsOffset = (uint64)firstVictimsListItem - (uint64)_pkt;
		buffsItem->victimsCount = victimsCount;

		return BuffsVictimsList(firstVictimsListItem, victimsCount);
	}

	SendBufferRef CloseAndReturn()
	{
		// 패킷 사이즈 계산
		_pkt->packetSize = _bw.WriteSize();

		_sendBuffer->Close(_bw.WriteSize());
		return _sendBuffer;
	}

private:
	PKT_S_TEST* _pkt = nullptr;
	SendBufferRef _sendBuffer;
	BufferWriter _bw;
};


#pragma pack()
*/