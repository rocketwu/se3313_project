
#include "thread.h"
#include "socket.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>

using namespace Sync;
// This thread handles the connection to the server
class ClientThread : public Thread
{
private:
	// Reference to our connected socket
	Socket& socket;

	// Data to send to server
	ByteArray data;
	std::string data_str;
public:
	Event teminateEvent;
	ClientThread(Socket& socket)
	: socket(socket)
	{
		
	}

	~ClientThread()
	{

	}

	virtual long ThreadMain()
	{
		try{
			int result = socket.Open();
		}
		catch(const std::string msg){
			std::cerr<<msg<<std::endl;
			teminateEvent.Trigger();
			return -1;
		}
		
		do{
			// Get the data
			std::cout << "Please input your data (done to exit): ";
			std::cout.flush();
			std::getline(std::cin, data_str);
			data = ByteArray(data_str);

			// Write to the server
			socket.Write(data);

			// Get the response
			int serverStatus=socket.Read(data);
			
			if (serverStatus>0){
				//use serverStatus to test whether the server alive or not
				std::cout << "Server Response: " << data.ToString() << std::endl;
			}else if (serverStatus==0){
				std::cout<<"Server Close the Connection"<<std::endl;
				break;
			}else{
				std::cout<<"Server error-=-=-=-=-=-="<<std::endl;
				break;
			}
			
		}while(data_str!="done");


		socket.Close();
		teminateEvent.Trigger();
		return 0;
	}
};

int main(void)
{
	// Welcome the user 
	std::cout << "SE3313 Lab 4 Client" << std::endl;

	// Create our socket
	Socket socket("35.162.177.130", 3000);
	ClientThread clientThread(socket);
	clientThread.teminateEvent.Wait();
	return 0;
}
