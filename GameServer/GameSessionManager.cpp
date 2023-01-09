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

	// ���� ���⼭ ������ ���ٸ� ?
	// 1) WRITE_LOCK ������ ������ �־ BroadCast �ϴ� ����, ���� Remove �Լ��� ȣ���ؼ� ���δٰų�
	//    => std::mutex �� ���ؼ� �ذ��غ��ų�...
	// 2) SessionRef �� ���� ref cnt �� 0 �� �Ǿ ������ �ǰų�
	// 3) Send �Լ� �ȿ��� RegisterSend ȣ�� => ������ ���ڱ� ������ WSASend ���� SOCKET_ERROR �� �� ���̴�.
	//    => ��¥ ������ ���ܼ� Disconnect �� ���.
	//    => Disconnect() �Լ��� ȣ�� => GameSession::OnDisconnect ȣ�� => GameSessionManager �� Remove ȣ��
	/*
		  ��,
		  for (GameSessionRef session : _sessions)
		{
			// �Ʒ��� ���� �Լ��� ��ǻ� �߰��ϴ� ���� ���� �ִٴ� ���̴�.
			_sessions.erase(session)
		}

		�ذ�å ?
		== session->Send(sendBuffer) �� ȣ��Ǵ� ���ȿ� GameSessionManager �� Remove �� ȣ����� �ʰ� �ϸ� �ȴ�.

		Session::Disconnect ���� OnDiscoonect �Լ��� �ٷ� �����ϴ� ���� �ƴ϶�
		Disconnect ����� ���ְ�
		ProcessConnect ���� �� �� �ֵ��� �������ָ� �ȴ�.

		�׷��� ���� BroadCast �� �������� �����尡 �ƴ϶�
		���� �Ͽ� �ٸ� �����尡 ProcessConnect() �� ȣ���ϸ鼭 GameSessionManager �� Remove�� ȣ���ϰ� �� ���̴�.
	*/

	WRITE_LOCK;
	for (GameSessionRef session : _sessions)
	{
		// sendBuffer ��� shared_ptr �� �Ѱ��ִ� ����
		// ��, ��� Client �鿡�� �����ϰ��� �ϴ� �����͸� �����ϴ�
		// ���ʿ��� ������ ���ϰ� �Ǵ� ���̴�.
		session->Send(sendBuffer);
	}
}