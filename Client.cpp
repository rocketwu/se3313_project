
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

					std::cout << "Please input your data (done to exit): ";
			std::cout.flush();
			std::getline(std::cin, data_str);
			data = ByteArray(data_str);

			// Write to the server
			socket.Write(data);

			// Get the response
			int serverStatus=socket.Read(data);
			std::cout << "Server Response: " << data.ToString() << std::endl;

		
		do{
			// Get the data

			std::cout << "Please input your data (done to exit): ";
			std::cout.flush();
			std::getline(std::cin, data_str);
			data = ByteArray(data_str);

			// Write to the server
			socket.Write(data);

			
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
	Socket socket("127.0.0.1", 9631);
	ClientThread clientThread(socket);
	clientThread.teminateEvent.Wait();
	return 0;
}
