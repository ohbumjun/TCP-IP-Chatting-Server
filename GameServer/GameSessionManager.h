#pragma once

// 참고 : Service 에서도 Session 들을 묶어서 관리하고 있다.
// - 하지만 이는 ServerCore 에서 들고 있으면서 Send, Recv 를 호출하는 것 Only
// - 조금 더 편하게 Contents 단에서 들고 있고자 하는 것이다.
class GameSession;

using GameSessionRef = shared_ptr<GameSession>;

class GameSessionManager
{
public:
	void Add(GameSessionRef session);
	void Remove(GameSessionRef session);

	// GameSessionManager 이 들고 있는 모든 Session 에게 동일 데이터 보내주기
	void Broadcast(SendBufferRef sendBuffer);

private:
	USE_LOCK;
	set<GameSessionRef> _sessions;
};

extern GameSessionManager GSessionManager;
