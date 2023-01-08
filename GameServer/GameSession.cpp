#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"

void GameSession::OnConnected()
{
	// SessionManager 에 추가 
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

	// SendBuffer 를 Ref 로 관리하는 이유 ?
	// - Send 함수 호출 => RegisterSend 호출 => WSASend 호출
	// - 이 과정동안에 실질적인 SendBuffer 메모리가 유지되어야 한다.

	// 참고 : 사실 들어오는 데이터의 크기보다 4096은 훨씬 큰 크기의 메모리이다.
	// 이를 조금이나마 아껴보기 위해 SendBuffer Pooling 을 적용해볼 것이다.
	// SendBufferRef sendBuffer = std::make_shared<SendBuffer>(4096);
	// 즉, 하나의 거대한 [              ] 라는 SendBufferChunk 를 만들고
	// 그 안에서 여러개의 SendBuffer 들이 [(    )(   )(     )] 와 같은 식으로 할 것이다.
	SendBufferRef sendBuffer = GSendBufferManager->Open(4096);
	::memcpy(sendBuffer->Buffer(), buffer, len);

	// Echo Server 기능
	// sendBuffer->CopyData(buffer, len);
	sendBuffer->Close(len);

	// 1) 서버 측에서 한명에게만 데이터 전송하기
	// Send(sendBuffer);

	// 2) 서버측에서 BroadCast 를 통해 모든 Client 들에게 전송해주기
	// 참고 : for (int i = 0; i < 5; ++i) => 패킷에 데이터를 뭉쳐서 보내주곤 한다.
	// 예를 들어 원본 데이터 크기가 12 라면, 120이렇게 보낸다는 것.
	for (int i = 0; i < 5; ++i)
		GSessionManager.Broadcast(sendBuffer);

	return len;
}

void GameSession::OnSend(int32 len)
{
	cout << "OnSend Len = " << len << endl;
}