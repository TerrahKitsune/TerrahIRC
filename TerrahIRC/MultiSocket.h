#pragma once
#include "TerrahSocket.h"

class MultiSocket;
typedef void(*SocketCallback)(MultiSocket * MS,SOCKET To, SOCKET From,const char * data, int nSize, DWORD Flag);

class MultiSocket
{
public:
	MultiSocket(SocketCallback SCB);
	~MultiSocket();

	SOCKET OpenClientSocket(const char * targetaddr, int port);
	SOCKET OpenServerSocket(int port);
	bool KillSocket(SOCKET sock);
	int Send(SOCKET sock, const char * data, int size, SOCKET client=0);
	int Listen();
	const char * SocketToAddr(SOCKET ServerSock, SOCKET optClientSock);
	int GetOpenSockets(){ return SocketsLen; }
	int GetSocketsReady();

	int SocketsLen;
	int SocketSize;
	TerrahSocket ** Sockets;
	SocketCallback EventFunc;
	char * PacketBuffer;
	int PBSize;
	char LastIP[NI_MAXHOST];

};

