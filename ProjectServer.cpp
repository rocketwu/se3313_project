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
// This thread handles each client connection



void SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
  std::string::size_type pos1, pos2;
  pos2 = s.find(c);
  pos1 = 0;
  while(std::string::npos != pos2)
  {
    v.push_back(s.substr(pos1, pos2-pos1));
 
    pos1 = pos2 + c.size();
    pos2 = s.find(c, pos1);
  }
  if(pos1 != s.length())
    v.push_back(s.substr(pos1));
}

std::vector<std::string>* dataPhars (ByteArray data){
    std::vector<std::string>* v = new std::vector<std::string>();
    std::string data_str = data.ToString();
    SplitString(data_str,*v,"|");
    return v;
}

class Player;
class ReciveData;
class SendData;

//item class
class Item{
    public:
        Item(){
            srand(time(NULL));
            id = rand()%10; //0-9
            price = (rand()%10+1)*50;   //50-500
            score = (rand()%100+1)*100; //100-10000
        }
        unsigned int id;    //for frontend to identify the item name
        unsigned int price;
        unsigned int score;

};

//Round class
class Round {
    public:
        Round(unsigned int roundNum)
        :roundNum(roundNum),payer(""),done(false)
        {}
        unsigned int roundNum;
        std::string payer;
        Item item;
        bool done;

};

class Dialog {
    public:
        Dialog(unsigned int dialogNum, std::string content, std::string name):dialogNum(dialogNum),content(content),sayer(name){}
        unsigned int dialogNum;
        std::string content;
        std::string sayer;
};

//room class: class used to manage room
class Room : public Thread{
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
        void timeup(){
            getCurrentRound()->done = true;
            resetTimer();
            sleep(2);
            logs.push(new Round(++roundNum));
            if(logs.size()>5){
                delete logs.back();
                logs.pop();
            }
        }
    
    public:
        std::queue<Dialog*> dialogs;
        unsigned int rank[3];
        std::string rankName[3];
		Event closeEvent;
        Room(Player* firstPlayer, float initTimer = 30):bidSem(1){
            //the room will create when the first player comes in
            players.push_back(firstPlayer);
            roundNum = 0;
            joinAble = true;
            initTimer = initTimer;
            timer=0;
            logs.push(new Round(0));
            dialogs.push(new Dialog(0,"",""));
            rank[0]=0;
            rank[1]=0;
            rank[2]=0;
            
        }
        Dialog* getCurrentDialog(){
            return (dialogs.front());
        }
        Round* getCurrentRound(){
            //player thread will get the round, to get info it needs
            return (logs.front());
        }
        Player* getCurrentPayer(){
            return currentPayer;
        }
        ThreadSem* getBidSem(){
            // TODO: sem may be mulfunctional
            return &bidSem;
        }
        ThreadSem* getSaySem(){
            // TODO: sem may be mulfunctional
            return &saySem;
        }
        void resetTimer(){
            timer=initTimer;
        }
        unsigned int getPlayerNum(){
            return players.size();
        }
        float getTime(){
            return timer;
        }
        bool join(Player* newPlayer){
            players.push_back(newPlayer);
            newPlayerJoin.Trigger();
            return true;
        }
        bool leave(Player* thePlayer){
            // TODO: list.remove may mulfunction
            players.remove(thePlayer);
            return true;
        }
        void terminateRoom(){
            // TODO: terminate the room
			joinAble = false;
			newPlayerJoin.Trigger();
			closeEvent.Trigger();
        }

        Event& getJoinEvent(){
            return newPlayerJoin;
        }

        virtual long ThreadMain(){
            while(joinAble){
                while (getPlayerNum()<2&&joinAble){
                    //wait for more player
                    newPlayerJoin.Wait();
                    newPlayerJoin.Reset();
                }
                while (true&&joinAble){
                    timer-=0.5;
                    if (timer<=0) break;    //confirmed
                    usleep(100000);
                }
                timeup();
            }
        }

};

//player class
class Player{
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
        unsigned int currentRoundNo=0;
        unsigned int currentDialogNum=5000;// TODO: set this value when server sending first dialog to client
        unsigned int currentPrice = 0;
        unsigned int score = 0;
        Player(Socket* recSocket, std::string name, Event& event):DeathEvent(event){recive = new ReciveData(recSocket,*this, ReciveClose);this->name=name;}
        void bindSendSocket (Socket * sendSocket){
            send = new SendData(sendSocket, *this, SendClose);
        }
        void terminatePlayer(){
            if(!terminate){
                terminate=true;
                // TODO: event trigger before wait??
                ReciveClose.Wait();
                delete recive;
                SendClose.Wait();
                delete send;
                DeathEvent.Trigger();
            }

        }
};

