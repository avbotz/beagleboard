#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <cstring>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "net_listener.h"
#include "state.h"

#define SELF_IP "127.0.0.1"

struct net_listener::boostImpl {
		boost::mutex m_mutex;
		boost::thread m_thread;
};


net_listener::net_listener(libconfig::Config *conf) : 
		m_stoprequested(false),
		new_message_flag(0),
		bimpl(new boostImpl)
{
		
	conf->lookupValue("iNav.listen_port", c_portNum);

	sockfd = init_listener(c_portNum);
	if (sockfd != -1) {
		printf("Initializing listener on port %s\n", c_portNum);
		bimpl->m_thread = boost::thread(boost::bind(&net_listener::listen, this));
	}
		
}

net_listener::~net_listener() {
	m_stoprequested = 1;
	
	//Recvfrom call in listener thread won't return until a packet is received
	//so temporarily create a sender and send a packet to self upon termination
	int lo_sock;
	struct addrinfo *lo_p;
	struct addrinfo lo_hints, *lo_servinfo;
	int lo_rv;
	
	memset(&lo_hints, 0, sizeof lo_hints);
	lo_hints.ai_family = AF_UNSPEC;
	lo_hints.ai_socktype = SOCK_DGRAM;
	
	if ((lo_rv = getaddrinfo(SELF_IP, c_portNum, &lo_hints, &lo_servinfo)) != 0) {
		printf("getaddrinfo: %s\n", gai_strerror(lo_rv));
		exit(1);
	}
	
	// loop through all the results and make a socket
	for(lo_p = lo_servinfo; lo_p != NULL; lo_p = lo_p->ai_next) {
		if ((lo_sock = socket(lo_p->ai_family, lo_p->ai_socktype,
							 lo_p->ai_protocol)) == -1) {
			printf("talker: socket");
			continue;
		}
		
		break;
	}
	if (lo_p == NULL) {
		printf("talker: failed to bind socket\n");
		exit(1);
	}
	freeaddrinfo(lo_servinfo);
	
	int numbytes;
	if ((numbytes = sendto(lo_sock, "EXIT", 5,
						   0, lo_p->ai_addr, lo_p->ai_addrlen)) == -1) {
		printf("talker: sendto");
		exit(1);
	}	
	
	//Join with the listener thread and close sockets
	bimpl->m_thread.join();
	close(lo_sock);
	close(sockfd);
	printf("iNav listener terminated.\n");
}

//Initialize a passive UDP listener socket on a given port
int net_listener::init_listener(const char* port) {
	struct addrinfo *servinfo;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, c_portNum, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
							 p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return -1;
	}

	freeaddrinfo(servinfo);
	
	return sockfd;
}

//Listen loop to be called in a separate thread
void net_listener::listen() {
	socklen_t addr_len;

	while (!m_stoprequested) {
//		printf("listener: waiting to recvfrom...\n");

		addr_len = sizeof their_addr;
		if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
								 (struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}

//		printf("listener: got packet\n");
//		inet_ntop(their_addr.ss_family,
//		get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
//		printf("listener: packet is %d bytes long\n", numbytes);
		buf[numbytes] = 0;
//		printf("listener: packet contains \"%s\"\n", buf);

		if (1) {
			boost::mutex::scoped_lock l(bimpl->m_mutex);
			strncpy(message, buf, numbytes+1);
			new_message_flag = 1;
		}
	}
}

//Gets the most recent message if it is new
int net_listener::getNewMessage(char * out) {
	boost::mutex::scoped_lock l(bimpl->m_mutex);
	if (new_message_flag) {
		new_message_flag = 0;
		strcpy(out, message);
		return 1;
	}
	return 0;
}

//Get the last message received
void net_listener::getLastMessage(char * out) {
	boost::mutex::scoped_lock l(bimpl->m_mutex);
	strcpy(out, message);
	return;
}
