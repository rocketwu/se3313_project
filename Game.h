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

//item class
class Item {
public:
	Item() {
		srand(time(NULL));
		id = rand() % 10; //0-9
		price = (rand() % 10 + 1) * 50;   //50-500
		score = (rand() % 100 + 1) * 100; //100-10000
	}
	unsigned int id;    //for frontend to identify the item name
	unsigned int price;
	unsigned int score;

};

//Round class
class Round {
public:
	Round(unsigned int roundNum)
		:roundNum(roundNum), payer(" "), done(false)
	{}
	unsigned int roundNum;
	std::string payer;
	Item item;
	bool done;

};

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
	Event DeathEvent;
	Event ReciveClose;
	Event SendClose;
	unsigned int currentRoundNo = 0;
	unsigned int currentDialogNum = 0;// TODO: set this value when server sending first dialog to client
	unsigned int currentPrice = 0;
	unsigned int score = 0;
	Player(Socket* recSocket, std::string name, Event& event);
	void bindSendSocket(Socket * sendSocket);
	Event inRoom;
	void terminatePlayer();
};

class Room : public Thread {
private:
	
<<<<<<< HEAD
	std::list<Player*> players;
=======
	//std::list<Player*> players;
>>>>>>> 703c85162ef4ae7387609753a59ff01bce9d99ef
	float initTimer;
	float timer;
	unsigned int roundNum;
	std::queue<Round*> logs;
	bool joinAble;
	ThreadSem bidSem;
	ThreadSem saySem;
	Event newPlayerJoin;
	void timeup();
	

public:
<<<<<<< HEAD
=======
	unsigned int playerNum;
	bool running;
	Round* lastRound;
	Player* lastPayer;
>>>>>>> 703c85162ef4ae7387609753a59ff01bce9d99ef
	Player* currentPayer;
	std::queue<Dialog*> dialogs;
	unsigned int rank[3];
	std::string rankName[3];
	Event closeEvent;
	Room(Player* firstPlayer, float initTimer);
	Dialog* getCurrentDialog();
	Round* getCurrentRound();
	Player* getCurrentPayer();
	ThreadSem* getBidSem();
	ThreadSem* getSaySem();
	void resetTimer();
	unsigned int getPlayerNum();
	float getTime();
	// void join(Player* newPlayer);
	// bool leave(Player* thePlayer);
	void join();
	void leave();
	void terminateRoom();

	Event& getJoinEvent();

	virtual long ThreadMain();

};


class ReciveData : public Thread {
public:
	ReciveData(Socket * socketptr, Player& player, Event& closeEvent);
	Socket * socketptr;
	ThreadSem* bidSem;
	ThreadSem* saySem;
	Event closeEvent;
	virtual long ThreadMain();
	void joinRoom(int roomNum);
	void bid(int newPrice);
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
	Event playerDeadEvent;
	Player* thePlayer;
	bool isRunning = true;
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