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



Room::Room(Player* firstPlayer):saySem(1) {
	//the room will create when the first player comes in
	//players.push_back(firstPlayer);
	playerNum=1;
	joinAble = true;
	dialogs.push(new Dialog(0, "", ""));
}

Dialog* Room::getCurrentDialog() {
	return (dialogs.back());
}



ThreadSem* Room::getSaySem() {
	// TODO: sem may be mulfunctional
	return &saySem;
}
unsigned int Room::getPlayerNum() {
	return playerNum;
}
void Room::join() {
	// players.push_back(newPlayer);
	playerNum++;
	//if(!running) {newPlayerJoin.Trigger();}
}
void Room::leave() {
	// TODO: list.remove may mulfunction
	//players.remove(thePlayer);
	playerNum--;
	//return true;
}
void Room::terminateRoom() {
	// TODO: terminate the room
	joinAble = false;
	//closeEvent.Trigger();
}


Player::Player(Socket* recSocket, std::string name, Event& killManage):killManage(killManage)
{ recive = new ReciveData(recSocket, *this, ReciveClose); this->name = name; }

void Player::bindSendSocket(Socket * sendSocket){
	currentDialogNum=0;
	send = new SendData(sendSocket, *this, SendClose, inRoom);
}

void Player::terminatePlayer() {
	//if (!terminate) {
		terminate = true;
		// TODO: event trigger before wait??
		std::cout<<"terminating player "<<name<<std::endl;
		
		SendClose.Wait();
		std::cout<<"deleting send "<<std::endl;
		delete send;

		ReciveClose.Wait();
		std::cout<<"deleting recive "<<std::endl;
		delete recive;

		room->leave();
	//}

}

ReciveData::ReciveData(Socket * socketptr, Player& player, Event& closeEvent) :closeEvent(closeEvent), socketptr(socketptr), player(player) {}

long ReciveData::ThreadMain() {
	std::vector<std::string>* data_v;
	int clientStatus=1;
	while ((!player.terminate)) {
		ByteArray data;
		clientStatus = socketptr->Read(data);
		std::string data_str = data.ToString();
		data_v = dataPhars(data);
		std::string action = (*data_v)[0];
		if (action == "say" || action == "dialog") {
			say((*data_v)[1]);
		}else if(action == "room"){
			int roomNum = std::stoi((*data_v)[1]);
			joinRoom(roomNum);
		// }
		// else if (action == "leave") {
		// 	leave();
		}else if (action == "done"){
			std::cout<<"end rec";
			delete data_v;
			break;
		}
		delete data_v;
		//socketptr->Write(data); //TODO: remove this line
	}
	//player.killManage.Trigger();
	player.terminate=true;
	socketptr->Close();
	delete socketptr;
	std::cout<<"end rec";
	closeEvent.Trigger();
	//player.terminatePlayer();
	

}
void ReciveData::joinRoom(int roomNum) {
	if(roomNum>=PlayerManage::rooms.size()){
		PlayerManage::rooms.push_back(new Room(&player));
		player.room = PlayerManage::rooms[PlayerManage::rooms.size()-1];
	}
	else{
		player.room = PlayerManage::rooms[roomNum];
		player.room->join();
	}	
	saySem = player.room->getSaySem();
	player.inRoom.Trigger();
}
void ReciveData::say(std::string content) {
	saySem->Wait();
	player.currentDialogNum = player.room->getCurrentDialog()->dialogNum;
	player.room->dialogs.push(new Dialog((player.currentDialogNum)+1, content, player.name));

	saySem->Signal();
}

SendData::SendData(Socket * socketptr, Player& player, Event closeEvent, Event inRoom): inRoom(inRoom), closeEvent(closeEvent), socketptr(socketptr), player(player) {}

long SendData::ThreadMain() {
	inRoom.Wait();
	socketptr->Write(ByteArray("Welcome to room"));
	while (!player.terminate) {
		while (!player.room){
			sleep(1);
		}
		//socketptr->Write(ByteArray(std::to_string(player.room->getCurrentRound()->item.score)));
		
		//normal running
		Dialog* d = player.room->getCurrentDialog();		
		if (d->dialogNum > player.currentDialogNum) {
			std::string content = d->content;
			std::string payer = d->sayer;
			std::string msg;
			msg = "dialog|" + content;
			msg=msg+"|"+payer;
			socketptr->Write(ByteArray(msg));
			player.currentDialogNum=d->dialogNum;
		}

	}
	//player.killManage.Trigger();
	socketptr->Close();
	delete socketptr;
	player.killManage.Trigger();
	closeEvent.Trigger();
	//player.terminatePlayer();
}

PlayerManage::PlayerManage(Socket * socketptr) :socketptr(socketptr){}


void PlayerManage::terminate(Event e) {
	terminatePlayer.Trigger();
	isRunning.Wait();
	e.Trigger();
	
}

long PlayerManage::ThreadMain()
{
	ByteArray data;
	socketptr->Read(data);
	std::vector<std::string>* data_v = dataPhars(data);

	hey((*data_v)[1]);
	delete data_v;
	terminatePlayer.Wait();	//anyone who want to terminate the player, need to trigger this event
	std::cout<<"pass wait"<<std::endl;
	thePlayer->terminatePlayer();
	playingPlayer.remove(thePlayer);
	delete thePlayer;
	isRunning.Trigger();
}

void PlayerManage::hey(std::string name) {
	int position = 0;
	sem->Wait();
	for (position = 0; position < shakingNum; position++) {
		if (!handshakingPlayer[position]) {
			thePlayer = handshakingPlayer[position] = new Player(socketptr, name, terminatePlayer);
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