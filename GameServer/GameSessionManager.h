#pragma once

// ���� : Service ������ Session ���� ��� �����ϰ� �ִ�.
// - ������ �̴� ServerCore ���� ��� �����鼭 Send, Recv �� ȣ���ϴ� �� Only
// - ���� �� ���ϰ� Contents �ܿ��� ��� �ְ��� �ϴ� ���̴�.
class GameSession;

using GameSessionRef = shared_ptr<GameSession>;

class GameSessionManager
{
public:
	void Add(GameSessionRef session);
	void Remove(GameSessionRef session);
	void Broadcast(SendBufferRef sendBuffer);

private:
	USE_LOCK;
	// ���������� ������ ����� ���� ������ �� (Ŭ���̾�Ʈ) ��ŭ�� session �� �����ϰ� �� ���̴�.
	Set<GameSessionRef> _sessions;
};

extern GameSessionManager GSessionManager;
