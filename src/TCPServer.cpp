#include "TCPServer.h" 

char TCPServer::msg[MAXPACKETSIZE];
int TCPServer::num_client;
int TCPServer::last_closed;
bool TCPServer::isonline;
vector<descript_socket*> TCPServer::Message;
vector<descript_socket*> TCPServer::newsockfd;
std::mutex TCPServer::mt;

void* TCPServer::Task(void *arg)
{
	int n;
	struct descript_socket *desc = (struct descript_socket*) arg;
	pthread_detach(pthread_self());

        cerr << "open client[ id:"<< desc->id <<" ip:"<< desc->ip <<" socket:"<< desc->socket<<" send:"<< desc->enable_message_runtime <<" ]" << endl;
	while(1)
	{
		n = recv(desc->socket, msg, MAXPACKETSIZE, 0);    // receive data  use socket from accpet() <0 error =0 client close
		if(n != -1) 
		{
			if(n==0)
			{
			   isonline = false;
			   cerr << "close client[ id:"<< desc->id <<" ip:"<< desc->ip <<" socket:"<< desc->socket<<" ]" << endl;
			   last_closed = desc->id;
			   close(desc->socket);    // close socket  from accept 

			   int id = desc->id;
			   auto new_end = std::remove_if(newsockfd.begin(), newsockfd.end(),    // delete all from begin  to end by 3rd para 
                				           		   [id](descript_socket *device)
		                              				   { return device->id == id; });
			   newsockfd.erase(new_end, newsockfd.end());

			   if(num_client>0) num_client--;
			   break;
			}
			msg[n]=0;
			desc->message = string(msg);
	                std::lock_guard<std::mutex> guard(mt);   // thread lock
			Message.push_back( desc );   
		}
		usleep(600);   
        }
	if(desc != NULL)
		free(desc);
	cerr << "exit thread: " << this_thread::get_id() << endl;
	pthread_exit(NULL);
	
	return 0;
}

int TCPServer::setup(int port, vector<int> opts)
{
	int opt = 1;
	isonline = false;
	last_closed = -1;
	sockfd = socket(AF_INET,SOCK_STREAM,0);    // socket found  AF_INET:IPV4  SOCK_STREAM:tcp stream
 	memset(&serverAddress,0,sizeof(serverAddress));      //   server address set 0 

	for(unsigned int i = 0; i < opts.size(); i++) {
		if( (setsockopt(sockfd, SOL_SOCKET, opts.size(), (char *)&opt, sizeof(opt))) < 0 ) {    // set socket  0:success
			cerr << "Errore setsockopt" << endl; 
      			return -1;
	      	}
	}

	serverAddress.sin_family      = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);  // link any address 
	serverAddress.sin_port        = htons(port);   

	if((bind(sockfd,(struct sockaddr *)&serverAddress, sizeof(serverAddress))) < 0){       // bind socket and server address
		cerr << "Errore bind" << endl;
		return -1;
	}
	
 	if(listen(sockfd,5) < 0){     // listen socket , 2nd pare is the maximum connecting number 
		cerr << "Errore listen" << endl;
		return -1;
	}
	num_client = 0;
	isonline = true;
	return 0;
}

void TCPServer::accepted()
{
	socklen_t sosize    = sizeof(clientAddress);
	descript_socket *so = new descript_socket;
	so->socket          = accept(sockfd,(struct sockaddr*)&clientAddress,&sosize);     // accpet socket queue and new socket found 
	so->id              = num_client;
	so->ip              = inet_ntoa(clientAddress.sin_addr);   // input  address in in_addr type output address in string
	newsockfd.push_back( so );
	cerr << "accept client[ id:" << newsockfd[num_client]->id << 
	                      " ip:" << newsockfd[num_client]->ip << 
		              " handle:" << newsockfd[num_client]->socket << " ]" << endl;
	pthread_create(&serverThread[num_client], NULL, &Task, (void *)newsockfd[num_client]);   //function Task 
	isonline=true;
	num_client++;
}

vector<descript_socket*> TCPServer::getMessage()
{
	std::lock_guard<std::mutex> guard(mt);  //  lock threak 
	return Message;
}

void TCPServer::Send(string msg, int id)
{
	send(newsockfd[id]->socket,msg.c_str(),msg.length(),0);
}

int TCPServer::get_last_closed_sockets()
{
	return last_closed;
}

void TCPServer::clean(int id)
{
	Message[id]->message = "";
	memset(msg, 0, MAXPACKETSIZE);
}

string TCPServer::get_ip_addr(int id)
{
	return newsockfd[id]->ip;
}

bool TCPServer::is_online() 
{
	return isonline;
}

void TCPServer::detach(int id)
{
	close(newsockfd[id]->socket);
	newsockfd[id]->ip = "";
	newsockfd[id]->id = -1;
	newsockfd[id]->message = "";
} 

void TCPServer::closed() 
{
	close(sockfd);
}

