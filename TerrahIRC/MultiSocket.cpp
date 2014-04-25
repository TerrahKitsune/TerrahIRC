#include "MultiSocket.h"


MultiSocket::MultiSocket(SocketCallback SCB){
	memset(this, 0, sizeof(MultiSocket));
	this->EventFunc = SCB;

	PBSize = 10000;
	PacketBuffer = new char[PBSize+1];
}


MultiSocket::~MultiSocket(){

	for (int n = 0; n < this->SocketsLen; n++){

		if (Sockets[n])
			delete Sockets[n];
	}

	free(Sockets);

	if (PacketBuffer)
		delete []PacketBuffer;
}

SOCKET MultiSocket::OpenClientSocket(const char * targetaddr, int port){

	if (this->SocketsLen >= this->SocketSize){

		void * temp = realloc(this->Sockets, sizeof(TerrahSocket*) * (this->SocketSize + 10) );
		if (!temp)
			return -1;
		else
			memset(temp, 0, sizeof(TerrahSocket*)* (this->SocketSize + 10));
		
		if (this->Sockets){

			memcpy(temp, this->Sockets, sizeof(TerrahSocket*)* (this->SocketSize));
			free(this->Sockets);			
		}

		this->Sockets = (TerrahSocket**)temp;
		this->SocketSize += 10;
	}

	TerrahSocket * TS = new TerrahSocket();
	if (!TS)
		return -1;
	
	if (TS->Client(targetaddr, port) != 1){

		delete TS;
		return -1;
	}
	
	this->Sockets[SocketsLen++] = TS;
	return TS->Socket;
}

SOCKET MultiSocket::OpenServerSocket(int port){

	if (this->SocketsLen >= this->SocketSize){

		void * temp = realloc(this->Sockets, sizeof(TerrahSocket*)* (this->SocketSize + 10));
		if (!temp)
			return -1;
		else
			memset(temp, 0, sizeof(TerrahSocket*)* (this->SocketSize + 10));

		if (this->Sockets){

			memcpy(temp, this->Sockets, sizeof(TerrahSocket*)* (this->SocketSize));
			free(this->Sockets);		
		}

		this->Sockets = (TerrahSocket**)temp;
		this->SocketSize += 10;
	}

	TerrahSocket * TS = new TerrahSocket();
	if (!TS)
		return -1;

	if (TS->Server(port)!= 1){

		delete TS;
		return -1;
	}

	this->Sockets[SocketsLen++] = TS;
	return TS->Socket;
}

bool MultiSocket::KillSocket(SOCKET sock){

	TerrahSocket * TC = NULL;
	for (int n = 0; n < this->SocketsLen; n++){

		if (Sockets[n]->Socket == sock){

			TC = Sockets[n];
			Sockets[n] = Sockets[SocketsLen - 1];
			Sockets[SocketsLen - 1] = NULL;
			SocketsLen--;
			break;
		}
		else if (Sockets[n]->isServer){

			for (int i = 0; i < Sockets[n]->SCLength; i++){

				if (Sockets[n]->SC[i]->socket == sock){

					Sockets[n]->SC[i]->die = true;
					return true;
				}
			}
		}
	}

	if (TC){

		delete TC;
		return true;
	}

	return false;
}

int MultiSocket::Send(SOCKET sock, const char * data, int size, SOCKET client){

	if (!Sockets)
		return -1;

	TerrahSocket * TC = NULL;

	if (sock <= 0)
		TC = Sockets[0];
	else{
		for (int n = 0; n < this->SocketsLen; n++){

			if (Sockets[n]->Socket == sock){

				TC = Sockets[n];
				break;
			}
		}
	}
	if (TC){

		if (TC->isServer && ( client == 0 || client == sock ) ){

			int sn = 0;

			for (int n = 0; n < TC->SCLength; n++){

				if (TC->SC[n]->Send(data, size)>0)
					sn++;
			}

			return sn;
		}

		return TC->Send(data, size, client);
	}

	return -1;
}

int MultiSocket::Listen(){

	SOCKET RecvSock;
	int Result;
	int Packets = 0;

	for (int n = 0; n < this->SocketsLen; n++){

		PacketBuffer[0] = 0;
		Result = Sockets[n]->Listen(PacketBuffer, PBSize, &RecvSock);

		//Nothing
		if (Result == 0){
			continue;
		}
		//Data
		else if (Result > 0){
			
			Packets++;

			if (Sockets[n]->isServer){

				if (Sockets[n]->WasAccept(RecvSock) && EventFunc){					
					EventFunc(this, RecvSock, Result, PacketBuffer, strlen(PacketBuffer), 2);
				}
				else if (EventFunc){
					PacketBuffer[Result] = 0;
					EventFunc(this, Sockets[n]->Socket, RecvSock, PacketBuffer, Result, 0);
				}

			}
			else if (EventFunc){
				PacketBuffer[Result] = 0;
				EventFunc(this, Sockets[n]->Socket, RecvSock, PacketBuffer, Result, 0);
			}
		}
		//DC/Error
		else{
			Packets++;

			if (EventFunc)
				EventFunc(this,Sockets[n]->Socket, RecvSock, PacketBuffer, strlen(PacketBuffer), 1);
		
			KillSocket(RecvSock);
			n--;
		}	
	}

	return Packets;
}

const char * MultiSocket::SocketToAddr(SOCKET ServerSock, SOCKET optClientSock){

	for (int n = 0; n < this->SocketsLen; n++){

		if (Sockets[n]->Socket == ServerSock){

			if (Sockets[n]->isServer){

				for (int i = 0; i < Sockets[n]->SCLength; i++){
					
					if (Sockets[n]->SC[i]->socket == optClientSock)
						return Sockets[n]->SC[i]->IP;
				}

			}
			else{
				return Sockets[n]->TargetServer;
			}
			return "";
		}
	}
	return "";
}

int MultiSocket::GetSocketsReady(){

	int nRet = 0;
	for (int n = 0; n < this->SocketsLen; n++){

		if (Sockets[n]->HasMessage())
			nRet++;

	}
	return nRet;
}