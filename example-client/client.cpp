#include <iostream>
#include <signal.h>
#include "TCPClient.h"

TCPClient tcp;

void sig_exit(int s)
{
	tcp.exit();
	exit(0);   // exit program
}

int main(int argc, char *argv[])
{
	if(argc != 4) {
		cerr << "Usage: ./client ip port message" << endl;
		return 0;
	}
	signal(SIGINT, sig_exit);    // catch SIGINT signal then deal in sig_exit()

	tcp.setup(argv[1],atoi(argv[2]));      // //  tranfer stirng to int number  //set   socket 
	while(1)
	{
		tcp.Send(argv[3]);      // send client message 
		string rec = tcp.receive();    //receive server message
		if( rec != "" )
		{
			cout << rec << endl;
		}
		sleep(1);
	}
	return 0;
}
