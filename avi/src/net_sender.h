#ifndef __net_sender_h
#define __net_sender_h

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <libconfig.h++>

#define MAXBUFLEN 100

class net_sender {
public:
	net_sender(libconfig::Config *conf);
	~net_sender();
	
	int init_sender(const char* address, const char* port);
	void sendMessage(const char* message, int len);
	
			

			
private:

	volatile bool m_stoprequested;
	int sockfd;
	struct addrinfo *p;
	struct sockaddr_storage their_addr;
	int rv;
	int numbytes;
	char buf[MAXBUFLEN];
	
	const char *c_ipNum;
	const char *c_portNum;
	
	void *get_in_addr(struct sockaddr *sa);


};

#endif