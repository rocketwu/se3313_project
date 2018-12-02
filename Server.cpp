#include "thread.h"
#include "socketserver.h"
#include <stdlib.h>
#include <time.h>
#include <list>
#include <vector>
#include <algorithm>
#include <stack>

using namespace Sync;
// This thread handles each client connection
class SocketThread : public Thread
{
private:
    // Reference to our connected socket
    Socket& socket;
    // The data we are receiving
    ByteArray data;
    bool stop=false;
public:
    SocketThread(Socket& socket)
    : socket(socket)
    {}

    SocketThread(Socket& socket, Event& e):socket(socket),e(e){}

    Event e;

    ~SocketThread()
    {}

    Socket& GetSocket()
    {
        return socket;
    }

    virtual long ThreadMain()
    {
        e.Wait();
        std::cout<<"threadpass";
        while(!stop)
        {
            try
            {
                // Wait for data
                int clientStatus=socket.Read(data);
                if(clientStatus<=0) return -1;

                // Perform operations on the data
                std::string data_str = data.ToString();
                std::reverse(data_str.begin(), data_str.end());
                data = ByteArray(data_str);

                // Send it back
                socket.Write(data);
            }
            catch (...)
            {
                // We catch the exception, but there is nothing for us to do with it here. Close the thread.
            }
        }
		
		// Need to gracefully exit

        return 0;
    }

    void terminate(Event e){
        stop=true;
        socket.Close();
        e.Trigger();
    }
};

// This thread handles the server operations
class ServerThread : public Thread
{
private:
    SocketServer& server;
    bool terminate = false;
    std::stack<SocketThread*> socketThreads;
public:
    ServerThread(SocketServer& server)
    : server(server)
    {}

    ~ServerThread()
    {
        // Cleanup

    }

    Event e;

    virtual long ThreadMain()
    {
        int i=0;
        while(!terminate){
            // Wait for a client socket connection
            Socket* newConnection = new Socket(server.Accept());
            //Make sure you CLOSE all socket at the end
            // Pass a reference to this pointer into a new socket thread
            Socket& socketReference = *newConnection;
            socketThreads.push(new SocketThread(socketReference,e));
            if(++i==3){
                e.Trigger();
                std::cout<<"trigger";
            }
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

        std::cout<<"All connection has been closed!"<<std::endl;
        finish.Trigger();
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
            
            std::cout<<"Listening on port# "<<port<<std::endl;
            std::cout.flush();
            // This will wait for input to shutdown the server
            FlexWait cinWaiter(1, stdin);
            cinWaiter.Wait();
            std::cin.get();

            Event finish;
            // Shut down and clean up the server
            serverThread.terminateServer(finish);    
            server.Shutdown();
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