class ReciveData : public Thread{
    public:
        ReciveData(Socket * socketptr, Player& player, Event& closeEvent):closeEvent(closeEvent),socketptr(socketptr),player(player){}
        Socket * socketptr;
        ThreadSem* bidSem;
        ThreadSem* saySem;
        Event closeEvent;
        virtual long ThreadMain(){
            std::vector<std::string>* data_v;
            while(!player.terminate){
                ByteArray data;
                int clientStatus = socketptr->Read(data);
                if (clientStatus == 0){
                    std::cout<<"client disconnected"<<std::endl;
                    player.terminatePlayer();
                }
                std::string data_str = data.ToString();
                data_v = dataPhars(data);
                std::string action = (*data_v)[0];
                if(action == "room"){
                    int roomNum = std::stoi((*data_v)[1]);
                    joinRoom(roomNum);
                }
                else if (action == "price"){
                    int newPrice = std::stoi((*data_v)[1]);
                    bid(newPrice);
                }
                else if (action == "say" || action == "dialog"){
                    say((*data_v)[1]);
                }
                else if(action == "leave"){
                    leave();
                }
                delete data_v;
            }
            socketptr->Close();
            delete socketptr;
            closeEvent.Trigger();
        }
        void joinRoom(int roomNum){
            player.room=PlayerManage::rooms[roomNum];
            player.room->join(&player);
            bidSem = player.room->getBidSem();
            saySem = player.room->getSaySem();
        }
        void bid(int newPrice){
            bidSem->Wait();
            if(player.room->getCurrentRound()->item.price<newPrice){
                player.room->getCurrentRound()->item.price = newPrice;
                player.room->getCurrentRound()->payer = player.name;
                player.isYou = true;
            }else{
                player.isYou = false;
            }
            //player.currentPrice=player.room->getCurrentRound()->item.price;
            bidSem->Signal();
        }
        void say(std::string content){
            saySem->Wait();
            player.currentDialogNum = player.room->getCurrentDialog()->dialogNum;
            player.room->dialogs.push(new Dialog(++player.currentDialogNum,content, player.name));
            
            saySem->Signal();
        }
        void leave(){
            for(int i =0;i<3;i++){
                if(player.room->rank[i]<player.score){
                    player.room->rank[i]=player.score;
                    player.room->rankName[i]=player.name;
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
            std::string msg_str="welcome|"+std::to_string(position)+"|"+std::to_string(room_size);
            for(int i = 0;i<room_size;i++){
                msg_str+="|" + std::to_string(PlayerManage::rooms[i]->getPlayerNum());
            }
            socketptr->Write(ByteArray(msg_str));

        }

    private:
        Player& player;
};

class SendData : public Thread{
    public:
        SendData(Socket * socketptr, Player& player, Event closeEvent):closeEvent(closeEvent), socketptr(socketptr),player(player){}
        Socket * socketptr;
        Event closeEvent;
        virtual long ThreadMain(){
            while(!player.terminate){
                while(player.room->getPlayerNum()<2){
                    //waiting for players
                    player.room->getJoinEvent().Wait();
                }
                //normal running
                Round* r = player.room->getCurrentRound();
                Dialog* d = player.room->getCurrentDialog();
                if(r->roundNum>player.currentRoundNo){
                    std::string msg = "round|"+ std::to_string(r->roundNum) + "|" + r->payer + "|";
                    if(player.isYou){
                        msg+="1";
                        player.score+=r->item.score;
                    }
                    else{
                        msg+="0";
                    }
                    socketptr->Write(ByteArray(msg));
                    msg = "item|"+std::to_string(r->item.id)+"|"+std::to_string(r->item.price)+"|"+std::to_string(r->item.score);
                    socketptr->Write(ByteArray(msg));
                }
                if(d->dialogNum>player.currentDialogNum){
                    std::string content = d->content;
                    std::string payer = d->sayer;
                    std::string msg;
                    if(content.size()==1){
                        try{
                            std::stoi(content);
                            msg="say|"+content;
                        }                            
                        catch(...){
                            msg="dialog|"+content;
                        }
                    }else{
                        msg="dialog|"+content;
                    }
                    socketptr->Write(ByteArray(msg));
                    
                }
                if(player.currentPrice<player.room->getCurrentRound()->item.price){
                    std::string msg = "price|" + std::to_string(player.room->getCurrentRound()->item.price);
                    player.currentPrice=player.room->getCurrentRound()->item.price;
                    socketptr->Write(ByteArray(msg));
                }
                if (true){
                    std::string msg = "rank";
                    std::string p1="";
                    std::string p2 ="";
                    for(int i=0;i<3;i++){
                        p1+="|"+player.room->rankName[i];
                        p2+="|"+player.room->rank[i];
                    }
                    msg=msg+p1+p2;
                    socketptr->Write(ByteArray(msg));
                }

            }
            player.terminatePlayer();
            socketptr->Close();
            delete socketptr;
            closeEvent.Trigger();
        }
    private:
        Player& player;

};



//player manage class
class PlayerManage : public Thread{
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
        PlayerManage(Socket * socketptr):socketptr(socketptr){}
		static void init() {
			for (int i = 0; i < shakingNum; i++) {
				handshakingPlayer[i] = NULL;
			}
		}
        void terminate(Event e){
            if (!isRunning){
                e.Trigger();
            }else{
                thePlayer->terminatePlayer();
                e.Trigger();
            }
        }
        virtual long ThreadMain()
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

            delete data_v;
            playerDeadEvent.Wait();
            playingPlayer.remove(thePlayer);
            delete thePlayer;
            isRunning = false;
        }
    private:
        void hey(std::string name){
            int position = 0;
            sem.Wait();
            for(position = 0; position<shakingNum; position++){
                if(!handshakingPlayer[position]) {
                    thePlayer = handshakingPlayer[position] = new Player(socketptr,name, playerDeadEvent);
                    break;
                }
            }
            sem.Signal();
            int room_size = rooms.size();
            std::string msg_str="welcome|"+std::to_string(position)+"|"+std::to_string(room_size);
            for(int i = 0;i<room_size;i++){
                msg_str+="|" + std::to_string(rooms[i]->getPlayerNum());
            }
            socketptr->Write(ByteArray(msg_str));    //send  welcome|<threadid>|<number of rooms>|<masters of room#1>...
        }
        //void thread(std::string thread){
        //    int position = std::stoi(thread);
        //    handshakingPlayer[position]->bindSendSocket(socketptr);
        //    playingPlayer.push_back(handshakingPlayer[position]);
        //    handshakingPlayer[position]=NULL;
        //}
};

class PlayerAssist : public Thread {
	public:
		Socket* socketptr;
		Event isEnd;
		PlayerAssist(Socket * ptr) :socketptr(ptr) {}
		virtual long ThreadMain() {
			ByteArray data;
			socketptr->Read(data);
			std::vector<std::string>* data_v = dataPhars(data);
			thread((*data_v)[1]);
			delete data_v;
			isEnd.Trigger();
		}
	private:
		void thread(std::string thread) {
			int position = std::stoi(thread);
			PlayerManage::handshakingPlayer[position]->bindSendSocket(socketptr);
			PlayerManage::playingPlayer.push_back(PlayerManage::handshakingPlayer[position]);
			PlayerManage::handshakingPlayer[position] = NULL;
		}
};

// This thread handles the server operations
class ServerThread : public Thread
{
private:
    SocketServer& server;
    bool terminate = false;
    std::stack<PlayerManage*> socketThreads;
public:
    ServerThread(SocketServer& server)
    : server(server)
    {}

