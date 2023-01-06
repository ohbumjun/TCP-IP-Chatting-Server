#pragma once

// Ŭ���̾�Ʈ���� IP �ּҵ��� �����ϰ� �;��� ���� �ִ�.
// - �Ź� �Լ��� ȣ���ϱ� ����, ��� �ּҸ� �ش� class �� wrapping �ؼ� ����
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

