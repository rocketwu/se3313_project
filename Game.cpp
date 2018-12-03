#include"Game.h"

const int PlayerManage::shakingNum;
std::list<Player*> PlayerManage::playingPlayer;
Player* PlayerManage::handshakingPlayer[shakingNum];
std::vector<Room*> PlayerManage::rooms;
ThreadSem* PlayerManage::sem;

void SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
	std::string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (std::string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
}

std::vector<std::string>* dataPhars(ByteArray data) {
	std::vector<std::string>* v = new std::vector<std::string>();
	std::string data_str = data.ToString();
	SplitString(data_str, *v, "|");
	return v;
}

void Room::timeup() {
	getCurrentRound()->done = true;
	getCurrentRound()->payer = currentPayer->name;
	resetTimer();
	
	sleep(2);
	logs.push(new Round(++roundNum));
	currentPayer==NULL;
	// if (logs.size() > 5) {
	// 	delete logs.back();
	// 	logs.pop();
	// }
}

Room::Room(Player* firstPlayer, float initTimer) :bidSem(1),saySem(1) {
	//the room will create when the first player comes in
	players.push_back(firstPlayer);
	roundNum = 0;
	joinAble = true;
	this->initTimer = initTimer;
	timer = 0;
	logs.push(new Round(0));
	dialogs.push(new Dialog(0, "", ""));
	rank[0] = 0;
	rank[1] = 0;
	rank[2] = 0;
}

Dialog* Room::getCurrentDialog() {
	return (dialogs.back());
}

Round* Room::getCurrentRound() {
	//player thread will get the round, to get info it needs
	return (logs.back());
}
Player* Room::getCurrentPayer() {
	return currentPayer;
}
ThreadSem* Room::getBidSem() {
	// TODO: sem may be mulfunctional
	return &bidSem;
}
ThreadSem* Room::getSaySem() {
	// TODO: sem may be mulfunctional
	return &saySem;
}
void Room::resetTimer() {
	timer = initTimer;
}
unsigned int Room::getPlayerNum() {
	return players.size();
}
float Room::getTime() {
	return timer;
}
bool Room::join(Player* newPlayer) {
	players.push_back(newPlayer);
	newPlayerJoin.Trigger();
	return true;
}
bool Room::leave(Player* thePlayer) {
	// TODO: list.remove may mulfunction
	players.remove(thePlayer);
	return true;
}
void Room::terminateRoom() {
	// TODO: terminate the room
	joinAble = false;
	newPlayerJoin.Trigger();
	closeEvent.Trigger();
}

Event& Room::getJoinEvent() {
	return newPlayerJoin;
}

long Room::ThreadMain() {
	while (joinAble) {
		while (getPlayerNum() < 2 && joinAble) {
			//wait for more player
			newPlayerJoin.Wait();
			newPlayerJoin.Reset();
		}
		while (true && joinAble) {
			timer -= 0.5;
			if (timer <= 0) break;    //confirmed
			usleep(100000);
		}
		timeup();
	}
}

Player::Player(Socket* recSocket, std::string name, Event& event) :DeathEvent(event) 
{ recive = new ReciveData(recSocket, *this, ReciveClose); this->name = name; }

void Player::bindSendSocket(Socket * sendSocket){
	currentDialogNum=0;
	currentPrice=0;
	currentRoundNo=0;
	send = new SendData(sendSocket, *this, SendClose, inRoom);
}

void Player::terminatePlayer() {
	if (!terminate) {
		terminate = true;
		// TODO: event trigger before wait??
		std::cout<<"terminating player"<<name<<std::endl;
		ReciveClose.Wait();
		std::cout<<"deleting recive"<<std::endl;
		delete recive;
		SendClose.Wait();
		std::cout<<"deleting send"<<std::endl;
		delete send;
		DeathEvent.Trigger();
	}

}

ReciveData::ReciveData(Socket * socketptr, Player& player, Event& closeEvent) :closeEvent(closeEvent), socketptr(socketptr), player(player) {}

