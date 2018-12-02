all: Client Server ProjectServer

Client : Client.o thread.o socket.o Blockable.o
	g++ -o Client Client.o thread.o socket.o Blockable.o -pthread -l rt

Client.o : Client.cpp socket.h
	g++ -c Client.cpp -std=c++11

Server : Server.o thread.o socket.o socketserver.o Blockable.o
	g++ -o Server Server.o thread.o socket.o socketserver.o Blockable.o -pthread -l rt

ProjectServer : ProjectServer.o thread.o socket.o socketserver.o Blockable.o Game.o
	g++ -o ProjectServer ProjectServer.o thread.o socket.o socketserver.o Blockable.o Game.o -pthread -l rt

Game.o: Game.cpp thread.h socket.h Game.h
	g++ -c Game.cpp -std=c++11

Blockable.o : Blockable.h Blockable.cpp
	g++ -c Blockable.cpp -std=c++11

Server.o : Server.cpp thread.h socketserver.h
	g++ -c Server.cpp -std=c++11

ProjectServer.o : ProjectServer.cpp thread.h socketserver.h Game.h
	g++ -c ProjectServer.cpp -std=c++11

thread.o : thread.cpp thread.h
	g++ -c thread.cpp -std=c++11

socket.o : socket.cpp socket.h
	g++ -c socket.cpp -std=c++11

socketserver.o : socketserver.cpp socket.h socketserver.h
	g++ -c socketserver.cpp -std=c++11
