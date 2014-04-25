--Endline def is CRLF
ENDL = "\r\n"

--This is an example file for TerrahIRC

--Class KSC:
--void print(str)
--socket client(addr, port)
--socket server(port)
--void cls()
--int send(data, sender, serverrecv)
--void exit()
--bool close(socket)
--table getall()
--
--IE: KSC.server(123); opens port 123 for incoming connections

function Send(str)
	print(str..ENDL);
end

--packet:
--Socket -> socket recving the data
--From -> if server, the client socket that sent the data
--Data -> the data
--Size -> size of the data
--Flag -> 0 ok 1 error 2 server accepted new connection
function Recv(packet)

	print("Socket: " .. tostring(packet.Socket));
	print("From: " .. tostring(packet.From));
	print("Data: " .. tostring(packet.Data));
	print("Size: " .. tostring(packet.Size));
	print("Flag: " .. tostring(packet.Flag));
end

function Tick()


end

function Exit()

end

function Error(str)
	print("Error: " .. str .. ENDL);
end