long ReciveData::ThreadMain() {
	std::vector<std::string>* data_v;
	while (!player.terminate) {
		ByteArray data;
		int clientStatus = socketptr->Read(data);	//FIXME: unblockable read needed
		// if (clientStatus == 0) {
		// 	std::cout << "client disconnected" << std::endl;
		// 	player.terminatePlayer();
		// }
		std::string data_str = data.ToString();
		data_v = dataPhars(data);
		std::string action = (*data_v)[0];
		if (action == "room") {
			int roomNum = std::stoi((*data_v)[1]);
			joinRoom(roomNum);
		}
		else if (action == "price") {
			int newPrice = std::stoi((*data_v)[1]);
			bid(newPrice);
		}
		else if (action == "say" || action == "dialog") {
			say((*data_v)[1]);
		}
		else if (action == "leave") {
			leave();
		}
		if (action == "done") player.terminatePlayer();
		delete data_v;
		socketptr->Write(data); //TODO: remove this line
	}
	
	socketptr->Close();
	delete socketptr;
	closeEvent.Trigger();
}
void ReciveData::joinRoom(int roomNum) {
	if(roomNum==PlayerManage::rooms.size()){
		PlayerManage::rooms.push_back(new Room(&player,30));
		player.room = PlayerManage::rooms[roomNum];
	}
	else{
		player.room = PlayerManage::rooms[roomNum];
		player.room->join(&player);
	}	
	bidSem = player.room->getBidSem();
	saySem = player.room->getSaySem();
	player.inRoom.Trigger();
}
void ReciveData::bid(int newPrice) {
	bidSem->Wait();
	std::cout<<player.name<<std::endl;
	if (player.room->getCurrentRound()->item.price < newPrice) {
		player.room->getCurrentRound()->item.price = newPrice;
		player.room->getCurrentRound()->payer = player.name;
		player.room->currentPayer = &player;
		player.room->resetTimer();
	}
	//player.currentPrice=player.room->getCurrentRound()->item.price;
	
	bidSem->Signal();
}
void ReciveData::say(std::string content) {
	saySem->Wait();
	player.currentDialogNum = player.room->getCurrentDialog()->dialogNum;
	player.room->dialogs.push(new Dialog(++player.currentDialogNum, content, player.name));

	saySem->Signal();
}
void ReciveData::leave() {
	for (int i = 0; i < 3; i++) {
		if (player.room->rank[i] < player.score) {
			player.room->rank[i] = player.score;
			player.room->rankName[i] = player.name;
			break;
		}
	}
	bidSem = NULL;
	saySem = NULL;
	player.room->leave(&player);
	player.room = NULL;

	//send new rooms info
	int position = 0;
	int room_size = PlayerManage::rooms.size();
	std::string msg_str = "welcome|" + std::to_string(position) + "|" + std::to_string(room_size);
	for (int i = 0; i < room_size; i++) {
		msg_str += "|" + std::to_string(PlayerManage::rooms[i]->getPlayerNum());
	}
	socketptr->Write(ByteArray(msg_str));

}

SendData::SendData(Socket * socketptr, Player& player, Event closeEvent, Event inRoom): inRoom(inRoom), closeEvent(closeEvent), socketptr(socketptr), player(player) {}

