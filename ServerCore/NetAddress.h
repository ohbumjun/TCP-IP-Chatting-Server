#pragma once

// 클라이언트들의 IP 주소등을 추출하고 싶어질 때가 있다.
// - 매번 함수를 호출하기 보다, 모든 주소를 해당 class 로 wrapping 해서 관리
class NetAddress
{
public :
	NetAddress(SOCKADDR_IN sockAddr);
	NetAddress(wstring ip, uint16 port);

	SOCKADDR_IN& GetSockAddr() { return _sockAddr; }
	wstring GetIPAddress();
	uint16 GetPort() { return ::ntohs(_sockAddr.sin_port); }

public :
	static IN_ADDR ip2Address(const WCHAR* ip);
private :
	SOCKADDR_IN _sockAddr = {};
};

