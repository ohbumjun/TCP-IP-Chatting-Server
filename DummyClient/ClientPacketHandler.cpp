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


// 1) 고정된 크기 데이터 몰빵
// 2) 가변 데이터 길이가 있는 영역에 데이터를 바로 넣지 않고
//    가변 데이터를 묘사하는 Header 정보를 넣을 것
// 3) 그 다음에 가변 데이터 참고하기 
// [PKT_S_TEST][BufferData, BufferData, BufferData]

// #pragma pack(1)
// 패킷 송수신 과정에서 동일한 크기로 인식할 수 있게
// byte padding 방지하기 => ClientPacketHandler 에도 동일하게 적용
#pragma pack(1)
struct PKT_S_TEST
{
	// 패킷 설계 TEMP
	struct BuffListItem
	{
		uint64 buffId;
		float remainTime;
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

	bool Validate()
	{
		uint32 size = 0;

		// PKT_S_TEST 만큼의 Size 추가
		size += sizeof(PKT_S_TEST);
		// 가변 데이터 크기까지 추가
		size += buffsCount * sizeof(BuffListItem);

		if (packetSize != size)
			return false;

		// offset 범위 정상적으로 들어가있는지 체크
		if (buffsOffset + buffsCount * sizeof(BuffListItem) > packetSize)
			return false;

		return true;
	}


	// 가변 데이터가 추가된다고 해보가
	// 1) 문자열 (ex. name)
	// 2) 그냥 바이트 배열 (ex. 길드 이미지)
	// 3) 일반 리스트
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
	/*
	(기존 방식)
	vector<BufferLs> buffs;
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
	*/

	vector<PKT_S_TEST::BuffListItem> buffs;
	buffs.resize(pkt.buffsCount);

	for (int32 i = 0; i < pkt.buffsCount; ++i)
	{
		// BufferReader 클래스 내에 있는 메모리
		// 로부터 데이터 가져오기 
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
