#ifndef __net_listener_h
#define __net_listener_h

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <libconfig.h++>

#define MAXBUFLEN 100

class state;

class net_listener {
public:
	net_listener(libconfig::Config *conf);
	~net_listener();
	

	void listen();
	
	int getNewMessage(char * out);
	void getLastMessage(char * out);


			
private:

	volatile bool m_stoprequested;
	int sockfd;
	struct addrinfo hints, *p;
	struct sockaddr_storage their_addr;
	int rv;
	int numbytes;
	char buf[MAXBUFLEN];
	char message[MAXBUFLEN];
	char new_message_flag;
	
	const char *c_portNum;
			
	struct boostImpl;
	boostImpl* bimpl;
	
	int init_listener(const char* port);
	
	void *get_in_addr(struct sockaddr *sa);


};

#endif