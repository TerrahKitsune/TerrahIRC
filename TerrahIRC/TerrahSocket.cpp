#include "TerrahSocket.h"


TerrahSocket::TerrahSocket(){

	memset(this, 0, sizeof(TerrahSocket));
	
	if (WSAStartup(MAKEWORD(2, 2), &this->WSA) != NO_ERROR){
		//Broken
		return;
	}
	
	SocketTimeoutMSec=1;
	SocketTimeoutSec=0;
}

TerrahSocket::~TerrahSocket(){

	if (this->isServer && this->SC){	
		delete[]this->SC;
	}

	if (this->Socket){
		shutdown(this->Socket, 2);
		closesocket(this->Socket);
	}
	WSACleanup();
}

int TerrahSocket::Client(const char * TargetAddr,int Port){

	if (this->Socket){
		shutdown(this->Socket, 2);
		closesocket(this->Socket);
	}

	this->port = Port;

	char Addr[30];
	hostent *h = gethostbyname(TargetAddr);
	if (h != NULL)
		strcpy(Addr, inet_ntoa(*((struct in_addr *) h->h_addr_list[0])));
	else
		strcpy(Addr,TargetAddr);

	sprintf(TargetServer, "%s:%d", Addr, Port);

	isServer = false;
	LastError = 0;

	this->Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->Socket == -1){
		LastError = WSAGetLastError();
		return -1;
	}

	memset(&this->TargetAddr, 0, sizeof(sockaddr_in));

	this->TargetAddr.sin_family = AF_INET;
	this->TargetAddr.sin_port = htons(Port);
	this->TargetAddr.sin_addr.s_addr = inet_addr(Addr);

	return connect(this->Socket, (sockaddr*)&this->TargetAddr, sizeof(sockaddr)) != -1;
}

int TerrahSocket::Server(int Port){

	if (this->Socket){
		shutdown(this->Socket, 2);
		closesocket(this->Socket);
	}

	isServer = true;
	this->port = Port;
	strcpy(this->TargetServer,"*");
	this->Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->Socket == -1){

		LastError = WSAGetLastError();
		return -1;
	}
	memset(&this->TargetAddr, 0, sizeof(sockaddr_in));

	this->TargetAddr.sin_family = AF_INET;
	this->TargetAddr.sin_port = htons(Port);
	this->TargetAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	bind(this->Socket, (sockaddr*)&this->TargetAddr, sizeof(sockaddr));

	listen(this->Socket, 5);

	return 1;
}

bool TerrahSocket::HasMessage(){

	if (LastError != 0)
		return false;

	fd_set fds;
	struct timeval tv;
	FD_ZERO(&fds);
	FD_SET(this->Socket, &fds);

	tv.tv_sec = SocketTimeoutSec;
	tv.tv_usec = SocketTimeoutMSec;

	int opt = select(this->Socket, &fds, NULL, NULL, &tv);
	if (opt == SOCKET_ERROR){

		LastError = WSAGetLastError();
		return false;
	}
	else if (opt == 0 ){

		if (this->isServer){

			for (int n = 0; n < this->SCLength; n++){

				if (this->SC[n]->LastError != 0)
					continue;

				FD_ZERO(&fds);
				FD_SET(this->SC[n]->socket, &fds);
				opt = select(this->SC[n]->socket, &fds, NULL, NULL, &tv);
				if (opt == SOCKET_ERROR){
					
					this->SC[n]->LastError = WSAGetLastError();
					continue;
				}
				else if (opt == 0)
					continue;
				else
					return true;
			}

			return false;
		}
		else
			return false;
	}
	
	return true;
}

int TerrahSocket::Send(const char * buffer, int size, SOCKET s){

	if (!this->isServer)
		s = this->Socket;
	else if (this->LastError!=0){
		return -1;
	}

	int Ret = send(s, buffer, size, 0);
	int sent = Ret;

	while (Ret<size){

		Ret = send(s, &buffer[sent], size - sent, 0);
		sent += Ret;

		if (Ret == SOCKET_ERROR){
			this->LastError = WSAGetLastError();
			return -1;
		}
	}

	return sent;
}

