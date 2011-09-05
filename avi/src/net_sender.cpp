#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <cstring>
#include <arpa/inet.h>


#include "net_sender.h"

//Create a net sender to a given port and ip address
net_sender::net_sender(libconfig::Config *conf) {

	conf->lookupValue("iNav.send_ip", c_ipNum);
	conf->lookupValue("iNav.send_port", c_portNum);

	init_sender(c_ipNum, c_portNum);
}
net_sender::~net_sender() {
	close(sockfd);

	printf("iNav sender terminated.\n");
}

//Initialize a UDP sender socket
int net_sender::init_sender(const char* address, const char* port) {
	struct addrinfo hints, *servinfo;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(address, port, &hints, &servinfo)) != 0) {
		printf("getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
							 p->ai_protocol)) == -1) {
			printf("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		printf("talker: failed to bind socket\n");
		return -1;
	}

	freeaddrinfo(servinfo);

	return sockfd;

}

//Send a message over the socket
void net_sender::sendMessage(const char* message, int len) {
	int numbytes;
	if ((numbytes = sendto(sockfd, message, len,
						   0, p->ai_addr, p->ai_addrlen)) == -1) {
		printf("talker: sendto");
		exit(1);
	}
	printf("talker: sent %d bytes to destination\n", numbytes);
	return;

}
