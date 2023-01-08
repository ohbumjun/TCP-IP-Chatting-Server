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
	// 패킷 설계 TEMP 가변 길이 구조체 
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

	// 아래와 같이 내부에 데이터를 받아들일 필요 없이
	// 1) 문자열 (ex. name)
	// 2) 그냥 바이트 배열 (ex. 길드 이미지)
	// 3) 일반 리스트
	// vector<BuffListItem> buffs;
	// wstring name;

	bool Validate()
	{
		uint32 size = 0;

		// PKT_S_TEST 만큼의 Size 추가
		size += sizeof(PKT_S_TEST);

		if (size > packetSize)
			return false;

		// 가변 데이터 크기까지 추가
		size += buffsCount * sizeof(BuffListItem);

		if (packetSize != size)
			return false;

		// offset 범위 정상적으로 들어가있는지 체크
		if (buffsOffset + buffsCount * sizeof(BuffListItem) > packetSize)
			return false;

		return true;
	}

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

#pragma pack()

void ClientPacketHandler::Handle_S_TEST(BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	if (len < sizeof(PKT_S_TEST))
		return;

	/*
	1번째 방법 : 각 원소들을 하나하나 받는 방식
	PacketHeader header;
	br >> header;
	uint64 id;
	uint32 hp;
	uint16 attack;
	br >> id >> hp >> attack;
	cout << "Id, Hp, Att : " << id << "," << hp << "." << attack << endl;
	*/
	
	/*
	2번째 : 직렬화 1st => 임시 객체 만들어서 데이터 받기 
	PKT_S_TEST pkt;
	br >> pkt;
	*/

	/*
	3번째 : 직렬화 2nd => 임시 객체 안만들고 buffer 에서 바로 받아오기 
	*/
	PKT_S_TEST* pkt = reinterpret_cast<PKT_S_TEST*>(buffer);

	if (pkt->Validate() == false)
		return;

	// SeverPacketHandler 에서 보낸 가변 데이터 길이 받아들이기 
	// 아래와 같이 객체를 만들어서 가변 데이터를 받아들이는 것이 아니라,
	// 마찬가지로 포인터 형태로 메모리를 다른 방식으로 바라볼 것이다 (형변환)
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
