#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "networks.h"
#include "gethostbyname.h"
#include "cpe464.h"
#include "safeUtil.h"


int udpServerSetup(int portNumber)
{
	struct sockaddr_in6 server;
	int socketNum = 0;
	socklen_t serverAddrLen = 0;

	// create the socket
	if ((socketNum = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket() call error");
		exit(-1);
	}

	// set up the socket
	server.sin6_family = AF_INET6;    		// internet (IPv6 or IPv4) family
	server.sin6_addr = in6addr_any ;  		// use any local IP address
	server.sin6_port = htons(portNumber);   // if 0 = os picks

	// bind the name (address) to a port
	if (bind(socketNum,(struct sockaddr *) &server, sizeof(server)) < 0)
	{
		perror("bind() call error");
		exit(-1);
	}

	/* Get the port number */
	serverAddrLen = sizeof(server);
	getsockname(socketNum,(struct sockaddr *) &server,  &serverAddrLen);
	printf("Server using Port #: %d\n", ntohs(server.sin6_port));

	return socketNum;

}

int udpClientSetup(char * hostName, int portNumber, int *sid, struct sockaddr_in6 * server123) {

	//char ipString[INET6_ADDRSTRLEN];
	uint8_t * ipAddress = NULL;

	*sid=0;
	// create the socket
	if ((*sid = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
	{
		perror("udpClientSetup, socket");
		exit(-1);
	}

	if ((ipAddress = gethostbyname6(hostName, server123)) == NULL)
	{
		exit(-1);
	}

	server123->sin6_port = ntohs(portNumber);
	server123->sin6_family = AF_INET6;

//	inet_ntop(AF_INET6, ipAddress, ipString, sizeof(ipString));
//	printf("Server info - IP: %s Port: %d \n", ipString, portNumber);

	return *sid;
}
