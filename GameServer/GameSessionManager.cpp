#include "pch.h"
#include "GameSessionManager.h"
#include "GameSession.h"

GameSessionManager GSessionManager;

void GameSessionManager::Add(GameSessionRef session)
{
	WRITE_LOCK;
	_sessions.insert(session);
}

void GameSessionManager::Remove(GameSessionRef session)
{
	WRITE_LOCK;
	_sessions.erase(session);
}

void GameSessionManager::Broadcast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	for (GameSessionRef session : _sessions)
	{
		// sendBuffer ��� shared_ptr �� �Ѱ��ִ� ����
		// ��, ��� Client �鿡�� �����ϰ��� �ϴ� �����͸� �����ϴ�
		// ���ʿ��� ������ ���ϰ� �Ǵ� ���̴�.
		session->Send(sendBuffer);
	}
}