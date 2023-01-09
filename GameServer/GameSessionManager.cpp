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

	// 만약 여기서 에러가 난다면 ?
	// 1) WRITE_LOCK 로직에 에러가 있어서 BroadCast 하는 순간, 위의 Remove 함수를 호출해서 꼬인다거나
	//    => std::mutex 를 통해서 해결해보거나...
	// 2) SessionRef 에 대한 ref cnt 가 0 이 되어서 해제가 되거나
	// 3) Send 함수 안에서 RegisterSend 호출 => 연결을 갑자기 끊으면 WSASend 에서 SOCKET_ERROR 가 뜰 것이다.
	//    => 진짜 연결이 끊겨서 Disconnect 가 뜬다.
	//    => Disconnect() 함수를 호출 => GameSession::OnDisconnect 호출 => GameSessionManager 의 Remove 호출
	/*
		  즉,
		  for (GameSessionRef session : _sessions)
		{
			// 아래와 같은 함수를 사실상 추가하는 것일 수도 있다는 것이다.
			_sessions.erase(session)
		}

		해결책 ?
		== session->Send(sendBuffer) 가 호출되는 동안에 GameSessionManager 의 Remove 가 호출되지 않게 하면 된다.

		Session::Disconnect 에서 OnDiscoonect 함수를 바로 실행하는 것이 아니라
		Disconnect 등록을 해주고
		ProcessConnect 에서 할 수 있도록 진행해주면 된다.

		그러면 현재 BroadCast 를 실행중인 쓰레드가 아니라
		다음 턴에 다른 쓰레드가 ProcessConnect() 를 호출하면서 GameSessionManager 의 Remove를 호출하게 될 것이다.
	*/

	WRITE_LOCK;
	for (GameSessionRef session : _sessions)
	{
		// sendBuffer 라는 shared_ptr 만 넘겨주는 형태
		// 즉, 모든 Client 들에게 전송하고자 하는 데이터를 복사하는
		// 불필요한 행위를 안하게 되는 것이다.
		session->Send(sendBuffer);
	}
}