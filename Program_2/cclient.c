/******************************************************************************
* myClient.c
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
#define DEBUG_FLAG 0


void sendToServer(int socketNum);
int readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);
//void getMemory();
void sendInitialPacket(); 

void clientControl();
int socketNum = 0;         //socket descriptor
char clienthandle[100];
uint8_t handleLength=0;

char unicast1[10];
char unicast2[10];

char exitcmd1[10];
char exitcmd2[10];

char listcmd1[10];
char listcmd2[10];

char bcastcmd1[10];
char bcastcmd2[10];

char mcastcmd1[10];
char mcastcmd2[10];

int main(int argc, char * argv[])
{	
	getMemory();

	checkArgs(argc, argv);
	printf("Client Handle Name: %s handleLength: %d\n", clienthandle, handleLength);fflush(stdout);

	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);
	
	clientControl();
	
	close(socketNum);
	
	return 0;
}

void sendInitialPacket()
{
	uint8_t sendBuf[MAXBUF];   //data buffer
	int sendLen = 0;        //amount of data to send
	//int sent = 0;            //actual amount of data sent/* get the data and send it   */
	uint8_t FLAG=1;

	memcpy(&sendBuf[0], &FLAG,sizeof(uint8_t));
	memcpy(&sendBuf[1], &handleLength,sizeof(uint8_t));
	memcpy(&sendBuf[2], &clienthandle,handleLength);

	sendLen=2+handleLength;
	int rt=sendPDU(socketNum, sendBuf, sendLen);
/*
	for(int i=2;i<rt+2;i++){
		printf("%c ",sendBuf[i]);
	}
	printf("\n");
	*/
	printf("Bytes send from:%d\n", rt);fflush(stdout);	
}

