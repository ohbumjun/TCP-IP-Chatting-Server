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

// ��Ŷ ���� TEMP
struct BuffData
{
	uint64 buffId;
	float remainTime;
};

struct S_TEST
{
	uint64 id;
	uint32 hp;
	uint16 attack;

	// ���� �����Ͱ� �߰��ȴٰ� �غ���
	// 1) ���ڿ� (ex. name)
	// 2) �׳� ����Ʈ �迭 (ex. ��� �̹���)
	// 3) �Ϲ� ����Ʈ
	vector<BuffData> buffs;
};

void ClientPacketHandler::Handle_S_TEST(BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	PacketHeader header;
	br >> header;

	uint64 id;
	uint32 hp;
	uint16 attack;

	br >> id >> hp >> attack;

	cout << "Id, Hp, Att : " << id << "," << hp << "." << attack << endl;

	
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
	vector<BuffData> buffs;
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
}