int TerrahSocket::Listen(char * buffer, int max, SOCKET * out){

	if (LastError != 0){
		GetErrorStr(buffer, this->LastError);
		return -1;
	}

	if (this->isServer){

		int nReturn=0;
		time_t current = time(NULL);
		for (int n = 0; n < this->SCLength; n++){

			//DC
			if (SC[n]->die){
				ServerClients * del = this->Remove(n);
				if (del)
					delete del;

				n = 0;
				continue;
			}
			else if (SC[n]->LastError != 0 || (this->Timeout>0 && current > SC[n]->Timeout)){

				if (SC[n]->LastError != 0){
					GetErrorStr(buffer, SC[n]->LastError);
					*out = SC[n]->socket;
				}
				else{
					strcpy(buffer,"Timed out");
					*out = SC[n]->socket;
				}

				SC[n]->die = true;
				
				return -1;
			}

			//Ignore the last socket that recived; proioritze the rest of the queue
			if (SC[n]->ignore){
				SC[n]->ignore = false;
			}
			else{
				SC[n]->SocketTimeoutMSec = this->SocketTimeoutMSec;
				SC[n]->SocketTimeoutSec = this->SocketTimeoutSec;
				nReturn = SC[n]->Listen(buffer, max, out);
			}

			if (nReturn > 0){
				SC[n]->Timeout = current + this->Timeout;
				SC[n]->ignore = true;
				return nReturn;
			}
		}
	}

	fd_set fds;
	struct timeval tv;
	FD_ZERO(&fds);
	FD_SET(this->Socket, &fds);

	tv.tv_sec = SocketTimeoutSec;
	tv.tv_usec = SocketTimeoutMSec;

	int opt = select(this->Socket, &fds, NULL, NULL, &tv);
	if (opt == 0)
		return 0;
	else if (opt==SOCKET_ERROR){
		LastError = WSAGetLastError();
		GetErrorStr(buffer, this->LastError);
		return -1;
	}
	else if(this->isServer){
		
		ServerClients * nSC = new ServerClients;
		if (!nSC)return -1;

		nSC->Timeout = time(NULL) + this->Timeout;
		int size = sizeof(sockaddr);
		nSC->socket = accept(this->Socket, &nSC->Addr, &size );
		if (nSC->socket == INVALID_SOCKET){

			delete nSC;
			return 0;
		}
		else if (this->Add(nSC) == 0){
			delete nSC;
			return 0;		
		}

		
		strcpy(nSC->IP, inet_ntoa(((sockaddr_in*)&nSC->Addr)->sin_addr));
		nSC->port = ntohs(((sockaddr_in*)&nSC->Addr)->sin_port);

		if (buffer)
			strcpy(buffer, nSC->IP);

		if (out)
			*out = this->Socket;

		return nSC->socket;
	}
	//Client
	else{
		if (out)
			*out = this->Socket;

		int recvr = recv(this->Socket, buffer, max, 0);

		if (recvr == 0 && opt > 0){
			LastError = WSAGetLastError();
			return -1;
		}

		return recvr;
	}
}

int TerrahSocket::ServerClients::Listen(char * buffer, int max, SOCKET * out){

	if (this->LastError != 0){
		return -1;
	}

	fd_set fds;
	struct timeval tv;
	FD_ZERO(&fds);
	FD_SET(this->socket, &fds);

	tv.tv_sec = SocketTimeoutSec;
	tv.tv_usec = SocketTimeoutMSec;

	int opt = select(this->socket, &fds, NULL, NULL, &tv);
	if (opt == 0)
		return 0;
	else if (opt == SOCKET_ERROR){
		this->LastError = WSAGetLastError();

		return -1;
	}

	*out = this->socket;

	return recv(this->socket, buffer, max, 0);
}

int TerrahSocket::Add(ServerClients*SC){
	
	if (this->SCLength >= this->SCSize){
		
		void * newHeap = realloc(this->SC, sizeof(ServerClients*)*(SCSize + 10));
		if (!newHeap)return 0;
		this->SC = (ServerClients**)newHeap;
		this->SCSize += 10;
	}

	this->SC[SCLength++] = SC;

	return 1;
}

TerrahSocket::ServerClients * TerrahSocket::Remove(int index){
	
	if (index < 0 || index >= this->SCLength)
		return NULL;

	ServerClients * ret = this->SC[index];
	memcpy(&this->SC[index], &this->SC[index + 1], sizeof(ServerClients*)*(this->SCLength-index-1));
	this->SCLength--;
	return ret;
}

int TerrahSocket::ServerClients::Send(const char * buffer, int max){

	if (max <= 0 || !buffer || this->die || this->LastError >0)
		return 0;

	int n = send(this->socket, buffer, max, 0);
	int sent = n;

	while (n<max){

		n = send(this->socket, &buffer[sent], max - sent, 0);
		sent += n;

		if (n == SOCKET_ERROR){
			this->LastError = WSAGetLastError();
			return 0;
		}
	}
	return 1;
}

void TerrahSocket::GetErrorStr(char * buf, DWORD err){

	LPTSTR Error = 0;
	buf[0] = 0;

	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		err,
		0,
		(LPTSTR)&Error,
		0,
		NULL) == 0)	{
		sprintf(buf, "%u: Unknown Error", err);
	}
	else{
		sprintf(buf, "%u: %s", err, Error);
		LocalFree(Error);
	}
}