    ~ServerThread()
    {
        // Cleanup

    }

    virtual long ThreadMain()
    {
        while(!terminate){
            // Wait for a client socket connection
            Socket* newConnection = new Socket(server.Accept());
            //Make sure you CLOSE all socket at the end
            // Pass a reference to this pointer into a new socket thread
            socketThreads.push(new PlayerManage(newConnection));
        }        
    }

    void terminateServer(Event finish){
        std::cout<<"+++++start termination+++++"<<std::endl;
        terminate=true;
        Event closeWait;
        while(!socketThreads.empty()){
            
            socketThreads.top()->terminate(closeWait);
            //wait the socketthread close the socket
            
            closeWait.Wait();
            closeWait.Reset();
            
            delete socketThreads.top();
            socketThreads.pop();
        }

		while (!PlayerManage::rooms.empty())
		{
			PlayerManage::rooms.back()->terminateRoom();
			PlayerManage::rooms.back()->closeEvent.Wait();
			delete PlayerManage::rooms.back();
			PlayerManage::rooms.pop_back();
		}

        std::cout<<"All connection has been closed!"<<std::endl;
        finish.Trigger();
    }

};

class AssistThread : public Thread
{
private:
	SocketServer& server;
	bool terminate = false;
	std::stack<PlayerAssist*> socketThreads;
public:
	AssistThread(SocketServer& server)
		: server(server)
	{}

	virtual long ThreadMain()
	{
		while (!terminate) {
			// Wait for a client socket connection
			Socket* newConnection = new Socket(server.Accept());
			//Make sure you CLOSE all socket at the end
			// Pass a reference to this pointer into a new socket thread
			socketThreads.push(new PlayerAssist(newConnection));
		}
	}

	void terminateServer() {
		terminate = true;
		while (!socketThreads.empty()) {
			socketThreads.top()->isEnd.Wait();
			delete socketThreads.top();
			socketThreads.pop();
		}
	}

};


int main(void)
{
    int port = 9631;
    std::cout << "I am a server." << std::endl;
    std::cout << "Press enter to terminate the server...";
    std::cout.flush();
    while (true){
        try{
            std::cout<<"trying to listen port# "<<port<<std::endl;            
            // Create our server
            SocketServer server(port);    

            // Need a thread to perform server operations
            ServerThread serverThread(server);

			SocketServer assist(port + 1);
			AssistThread assistThread(assist);
            
            std::cout<<"Listening on port# "<<port<<std::endl;
            std::cout.flush();
            // This will wait for input to shutdown the server
            FlexWait cinWaiter(1, stdin);
            cinWaiter.Wait();
            std::cin.get();

            Event finish;
            // Shut down and clean up the server
			assistThread.terminateServer();
            serverThread.terminateServer(finish);    
            server.Shutdown();
			assist.Shutdown();
            finish.Wait();      //wait for termination of serverThread finish.
            std::cout << "Exit the Program...";
            break;
            //Cleanup, including exiting clients, when the user presses enter
        }
        catch (const std::string msg){
            std::cout<<msg<<std::endl;
            port++;
        }
    }

}
