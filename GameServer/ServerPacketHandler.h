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

// ���� ������ packet �� ���� �� �����Ƿ� template ���� ������ ���̴�.
template<typename T>
SendBufferRef _MakeSendBuffer(T& pkt, uint16 pktId)
{
	// �������� ũ�� ���� 
	const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
	// ��Ŷ�� ����ִ� ��ü ũ�� (Packet Header �� ���� ũ��, Protocol ID ����)
	const uint16 packetSize = dataSize + sizeof(PacketHeader);

	// �ʿ��� ��ŭ Open �Ѵ�.
	SendBufferRef sendBuffer = GSendBufferManager->Open(packetSize);

	// ���� �ּ� �������� => packet Header ���� �����ϱ� 
	PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
	header->size = packetSize;
	header->id = pktId;

	// Serialize �� ���̴�
	// 1) �������� ���� �ּҸ� �Ѱ���� �Ѵ�. => Packet Header ��ŭ �ǳ� �ٱ�
	//    header[1] ? => header �� 4byte => header[1] �� �� ���� �ּҷ� ���� 4byte �̵�
	ASSERT_CRASH(pkt.SerializeToArray(&header[1], dataSize));

	// ���� �� Close
	sendBuffer->Close(packetSize);

	return sendBuffer;
}

/*��ü ��Ŷ ���� �ڵ�*/
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

// T : ������
template<typename T>
class PacketList
{
public:
	PacketList() : _data(nullptr), _count(0) {}
	// data �� ���� �ּҰ�
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

		// ���� ������ ������ �߰����� ���� �����Ͱ� ���� ����
		// ex) 2���� �迭 ���¶�� �����ϸ� �ȴ�.
		// ex) ���ظ� ���� ? ����� ��� ������ �Ѱ��شٰ� ��������.
		uint16 victimsOffset;
		uint16 victimsCount;
	};

	// packet Header ���� ���Խ�Ű��
	uint16 packetSize;
	uint16 packetId;

	uint64 id;
	uint32 hp;
	uint16 attack;

	// ���� ������ �����ϴ� Header
	// [PKT_S_TEST][BufferData, BufferData, BufferData]
	//			�߿��� BufferData �� �����ϴ� ��ġ
	uint16 buffsOffset;

	// ���������� �� ����
	uint16 buffsCount;

	// �Ʒ��� ���� ���ο� �����͸� �޾Ƶ��� �ʿ� ����
	// 1) ���ڿ� (ex. name)
	// 2) �׳� ����Ʈ �迭 (ex. ��� �̹���)
	// 3) �Ϲ� ����Ʈ
	// vector<BuffListItem> buffs;
	// wstring name;

	// ������ �������� Validata �� �ʿ� X
	// bool Validate();

	// ���� �����͵��� �迭 ����
	using BuffsList = PacketList<PKT_S_TEST::BuffListItem>;

	BuffsList GetBuffsList()
	{
		// ���� �ּ� + offset == ���� ������ �ִ� ���� ��ġ
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

	// ���� �κ��� ���� �޾��ش�.
	PKT_S_TEST_WRITE(uint64 id, uint32 hp, uint16 attack)
	{
		// Send Buffer �� �����ش�.
		// �ݾ��ִ� �Լ��� CloseAndReturn �� ���� ������ ����
		_sendBuffer = GSendBufferManager->Open(4096);
		_bw = BufferWriter(_sendBuffer->Buffer(), _sendBuffer->AllocSize());

		// ���� �κб����� ä���. ��, [ PKT_S_TEST ] �� �� �� �κ��� ä�� ��
		_pkt = _bw.Reserve<PKT_S_TEST>();
		_pkt->packetSize = 0;  // To Fill
		_pkt->packetId = S_TEST;
		_pkt->id = id;
		_pkt->hp = hp;
		_pkt->attack = attack;
		_pkt->buffsOffset = 0; // To Fill
		_pkt->buffsCount = 0;  // To Fill
	}

	// ���� �����͸� �߰��ϱ�
	// [BuffsListItem BuffsListItem BuffsListItem] �� ����
	// ���� Ÿ���� ���� �����͸� �迭 ���·� �Ѳ����� �߰��ϱ�
	// �� ? �߰��� �ٸ� Ÿ���� �����Ͱ� ���� ������ �ʴ´�
	// ���� Ÿ���̸� �迭 ó�� �� ~ �̾��� ���°� �����⸦ ����Ѵ�.
	BuffsList ReserveBuffsList(uint16 buffCount)
	{
		// ���� �������� ù��° offset ��ġ ��ȯ
		BuffsListItem* firstBuffsListItem = _bw.Reserve<BuffsListItem>(buffCount);

		// ��Ŷ���� ���� ������ �κ� ������ ä���ֱ�
		_pkt->buffsOffset = (uint64)firstBuffsListItem - (uint64)_pkt;
		_pkt->buffsCount = buffCount;

		return BuffsList(firstBuffsListItem, buffCount);
	}

	BuffsVictimsList ReserveBuffsVictimsList(BuffsListItem* buffsItem, uint16 victimsCount)
	{
		uint64* firstVictimsListItem = _bw.Reserve<uint64>(victimsCount);

		// Victim �� ���� ������ ���� Offset ����
		buffsItem->victimsOffset = (uint64)firstVictimsListItem - (uint64)_pkt;
		buffsItem->victimsCount = victimsCount;

		return BuffsVictimsList(firstVictimsListItem, victimsCount);
	}

	SendBufferRef CloseAndReturn()
	{
		// ��Ŷ ������ ���
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