long SendData::ThreadMain() {
	inRoom.Wait();
	socketptr->Write(ByteArray("Welcome to room"));
	bool updated=false;
	while (!player.terminate) {
		
		while (player.room->getPlayerNum() < 2) {
			//waiting for players
			player.room->getJoinEvent().Wait();
			
		}
		if(updated)
		//socketptr->Write(ByteArray("updated"));
		updated = false;
		//normal running
		Round* r = player.room->getCurrentRound();
		Dialog* d = player.room->getCurrentDialog();
		if (r->roundNum > player.currentRoundNo) {
			std::string msg = "round|" + std::to_string(r->roundNum) + "|" + r->payer + "|";
			if (player.room->currentPayer== &player) {
				msg += "1";
				player.score += r->item.score;
			}
			else {
				msg += "0";
			}
			usleep(100);
			socketptr->Write(ByteArray(msg));
			msg = "item|" + std::to_string(r->item.id) + "|" + std::to_string(r->item.price) + "|" + std::to_string(r->item.score);
			socketptr->Write(ByteArray(msg));
			player.currentRoundNo = r->roundNum;
			player.currentPrice=r->item.price;
			updated = true;
			
		}
		
		if (d->dialogNum > player.currentDialogNum) {
			std::string content = d->content;
			std::string payer = d->sayer;
			std::string msg;
			if (content.size() == 1) {
				try {
					std::stoi(content);
					msg = "say|" + content;
				}
				catch (...) {
					msg = "dialog|" + content;
				}
			}
			else {
				msg = "dialog|" + content;
			}
			socketptr->Write(ByteArray(msg));
			player.currentDialogNum=d->dialogNum;
			updated = true;
			usleep(100);
		}
		if (player.currentPrice < player.room->getCurrentRound()->item.price) {
			std::string msg = "price|" + std::to_string(player.room->getCurrentRound()->item.price);
			player.currentPrice = player.room->getCurrentRound()->item.price;
			socketptr->Write(ByteArray(msg));
			updated = true;
			usleep(100);
		}
		if (r->roundNum > player.currentRoundNo) {
			std::string msg = "rank";
			std::string p1 = "";
			std::string p2 = "";
			for (int i = 0; i < 3; i++) {
				p1 += "|" + player.room->rankName[i];
				p2 += "|" + player.room->rank[i];
			}
			msg = msg + p1 + p2;
			socketptr->Write(ByteArray(msg));
			updated = true;
			usleep(100);
		}

	}
	player.terminatePlayer();
	socketptr->Close();
	delete socketptr;
	closeEvent.Trigger();
}

PlayerManage::PlayerManage(Socket * socketptr) :socketptr(socketptr){}


void PlayerManage::terminate(Event e) {
	if (!isRunning) {
		e.Trigger();
	}
	else {
		thePlayer->terminatePlayer();
		e.Trigger();
	}
}

long PlayerManage::ThreadMain()
{
	ByteArray data;
	socketptr->Read(data);
	std::vector<std::string>* data_v = dataPhars(data);

	//if ((*data_v)[0]=="hey"){
	//    hey((*data_v)[1]);
	//}
	//else{
	//    thread((*data_v)[1]);
	//}

	hey((*data_v)[1]);
	std::cout<<"f";
	delete data_v;
	playerDeadEvent.Wait();
	playingPlayer.remove(thePlayer);
	delete thePlayer;
	isRunning = false;
}

void PlayerManage::hey(std::string name) {
	int position = 0;
	sem->Wait();
	for (position = 0; position < shakingNum; position++) {
		if (!handshakingPlayer[position]) {
			thePlayer = handshakingPlayer[position] = new Player(socketptr, name, playerDeadEvent);
			break;
		}
	}
	sem->Signal();
	int room_size = rooms.size();
	std::string msg_str = "welcome|" + std::to_string(position) + "|" + std::to_string(room_size);
	for (int i = 0; i < room_size; i++) {
		msg_str += "|" + std::to_string(rooms[i]->getPlayerNum());
	}
	socketptr->Write(ByteArray(msg_str));    //send  welcome|<threadid>|<number of rooms>|<masters of room#1>...
}

PlayerAssist::PlayerAssist(Socket * ptr) :socketptr(ptr) {}

long PlayerAssist::ThreadMain() {
	ByteArray data;
	socketptr->Read(data);
	std::vector<std::string>* data_v = dataPhars(data);
	thread((*data_v)[1]);
	delete data_v;
	isEnd.Trigger();
}

void PlayerAssist::thread(std::string thread) {
	int position = std::stoi(thread);
	PlayerManage::handshakingPlayer[position]->bindSendSocket(socketptr);
	PlayerManage::playingPlayer.push_back(PlayerManage::handshakingPlayer[position]);
	PlayerManage::handshakingPlayer[position] = NULL;
}