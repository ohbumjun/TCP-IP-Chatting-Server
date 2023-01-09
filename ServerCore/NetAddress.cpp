#include "pch.h"
#include "NetAddress.h"

/*--------------
	NetAddress
---------------*/

NetAddress::NetAddress(SOCKADDR_IN sockAddr) : _sockAddr(sockAddr)
{
}

NetAddress::NetAddress(wstring ip, uint16 port)
{
	::memset(&_sockAddr, 0, sizeof(_sockAddr));
	_sockAddr.sin_family = AF_INET;
	_sockAddr.sin_addr = Ip2Address(ip.c_str());
	_sockAddr.sin_port = ::htons(port);
}

wstring NetAddress::GetIpAddress()
{
	WCHAR buffer[100];
	// len32(buffer) : �� 100byte ũ�� ��ŭ�� �������ֱ� ����
	// sizeof(buffer) != 100 -> 200 �̴�. �ϳ��� ���ڿ��� 2����Ʈ ������ �ν��ϱ� �����̴�.
	::InetNtopW(AF_INET, &_sockAddr.sin_addr, buffer, len32(buffer));
	return wstring(buffer);
}

// ���ڿ� ip �ּҸ� ������ ���·� ��ȯ
IN_ADDR NetAddress::Ip2Address(const WCHAR* ip)
{
	IN_ADDR address;
	::InetPtonW(AF_INET, ip, &address);
	return address;
}
