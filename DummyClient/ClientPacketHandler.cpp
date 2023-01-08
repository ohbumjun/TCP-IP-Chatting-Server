#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"

void ClientPacketHandler::HandlePacket(BYTE* buffer, int32 len)
{
	// Helper Ŭ���� Ȱ���ϱ� 
	BufferReader br(buffer, len);

	PacketHeader header;
	br >> header;

	switch (header.id)
	{
	case S_TEST:
		Handle_S_TEST(buffer, len);
		break;
	}
}


// 1) ������ ũ�� ������ ����
// 2) ���� ������ ���̰� �ִ� ������ �����͸� �ٷ� ���� �ʰ�
//    ���� �����͸� �����ϴ� Header ������ ���� ��
// 3) �� ������ ���� ������ �����ϱ� 
// [PKT_S_TEST][BufferData, BufferData, BufferData]

// #pragma pack(1)
// ��Ŷ �ۼ��� �������� ������ ũ��� �ν��� �� �ְ�
// byte padding �����ϱ� => ClientPacketHandler ���� �����ϰ� ����
#pragma pack(1)
struct PKT_S_TEST
{
	// ��Ŷ ���� TEMP ���� ���� ����ü 
	struct BuffListItem
	{
		uint64 buffId;
		float remainTime;
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

	bool Validate()
	{
		uint32 size = 0;

		// PKT_S_TEST ��ŭ�� Size �߰�
		size += sizeof(PKT_S_TEST);

		if (size > packetSize)
			return false;

		// ���� ������ ũ����� �߰�
		size += buffsCount * sizeof(BuffListItem);

		if (packetSize != size)
			return false;

		// offset ���� ���������� ���ִ��� üũ
		if (buffsOffset + buffsCount * sizeof(BuffListItem) > packetSize)
			return false;

		return true;
	}

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

#pragma pack()

void ClientPacketHandler::Handle_S_TEST(BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	if (len < sizeof(PKT_S_TEST))
		return;

	/*
	1��° ��� : �� ���ҵ��� �ϳ��ϳ� �޴� ���
	PacketHeader header;
	br >> header;
	uint64 id;
	uint32 hp;
	uint16 attack;
	br >> id >> hp >> attack;
	cout << "Id, Hp, Att : " << id << "," << hp << "." << attack << endl;
	*/
	
	/*
	2��° : ����ȭ 1st => �ӽ� ��ü ���� ������ �ޱ� 
	PKT_S_TEST pkt;
	br >> pkt;
	*/

	/*
	3��° : ����ȭ 2nd => �ӽ� ��ü �ȸ���� buffer ���� �ٷ� �޾ƿ��� 
	*/
	PKT_S_TEST* pkt = reinterpret_cast<PKT_S_TEST*>(buffer);

	if (pkt->Validate() == false)
		return;

	// SeverPacketHandler ���� ���� ���� ������ ���� �޾Ƶ��̱� 
	// �Ʒ��� ���� ��ü�� ���� ���� �����͸� �޾Ƶ��̴� ���� �ƴ϶�,
	// ���������� ������ ���·� �޸𸮸� �ٸ� ������� �ٶ� ���̴� (����ȯ)
	// vector<PKT_S_TEST::BuffListItem> buffs;
	// buffs.resize(pkt->buffsCount);
	PKT_S_TEST::BuffsList buffs = pkt->GetBuffsList();
	
	cout << "BufCount : " << pkt->buffsCount << endl;

	for (int32 i = 0; i < pkt->buffsCount; i++)
	{
		cout << "BufInfo : " << buffs[i].buffId << " " << buffs[i].remainTime << endl;
	};

	for (auto it = buffs.begin(); it != buffs.end(); ++it)
	{
		cout << "BufInfo : " << it->buffId << " " << it->remainTime << endl;
	}

	for (auto& buff : buffs)
	{
		cout << "BufInfo : " << buff.buffId << " " << buff.remainTime << endl;
	}

	wstring name;
	uint16 nameLen;
	br >> nameLen;
	name.resize(nameLen);

	br.Read((void*)name.data(), nameLen * sizeof(WCHAR));

	wcout.imbue(std::locale("kor"));
	wcout << name << endl;
};
