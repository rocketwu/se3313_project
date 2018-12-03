#ifndef GAME_H
#define GAME_H

#include "thread.h"
#include "socketserver.h"
#include <stdlib.h>
#include <time.h>
#include <list>
#include <vector>
#include <algorithm>
#include <stack>
#include <list>
#include <queue>
#include <vector>
using namespace Sync;

class Dialog {
public:
	Dialog(unsigned int dialogNum, std::string content, std::string name) :dialogNum(dialogNum), content(content), sayer(name) {}
	unsigned int dialogNum;
	std::string content;
	std::string sayer;
};

class Room;
class ReciveData;
class SendData;

class Player {
public:
	std::string name;
	Room* room;
	ReciveData* recive;
	SendData* send;
	bool terminate = false;
	//bool isYou = false;
	Event ReciveClose;
	Event SendClose;
	Event killManage;
	unsigned int currentDialogNum = 0;// TODO: set this value when server sending first dialog to client
	Player(Socket* recSocket, std::string name, Event& killManage);
	void bindSendSocket(Socket * sendSocket);
	Event inRoom;
	void terminatePlayer();
};

class Room{
private:
	
	bool joinAble;
	ThreadSem saySem;
	

public:

	unsigned int playerNum;
	std::queue<Dialog*> dialogs;
	//Event closeEvent;
	Room(Player* firstPlayer);
	Dialog* getCurrentDialog();
	ThreadSem* getSaySem();
	unsigned int getPlayerNum();
	void join();
	void leave();
	void terminateRoom();

};


class ReciveData : public Thread {
public:
	ReciveData(Socket * socketptr, Player& player, Event& closeEvent);
	Socket * socketptr;
	ThreadSem* saySem;
	Event closeEvent;
	virtual long ThreadMain();
	void joinRoom(int roomNum);
	void say(std::string content);
	void leave();

private:
	Player& player;
};


class SendData : public Thread {
public:
	SendData(Socket * socketptr, Player& player, Event closeEvent, Event inRoom);
	Socket * socketptr;
	Event closeEvent;
	Event inRoom;
	virtual long ThreadMain();
private:
	Player& player;

};

//player manage class
class PlayerManage : public Thread {
public:
	static const int shakingNum = 50;
	static std::list<Player*> playingPlayer;
	static Player* handshakingPlayer[shakingNum];
	static std::vector<Room*> rooms;
	static ThreadSem* sem;
	static void init() {
		sem = new ThreadSem(1);
	for (int i = 0; i < shakingNum; i++) {
		handshakingPlayer[i] = NULL;
	}
}
	Socket* socketptr;
	Event terminatePlayer;
	Player* thePlayer;
	Event isRunning;
	PlayerManage(Socket * socketptr);
	void terminate(Event e);
	virtual long ThreadMain();
private:
	void hey(std::string name);
	//void thread(std::string thread);
};

class PlayerAssist : public Thread {
public:
	Socket* socketptr;
	Event isEnd;
	PlayerAssist(Socket * ptr);
	virtual long ThreadMain();
private:
	void thread(std::string thread);
};

#endif //GAME_H