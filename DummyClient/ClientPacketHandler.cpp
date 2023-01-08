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
	// ��Ŷ ���� TEMP
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

	bool Validate()
	{
		uint32 size = 0;

		// PKT_S_TEST ��ŭ�� Size �߰�
		size += sizeof(PKT_S_TEST);
		// ���� ������ ũ����� �߰�
		size += buffsCount * sizeof(BuffListItem);

		if (packetSize != size)
			return false;

		// offset ���� ���������� ���ִ��� üũ
		if (buffsOffset + buffsCount * sizeof(BuffListItem) > packetSize)
			return false;

		return true;
	}


	// ���� �����Ͱ� �߰��ȴٰ� �غ���
	// 1) ���ڿ� (ex. name)
	// 2) �׳� ����Ʈ �迭 (ex. ��� �̹���)
	// 3) �Ϲ� ����Ʈ
	// vector<BuffListItem> buffs;
	// wstring name;
};

#pragma pack()

void ClientPacketHandler::Handle_S_TEST(BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	if (len < sizeof(PKT_S_TEST))
		return;

	// PacketHeader header;
	// br >> header;
	// uint64 id;
	// uint32 hp;
	// uint16 attack;
	// br >> id >> hp >> attack;
	// cout << "Id, Hp, Att : " << id << "," << hp << "." << attack << endl;
	PKT_S_TEST pkt;
	br >> pkt;

	if (pkt.Validate() == false)
		return;

	/*
	(PacketSession Ȱ��)
	PacketHeader header = *((PacketHeader*)&buffer[0]);
	cout << "Packet Id, Size : " << header.id << "," << header.size << endl;

	char recvBuffer[4096];
	::memcpy(recvBuffer, &buffer[4], header.size - sizeof(PacketHeader));

	char recvBuffer[4096];

	// �߰��� ������� id, hp, attack ����Ͽ� size ���
	br.Read(recvBuffer, header.size - sizeof(PacketHeader) - 8 - 4 - 2);

	cout << recvBuffer << endl;
	*/

	/*
	(���� �⺻���� ����)
	this_thread::sleep_for(1s);

	// Server ������ ���ڼ��� ������� ������ �ٽ� �����ָ�
	// �ٽ� �� ������
	SendBufferRef sendBuffer = GSendBufferManager->Open(4096);
	::memcpy(sendBuffer->Buffer(), sendData, sizeof(sendData));

	// Echo Server ���
	// sendBuffer->CopyData(buffer, len);
	sendBuffer->Close(sizeof(sendData));

	Send(sendBuffer);
	*/

	// SeverPacketHandler ���� ���� ���� ������ ���� �޾Ƶ��̱� 
	/*
	(���� ���)
	vector<BufferLs> buffs;
	uint16 buffCount;
	br >> buffCount;

	buffs.resize(buffCount);
	for (int32 i = 0; i < buffCount; i++)
	{
		// BufferReader Ŭ���� ���� �ִ� �޸�
		// �κ��� ������ �������� 
		br >> buffs[i].buffId >> buffs[i].remainTime;
	}

	cout << "BufCount : " << buffCount << endl;

	for (int32 i = 0; i < buffCount; i++)
	{
		cout << "BufInfo : " << buffs[i].buffId << " " << buffs[i].remainTime << endl;
	}
	*/

	vector<PKT_S_TEST::BuffListItem> buffs;
	buffs.resize(pkt.buffsCount);

	for (int32 i = 0; i < pkt.buffsCount; ++i)
	{
		// BufferReader Ŭ���� ���� �ִ� �޸�
		// �κ��� ������ �������� 
		br >> buffs[i];
	}

	cout << "BufCount : " << pkt.buffsCount << endl;

	for (int32 i = 0; i < pkt.buffsCount; i++)
	{
		cout << "BufInfo : " << buffs[i].buffId << " " << buffs[i].remainTime << endl;
	};

	wstring name;
	uint16 nameLen;
	br >> nameLen;
	name.resize(nameLen);

	br.Read((void*)name.data(), nameLen * sizeof(WCHAR));

	wcout.imbue(std::locale("kor"));
	wcout << name << endl;
};
