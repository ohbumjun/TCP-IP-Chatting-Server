#pragma once

// Packet ID 
enum
{
	// Server���� Client�� ������ ��
	S_TEST = 1
};

// Packet ID �� ���� ���� �ٸ� ó���� �������ֱ� ���� ��
class ClientPacketHandler
{
public:
	static void HandlePacket(BYTE* buffer, int32 len);

	static void Handle_S_TEST(BYTE* buffer, int32 len);
};

