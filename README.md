TerrahIRC
=========
This is a lua-based console, its original pourpose acting as a lua-scriptable IRC client.

All standard 5.1 lua functions are loaded and included.

Custom Lua:

void print(...) - works with the console
void cls() - clear screen
void exit() - kill application
void close(socket) - close a socket
list-table getall() - return a table containing all sockets
info-table getinfo() - return all info about a socket
socket Client(address,port) - TCP connect to a server
socket Server(port) - Open a listning TCP socket

socket:Send(data, optional clientsocket) - Send data to the socket, if the socket is a server provide a client to send too
socket:AsInt() - Return the socket as an int rather then userdata
socket:Close() - Close the socket

info-table:

Key: Value

IP: IP of the socket if this is a server it'll be *
Port: The port
Type: 0: client 1: server 2: client connected to server
Clients: list-table containing the connected clients, this is nil for non server sockets
Server: If its a client connected to a server, this is the socket it connected too

list-table: the keys are socket numbers and the values are ip:port. Traverse with pairs.

packet-table:

Key: Value

Socket: socket recving the data
From: if server, the client socket that sent the data
Data: the data
Size: size of the data
Flag: 0 ok 1 error 2 server accepted new connection

Event functions/Options:

Send(str) - the send eventfunction registered in the app needs to take a single string, this is the data the user typed.
Recv(table) - recives a packet-table
Tick() - Runs with ever tick interval IE heartbeat
Exit() - Runs when the application closes
Error(string) - If something errors out, this function runs containing the errormessage
