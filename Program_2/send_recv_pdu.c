/*#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>*/

#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
  
#include "send_recv_pdu.h"
#include "pollLib.h"

int sendPDU(int clientSocket, uint8_t * dataBuffer, u_int16_t lengthOfData)
{
	u_int16_t bufSendLen= ((uint16_t)(lengthOfData+2));
	u_int16_t bufSendLen_nbo=htons(bufSendLen);
	
	memcpy(&memBuf[0],&bufSendLen_nbo,sizeof(bufSendLen_nbo));
	
	memcpy(&memBuf[2],dataBuffer,lengthOfData);

	int bytesSent = 0;

	//if ((bytesSent = send(clientSocket, memBuf, bufSendLen, 0)) < 0)
	if ((bytesSent = send(clientSocket, memBuf, bufSendLen, 0)) < 0)

	{
        perror("recv call");
       exit(-1);
     }

	 //printf("BYTESSENT:%d\n",bytesSent);fflush(stdout);
	 
    return lengthOfData;
}

int recvPDU(int socketNumber, uint8_t * dataBuffer, int bufferSize)
{
	int bytesReceived=0;
	u_int16_t sizePDU_nbo=0, sizePDU_hbo=0;

	bytesReceived=0;
	bytesReceived=recv(socketNumber, dataBuffer, HEADERSIZE,0);
	if(bytesReceived==-1){
		perror("Recieve failure");
		exit(EXIT_FAILURE);
	}

	//printf("I have recieved :%d bytes\n", num_bytes_rxd);fflush(stdout);
	memcpy(&sizePDU_nbo,dataBuffer,2);

	sizePDU_hbo=ntohs(sizePDU_nbo);
	//printf("size:%d\n", sizePDU_hbo);fflush(stdout);

	if(sizePDU_hbo != 0){

		bytesReceived=0;
		memset(dataBuffer, 0, bufferSize);

		if(sizePDU_hbo>bufferSize)
		{
			printf("Buffer is not large enough to hold the data\n");
			fflush(stdout);
			exit(EXIT_FAILURE);
		}

			
		bytesReceived=0;
/*
		int pollfd_sfd=pollCall(500);
		
		if(pollfd_sfd==socketNumber)
		{*/
			//printf("Going for recieve again\n");fflush(stdout);

			bytesReceived=recv(socketNumber, dataBuffer, bufferSize,0);

			if(bytesReceived<0)
			{
				perror("Recieve failure");
				exit(EXIT_FAILURE);
			}
			/*
			else
			{
				for(int i=0;i<bytesReceived;++i)
				{
					printf("%X ",dataBuffer[i]);fflush(stdout);
				}
				printf("\n");
				printf("2ndBytes recieved:%d\n", bytesReceived);fflush(stdout);
			}*/
		//}
	}

    return bytesReceived;
}


void getMemory(){

	// Allocate memory dynamically
    memBuf = (uint8_t*) malloc(MEMBUFSIZE);

	// Check if memory was allocated successfully
    if (memBuf == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }
}
