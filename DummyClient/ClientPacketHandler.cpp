#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"

void ClientPacketHandler::HandlePacket(BYTE* buffer, int32 len)
{
	// Helper 클래스 활용하기 
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

// 패킷 설계 TEMP
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

	// 가변 데이터가 추가된다고 해보가
	// 1) 문자열 (ex. name)
	// 2) 그냥 바이트 배열 (ex. 길드 이미지)
	// 3) 일반 리스트
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
	(PacketSession 활용)
	PacketHeader header = *((PacketHeader*)&buffer[0]);
	cout << "Packet Id, Size : " << header.id << "," << header.size << endl;

	char recvBuffer[4096];
	::memcpy(recvBuffer, &buffer[4], header.size - sizeof(PacketHeader));
	
	char recvBuffer[4096];

	// 중간에 끼어넣은 id, hp, attack 고려하여 size 계산
	br.Read(recvBuffer, header.size - sizeof(PacketHeader) - 8 - 4 - 2);

	cout << recvBuffer << endl;
	*/

	/*
	(제일 기본적인 형태)
	this_thread::sleep_for(1s);

	// Server 측에서 에코서버 방식으로 데이터 다시 보내주면
	// 다시 또 보내기
	SendBufferRef sendBuffer = GSendBufferManager->Open(4096);
	::memcpy(sendBuffer->Buffer(), sendData, sizeof(sendData));

	// Echo Server 기능
	// sendBuffer->CopyData(buffer, len);
	sendBuffer->Close(sizeof(sendData));

	Send(sendBuffer);
	*/

	// SeverPacketHandler 에서 보낸 가변 데이터 길이 받아들이기 
	vector<BuffData> buffs;
	uint16 buffCount;
	br >> buffCount;

	buffs.resize(buffCount);
	for (int32 i = 0; i < buffCount; i++)
	{
		// BufferReader 클래스 내에 있는 메모리
		// 로부터 데이터 가져오기 
		br >> buffs[i].buffId >> buffs[i].remainTime;
	}

	cout << "BufCount : " << buffCount << endl;
	
	for (int32 i = 0; i < buffCount; i++)
	{
		cout << "BufInfo : " << buffs[i].buffId << " " << buffs[i].remainTime << endl;
	}
}
