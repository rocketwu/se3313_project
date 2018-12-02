#pragma once
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
		:roundNum(roundNum), payer(""), done(false)
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

class Room : public Thread {
private:
	Player* currentPayer;
	std::list<Player*> players;
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
	std::queue<Dialog*> dialogs;
	unsigned int rank[3];
	std::string rankName[3];
	Event closeEvent;
	Room(Player* firstPlayer, float initTimer = 30);
	Dialog* getCurrentDialog();
	Round* getCurrentRound();
	Player* getCurrentPayer();
	ThreadSem* getBidSem();
	ThreadSem* getSaySem();
	void resetTimer();
	unsigned int getPlayerNum();
	float getTime();
	bool join(Player* newPlayer);
	bool leave(Player* thePlayer);
	void terminateRoom();

	Event& getJoinEvent();

	virtual long ThreadMain();

};

class Player {
public:
	std::string name;
	Room* room;
	ReciveData* recive;
	SendData* send;
	bool terminate = false;
	bool isYou = false;
	Event DeathEvent;
	Event ReciveClose;
	Event SendClose;
	unsigned int currentRoundNo = 0;
	unsigned int currentDialogNum = 5000;// TODO: set this value when server sending first dialog to client
	unsigned int currentPrice = 0;
	unsigned int score = 0;
	Player(Socket* recSocket, std::string name, Event& event);
	void bindSendSocket(Socket * sendSocket);
	void terminatePlayer();
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
	SendData(Socket * socketptr, Player& player, Event closeEvent);
	Socket * socketptr;
	Event closeEvent;
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
	static ThreadSem sem;
	Socket* socketptr;
	Event playerDeadEvent;
	Player* thePlayer;
	bool isRunning = true;
	PlayerManage(Socket * socketptr) :socketptr(socketptr);
	static void init();
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
	PlayerAssist(Socket * ptr) :socketptr(ptr);
	virtual long ThreadMain();
private:
	void thread(std::string thread);
};

#endif //GAME_H