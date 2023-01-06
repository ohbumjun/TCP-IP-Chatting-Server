#include "pch.h"
#include "NetAddress.h"

NetAddress::NetAddress(SOCKADDR_IN sockAddr) :
    _sockAddr(sockAddr)
{
}

NetAddress::NetAddress(wstring ip, uint16 port)
{
    ::memset(&_sockAddr, 0, sizeof(_sockAddr));
    _sockAddr.sin_family = AF_INET;
    _sockAddr.sin_addr = ip2Address(ip.c_str());
    _sockAddr.sin_port = ::htons(port);
}

wstring NetAddress::GetIPAddress()
{
    WCHAR buffer[100];
    // len32(buffer) : 딱 100byte 크기 만큼을 세팅해주기 위함
    // sizeof(buffer) != 100 -> 200 이다. 하나의 문자열을 2바이트 단위로 인식하기 때문이다.
    ::InetNtopW(AF_INET, &_sockAddr.sin_addr, buffer, len32(buffer));
    return wstring();
}
// 문자열 ip 주소를 적절한 형태로 변환
IN_ADDR NetAddress::ip2Address(const WCHAR* ip)
{
    IN_ADDR address;
    ::InetPtonW(AF_INET, ip, &address);
    return address;
}
