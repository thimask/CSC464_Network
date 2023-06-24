#ifndef __NETWORKS_H__
#define __NETWORKS_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
//#include "sendrecvpdu.h"

#define FLAG_FILENAME_RCOPY_TO_SERVER 7
#define FLAG_FILENAME_SERVER_TO_RCOPY_OK 8
#define FLAG_EOF 9
#define FLAG_END_CONNECTION 10
#define FLAG_FILENAME_SERVER_TO_RCOPY_NAK 18
#define FLAG_DATA_PACKET 3
#define FLAG_RR 5
#define FLAG_SREJ 6

#define MAX_LEN 1500
#define MAX_RLEN 1800

int udpServerSetup(int portNumber);
int udpClientSetup(char *hostname, int portNumber, int *sid, struct sockaddr_in6 * server123);

#endif
