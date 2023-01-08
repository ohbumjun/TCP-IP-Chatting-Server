#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"

void GameSession::OnConnected()
{
	// SessionManager �� �߰� 
	GSessionManager.Add(static_pointer_cast<GameSession>(shared_from_this()));
}

void GameSession::OnDisconnected()
{
	GSessionManager.Remove(static_pointer_cast<GameSession>(shared_from_this()));
}

int32 GameSession::OnRecv(BYTE* buffer, int32 len)
{
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

	return len;
}

void GameSession::OnSend(int32 len)
{
	cout << "OnSend Len = " << len << endl;
}