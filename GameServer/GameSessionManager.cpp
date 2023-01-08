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
		// sendBuffer 라는 shared_ptr 만 넘겨주는 형태
		// 즉, 모든 Client 들에게 전송하고자 하는 데이터를 복사하는
		// 불필요한 행위를 안하게 되는 것이다.
		session->Send(sendBuffer);
	}
}