void processMsgFromServer()
{
	
	uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;
	messageLen = recvPDU(socketNum, dataBuffer, MAXBUF);
	int16_t result;
	//now get the data from the client_socket
	
	if (messageLen < 0)
	{
		perror("recv call");
		exit(-1);
	}

	if (messageLen > 0)
	{
		result=checkFlag(dataBuffer,socketNum,messageLen);

		if(result==-1){
			printf("Handle already in use:%s\n",clienthandle);fflush(stdout);
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		printf("Connection closed by other side\n");
	}
}

void processStdin()
{
	sendToServer(socketNum);
}

void clientControl(){

	//create poll object
	setupPollSet();
	printf("Poll setup has been completed\n");fflush(stdout);

	addToPollSet(socketNum);
	printf("Client Socket has been added to Poll\n");fflush(stdout);

	addToPollSet(STDIN_FILENO);
	printf("STDIN_FILENO has been added to Poll\n");fflush(stdout);

	sendInitialPacket();
	int pollfd_sfd;
	//printf("$: ");fflush(stdout);	
	int8_t isnumbervisible=1;
	while(1)
	{
		if(isnumbervisible==1){
			printf("$: ");fflush(stdout);
		}

		pollfd_sfd=-1;

		pollfd_sfd=pollCall(5000);

		if(pollfd_sfd==socketNum){

			//printf("Activity on client/server socket\n");fflush(stdout);
			
			processMsgFromServer();
			isnumbervisible=1;
		}
		else if(pollfd_sfd == STDIN_FILENO)
		{
			processStdin();
			isnumbervisible=1;
		}
		else
		{
			isnumbervisible=0;
			//printf("Activity on already connected client socket\n");fflush(stdout);
			//processClient(pollfd_sfd);
		}
	}
}

void sendToServer(int socketNum)
{
	uint8_t sendBuf[MAXBUF];   //data buffer
	int mlen;
	uint8_t tempmsgbuf[MAXBUF];

	memset(tempmsgbuf,0,sizeof(tempmsgbuf));

	uint8_t sendBuf1[MAXBUF];   //data buffer
	int sendLen = 0;        //amount of data to send
	//int sent = 0;            //actual amount of data sent/* get the data and send it   */
	uint8_t flag;	
	
	sendLen = readFromStdin(sendBuf);

	int rt;

	memcpy(sendBuf1,sendBuf,sizeof(sendBuf));

	//printf("read: %s string len: %d (including null)\n", sendBuf, sendLen);
	
	if((memcmp(sendBuf,unicast1,2)==0) || (memcmp(sendBuf,unicast2,2)==0))
	{
		//printf("Its a unicast chat message");

		uint8_t destHandleLenght1=0;

		flag=5;

		uint8_t unicastsendBuf[2048];   //data buffer

		memset(unicastsendBuf,0,sizeof(unicastsendBuf));

		memcpy(&unicastsendBuf[0],&flag,1);

		memcpy(&unicastsendBuf[1],&handleLength,1);

		memcpy(&unicastsendBuf[2],clienthandle,handleLength);

		uint8_t one=1;

		int idx=2+handleLength;
		//printf("\nidx:%d\n",idx);fflush(stdout);

		memcpy(&unicastsendBuf[idx],&one,1);
		
		//printf("unicastsendBuf size: %ld",strlen((char*)unicastsendBuf));



		int i=0;
		char tb1[MAXBUF];
		int containsCharacters=-1;
		uint8_t tbuf[MAXBUF];
		//uint8_t tbuf1[100];
		char * token = strtok((char *) sendBuf, " ");
		while( token != NULL ) 
		{
			if(i==1)
			{

				memcpy(tbuf,token,sizeof(tbuf));
			//	printf("tbuf:%s\n",tbuf);fflush(stdout);

				int abc=atoi((char*)tbuf);
				
				sprintf(tb1,"%d", abc);
				
				int a=strlen(tb1);
				int b=strlen((char *)tbuf);

				//printf("a:%d b:%d\n",a,b);fflush(stdout);
				
				if(a==b){
					containsCharacters=0;
				}else{
					containsCharacters=1;
				}
/*
				if(containsCharacters==0)
				{
					printf("ITS ONLY number\n");fflush(stdout);
				}
				else
				{
					printf("It contains ascii values also\n");fflush(stdout);
				}*/
			}


		//	printf( " %s\n", token );
			i++;
			token = strtok(NULL, " ");
		}

		int dobreak=0;
		while(dobreak!=1)
		{
			if((containsCharacters==0)||(containsCharacters==1))
			{
				
//				memcpy(&tbuf1[0],tbuf,destHandleLenght-1);
				destHandleLenght1=sizeof((char *)tbuf);
				//--destHandleLenght1;

				memcpy(&unicastsendBuf[2+handleLength+1],&destHandleLenght1,sizeof(uint8_t));
				memcpy(&unicastsendBuf[2+handleLength+2],tbuf,destHandleLenght1);

				int copyindex=2+1+destHandleLenght1;

				int messagelen=strlen((char*)sendBuf1)-3-destHandleLenght1;

				//printf("dest handle:%s len:%d\n",tbuf,destHandleLenght1);fflush(stdout);
				//printf("copyindex:%d messagelen:%d sendbuf:%d\n",copyindex,messagelen,strlen((char *)sendBuf1));fflush(stdout);
				
				memcpy(tempmsgbuf,&sendBuf1[copyindex],messagelen);

				mlen=strlen((char *)tempmsgbuf);

				//printf("Message: %s mlen:%d \n ",tempmsgbuf,mlen);fflush(stdout);

				idx=0;

				idx=2+handleLength+2+destHandleLenght1;

				//printf("new idx:%d\n",idx);fflush(stdout);
				
				memcpy(&unicastsendBuf[idx],&tempmsgbuf[0],mlen);

			}

			int sentBytes=mlen+idx;
			
			/*
			printf("sendBuffer\n");
			for(int i=0;i<sentBytes;i++){
				printf("%x ",unicastsendBuf[i]);fflush(stdout);
			}

			printf("\n");

			printf("sb:%d",sentBytes);
			*/
			int rt=sendPDU(socketNum, unicastsendBuf, sentBytes);

			if(DEBUG_FLAG){
				printf("rt:%d\n",rt);
			}
			//printf("Bytes send from:%d\n", rt);fflush(stdout);
			dobreak=1;
		}
	}
	else if((memcmp(sendBuf,exitcmd1,2)==0) || (memcmp(sendBuf,exitcmd2,2)==0))
	{
		uint8_t sb[500];
		uint8_t cf=8;
		memcpy(sb, &cf,1);
		int rt=sendPDU(socketNum, sb, 1);
		//printf("socketNumber:%d\n",rt);
		if(DEBUG_FLAG){
			printf("rt:%d\n",rt);
		}
		removeFromPollSet(STDIN_FILENO);
		printf("User can't enter any input\n");fflush(stdout);
	}
	else if((memcmp(sendBuf,mcastcmd1,2)==0) || (memcmp(sendBuf,mcastcmd2,2)==0))
	{
		printf("Its a multicast chat message");

		//uint8_t destHandleLenght1=0;

		flag=6;

		uint8_t mcastsendBuf[2048];   //data buffer

		memset(mcastsendBuf,0,sizeof(mcastsendBuf));

		memcpy(&mcastsendBuf[0],&flag,1);
		
		memcpy(&mcastsendBuf[1],&handleLength,1);

		memcpy(&mcastsendBuf[2],clienthandle,handleLength);


		int itr=0;
		uint8_t numberhandles=0;
		uint8_t handleused=0;
		int handlecount=0;
		//int oddcheck=0;

		//int sizeh=0;

		uint8_t sizeh8=0;
		int destindex=2+handleLength;
		int copyindex=2;

		char * token = strtok((char *) sendBuf, " ");
		while( token != NULL ) 
		{
			char tbuf[MAXBUF];
			memset(tbuf,0,sizeof(tbuf));
			if(itr == 1)
			{
				memcpy(tbuf,token,sizeof(tbuf));
				printf("tbuf:%s len:%ld\n",tbuf,strlen((char*)tbuf));fflush(stdout);
				copyindex=copyindex+1+strlen((char*)tbuf);
				handlecount=atoi((char*)tbuf);	
				numberhandles=((uint8_t)handlecount);				
				memcpy(&mcastsendBuf[destindex],&numberhandles,1);
				++destindex;
			}
			printf( " %s\n", token );
			if(( itr > 1 ) && ( handleused < handlecount ))
			{
				memcpy(tbuf,token,sizeof(tbuf));
				uint8_t hll=strlen(tbuf);
				memcpy(&mcastsendBuf[destindex],&hll,1);
				destindex++;

				memcpy(&mcastsendBuf[destindex],tbuf,hll);
				printf("tbuf:%s len:%ld\n",tbuf,strlen((char*)tbuf));fflush(stdout);
				copyindex=copyindex+1+strlen((char*)tbuf);
				handleused++;
				destindex=destindex+hll;
			}
			itr++;
			token = strtok(NULL, " ");
		}

		copyindex++;
		int mleen=strlen((char*)sendBuf1)-copyindex;
		memcpy(tempmsgbuf,&sendBuf1[copyindex],mleen);
		printf("tempmsgbuf:%s\n",tempmsgbuf);fflush(stdout);
		destindex=destindex+sizeh8;
		memcpy(&mcastsendBuf[destindex],tempmsgbuf,mleen);

		destindex=destindex+mleen;
		int rt=sendPDU(socketNum, mcastsendBuf, destindex);
		if(!DEBUG_FLAG){
			printf("rt:%d\n",rt);
		}
	}
	else if((memcmp(sendBuf,listcmd1,2)==0) || (memcmp(sendBuf,listcmd2,2)==0))
	{
		uint8_t sb[500];
		uint8_t cf=10;
		memcpy(sb, &cf,1);
		int rt=sendPDU(socketNum, sb, 1);
		if(DEBUG_FLAG){
			printf("rtsdf:%d\n",rt);
		}
		//printf("socketNumber:%d\n",rt);
		//removeFromPollSet(STDIN_FILENO);
		//printf("User can't enter any input\n");fflush(stdout);
	}
	else if((memcmp(sendBuf,bcastcmd1,2)==0) || (memcmp(sendBuf,bcastcmd2,2)==0))
	{
		//printf("Its a broadcast chat message");

		int bflag=4;

		uint8_t broadcastsendBuf[2048];   //data buffer

		memset(broadcastsendBuf,0,sizeof(broadcastsendBuf));

		memcpy(&broadcastsendBuf[0],&bflag,1);

		memcpy(&broadcastsendBuf[1],&handleLength,1);

		memcpy(&broadcastsendBuf[2],clienthandle,handleLength);

		int messageidx=strlen((char*)sendBuf);
		int mlen=messageidx-3;

		memset(tempmsgbuf,0,sizeof(tempmsgbuf));
		memcpy(tempmsgbuf,&sendBuf1[3],mlen);

		//printf("Broadcast Message:%s\n",tempmsgbuf);

		memcpy(&broadcastsendBuf[2+handleLength],tempmsgbuf,mlen);		
		int sentlex=2+handleLength+mlen;
		rt=sendPDU(socketNum, broadcastsendBuf, sentlex);

		if(DEBUG_FLAG){
			printf("rt:%d\n",rt);
		}

	}
	else
	{
		rt=sendPDU(socketNum, sendBuf, sendLen);
		if(!DEBUG_FLAG){
			printf("rt:%d\n",rt);
		}
	}
}

int readFromStdin(uint8_t * buffer)
{
	char aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	//printf("Enter data: ");
	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s ClientHandle_name host-name port-number \n", argv[0]);
		exit(1);
	}
	strcpy(clienthandle,argv[1]);
	handleLength=strlen(clienthandle);

	strcpy(unicast1,"%m");
	strcpy(unicast2,"%M");

	printf("unicast1:%s unicast2:%s\n", unicast1, unicast2);fflush(stdout);

	strcpy(exitcmd1,"%e");
	strcpy(exitcmd2,"%E");

	strcpy(listcmd1,"%l");
	strcpy(listcmd2,"%L");

	strcpy(bcastcmd1,"%b");
	strcpy(bcastcmd2,"%B");

	strcpy(mcastcmd1,"%c");
	strcpy(mcastcmd2,"%C");
}