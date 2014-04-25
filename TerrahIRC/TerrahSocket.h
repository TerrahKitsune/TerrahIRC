/***************************************************************************
Written by Robin "Terrah" Karlsson (emailarbra@gmail.com)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
***************************************************************************/

#if !defined(__TERRAHSOCKET__H__)
#define __TERRAHSOCKET__H__

//Startup and bind the winsock lib
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <time.h>
#pragma comment(lib, "Ws2_32.lib")


class TerrahSocket
{
public:

	class ServerClients{
	public:
		ServerClients(){ memset(this, 0, sizeof(ServerClients)); }
		~ServerClients(){
			if (this->socket){
				shutdown(this->socket, 2);
				closesocket(this->socket);
				this->socket = 0;
			}
		}
		SOCKET socket;
		sockaddr Addr;
		time_t Timeout;
		char IP[15];
		int Listen(char * buffer, int max, SOCKET * out);
		int Send(const char * buffer, int max);
		bool ignore;
		bool die;
		long SocketTimeoutMSec;
		long SocketTimeoutSec;
		int port;
		DWORD LastError;
	};

	TerrahSocket();
	~TerrahSocket();

	//Connect to a server as a client
	int Client(const char * TargetAddr, int Port);

	//Serve as a server (client and server should not occur within the same object)
	int Server(int Port);

	//Listens for incomming packets
	//buffer will be filled out with the data from the next packet
	//out will be filled out with the socket that recived the packet (irrelevant for clients)
	//Returns the number of bytes written, -1 or error or 0 if there was no new data
	//If this was an accepted listen call it'll return the socket
	int Listen(char * buffer, int max, SOCKET * out);

	//Send data, in a client SOCKET s isnt used
	//on a server s needs to be the client you're sending to
	int Send(const char * buffer, int size, SOCKET s);

	//Returns true if the client or server sockets has something
	bool HasMessage();

	//Check if the socket is the listen socket
	//Will always be true on clients
	bool WasAccept(SOCKET s){ return s == this->Socket; }

	void GetErrorStr(char * buf, DWORD err);

	SOCKET Socket;
	sockaddr_in TargetAddr;
	time_t Timeout;
	long SocketTimeoutMSec;
	long SocketTimeoutSec;
	DWORD LastError;
	bool isServer;
	char TargetServer[NI_MAXHOST];
	int port;

//protected:
	WSADATA WSA;
	ServerClients ** SC;
	int SCLength;
	int SCSize;
protected:
	int Add(ServerClients*SC);
	ServerClients * Remove(int index);
};

#endif
