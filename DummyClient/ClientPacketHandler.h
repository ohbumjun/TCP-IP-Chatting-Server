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

