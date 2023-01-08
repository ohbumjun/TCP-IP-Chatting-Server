#include "pch.h"
#include "ServerPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"

void ServerPacketHandler::HandlePacket(BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	PacketHeader header;
	br.Peek(&header);

	switch (header.id)
	{
	default:
		break;
	}
}

SendBufferRef ServerPacketHandler::Make_S_TEST(uint64 id, 
	uint32 hp, uint16 attack, vector<BuffData> buffs, wstring name)
{
	SendBufferRef sendBuffer = GSendBufferManager->Open(4096);

	BufferWriter bw(sendBuffer->Buffer(), sendBuffer->AllocSize());

	PacketHeader* header = bw.Reserve<PacketHeader>();

	// id(uint64), ü��(uint32), ���ݷ�(uint16)
	bw << id << hp << attack;

	// ���� ������
	// - ���� �������� ũ�� ���� �ְ� + �� ���� ������ ���������� �Է�
	struct ListHeader
	{
		uint16 offset;
		uint16 count;
	};

	ListHeader* buffsHeader = bw.Reserve<ListHeader>();

	buffsHeader->offset = bw.WriteSize();
	buffsHeader->count = buffs.size();

	for (BuffData& buff : buffs)
		bw << buff.buffId << buff.remainTime;

	// �����ϴ� �� ������ ũ��
	header->size = bw.WriteSize();
	// �����ϴ� ��Ŷ�� ID
	header->id = S_TEST; // 1 : Test Msg

	sendBuffer->Close(bw.WriteSize());

	return sendBuffer;
}

/*

// Echo
cout << "OnRecv Len Server = " << len << endl;

// SendBuffer �� Ref �� �����ϴ� ���� ?
// - Send �Լ� ȣ�� => RegisterSend ȣ�� => WSASend ȣ��
// - �� �������ȿ� �������� SendBuffer �޸𸮰� �����Ǿ�� �Ѵ�.

// ���� : ��� ������ �������� ũ�⺸�� 4096�� �ξ� ū ũ���� �޸��̴�.
// �̸� �����̳��� �Ʋ����� ���� SendBuffer Pooling �� �����غ� ���̴�.
// SendBufferRef sendBuffer = std::make_shared<SendBuffer>(4096);
// ��, �ϳ��� �Ŵ��� [              ] ��� SendBufferChunk �� �����
// �� �ȿ��� �������� SendBuffer ���� [(    )(   )(     )] �� ���� ������ �� ���̴�.
SendBufferRef sendBuffer = GSendBufferManager->Open(4096);
::memcpy(sendBuffer->Buffer(), buffer, len);

// Echo Server ���
// sendBuffer->CopyData(buffer, len);
sendBuffer->Close(len);

// 1) ���� ������ �Ѹ��Ը� ������ �����ϱ�
// Send(sendBuffer);

// 2) ���������� BroadCast �� ���� ��� Client �鿡�� �������ֱ�
// ���� : for (int i = 0; i < 5; ++i) => ��Ŷ�� �����͸� ���ļ� �����ְ� �Ѵ�.
// ���� ��� ���� ������ ũ�Ⱑ 12 ���, 120�̷��� �����ٴ� ��.
for (int i = 0; i < 5; ++i)
	GSessionManager.Broadcast(sendBuffer);

*/