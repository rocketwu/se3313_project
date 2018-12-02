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
