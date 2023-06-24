/******************************************************************************
* myServer.c
* 
* Writen by Prof. Smith, updated Jan 2023
* Use at your own risk.  
*
*****************************************************************************/

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
#include <stdint.h>

#include "networks.h"
#include "safeUtil.h"
#include "send_recv_pdu.h"
#include "pollLib.h"
#include "helper.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1
#define MAX_CLIENTS 10

void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);

void serverControl();
int num_clients=0;
int mainServerSocket = 0;   //socket descriptor for the server socket

int main(int argc, char *argv[])
{

	int clientSocket = 0;   //socket descriptor for the client socket
	int portNumber = 0;
	
	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	mainServerSocket = tcpServerSetup(portNumber);

	serverControl();
	
	/* close the sockets */
	close(clientSocket);
	close(mainServerSocket);

	
	return 0;
}
void addNewSocket(){
	// wait for client to connect
	int clientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);
	addToPollSet(clientSocket);
}

void processClient(int cs)
{
	recvFromClient(cs);
}

void serverControl(){

	//create poll object
	setupPollSet();
	printf("Poll setup has been completed\n");fflush(stdout);

	addToPollSet(mainServerSocket);
	printf("main Server Socket has been added to Poll\n");fflush(stdout);

	int pollfd_sfd;

	while(1){
		
		pollfd_sfd=-1;

		pollfd_sfd=pollCall(5000);

		if(pollfd_sfd==mainServerSocket){

			printf("Activity on main socket\n");fflush(stdout);
			
			if(num_clients<MAX_CLIENTS)
			{
				addNewSocket();
				++num_clients;
			}

		}
		else if(pollfd_sfd == -1)
		{
			//printf("WARNING::activity on wrong fd\n");fflush(stdout);
		}
		else
		{
			printf("Activity on already connected client socket\n");fflush(stdout);
			processClient(pollfd_sfd);
		}
	}

}

void recvFromClient(int clientSocket)
{
	uint8_t dataBuffer[2048];
	int16_t result=-1;
	int messageLen = 0;
	messageLen = recvPDU(clientSocket, dataBuffer, 2048);

	//now get the data from the client_socket
	
	if (messageLen < 0)
	{
		perror("recv call");
		exit(-1);
	}

	if (messageLen > 0)
	{	
		result=checkFlag(dataBuffer,clientSocket,messageLen);
		printf("\n");
		printf("result:%d\n",result);fflush(stdout);
	}
	else
	{
		printf("Connection closed by other side\n");
		--num_clients;
		removeFromPollSet(clientSocket);

		for(int i=0;i<100;i++){
			if(clientinfo_str[i].socketNumber==clientSocket)
			{
				clientinfo_str[i].isUsed=0;
			}
		}
	}
}


int checkArgs(int argc, char *argv[])
{
	getMemory();
	initializeHelper();

	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}