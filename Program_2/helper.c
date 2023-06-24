#include "helper.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "pollLib.h"
#include "send_recv_pdu.h"

void initializeHelper()
{
	isServer=1;
	int i;	
	for(i=0;i<100;i++){
		clientinfo_str[i].isUsed=0;
		clientinfo_str[i].socketNumber=0;
		memset(clientinfo_str[i].handle,0,100);
		clientinfo_str[i].handlelength=0;
	}
}

int16_t checkFlag(uint8_t * data,int csocket,int numBytes)
{
	int16_t result=0;
	memcpy(&currentFlag,&data[0],sizeof(uint8_t));
	uint8_t client_handle[100];
	uint8_t handlelen=0;
	int rt;		
	uint8_t hand[100];

	uint32_t nc_hbo;
	uint32_t nc_nbo;
	uint32_t lenh;
	int sendhl;
	//printf("Client has sent %u FLAG\n",currentFlag);fflush(stdout);

	switch(currentFlag)
	{
		case 1:
			printf("server has recieved flag1\n");fflush(stdout);
			memcpy(&handlelen,&data[1],sizeof(uint8_t));
			memcpy(client_handle,&data[2],handlelen);
			printf("CLIENT HANDLE NAME: %s\n",client_handle);fflush(stdout);
			
			if(addHandle(client_handle, handlelen,csocket)==-1)
			{
				printf("DUPLICATE Handle exist\n");fflush(stdout);
				sendnegativePacket(csocket);
				result=-1;
			}
			else
			{
				printf("Handle has been added successfully\n");fflush(stdout);
				sendpositivePacket(csocket);
			}
			break;
		
		case 2:
			printf("Postive Response has been successfullay added at the server\n");fflush(stdout);
			result=0;
			break;

		case 3:
			result=-1;
			break;

		case 4:
			int i;
			if(isServer==1){
				printf("Server has recieved a broadcast message\n");fflush(stdout);
				for(i=0;i<100;i++){
					if(clientinfo_str[i].isUsed==1){
						if(clientinfo_str[i].socketNumber!=csocket)
						{
							rt=sendPDU(clientinfo_str[i].socketNumber, data, numBytes);
						}
					}
				}
			}else{
				printf("Client has recieved a broadcast Meesage\n");fflush(stdout);
				char dhand[100];
				uint8_t dhand_len;
				memcpy(&dhand_len,&data[1],1);
				memcpy(&dhand,&data[2],dhand_len);
				uint8_t messageb[1024];
				memcpy(messageb,&data[2+dhand_len],numBytes-2-dhand_len);
				printf("%s: %s\n",dhand,messageb);fflush(stdout);
			}
			break;

		case 5:
			int i;
			if(isServer==1)
			{
					printf("Server has recieved a unicast message\n");
					
					for(i=0;i < numBytes;i++)
					{
						printf("%x ",data[i]);
					}

					uint8_t sendingHandleLen;
					memcpy(&sendingHandleLen,&data[1],1);
					
					uint8_t sendingHandle_name[100];
					memcpy(sendingHandle_name,&data[2],sendingHandleLen);

					printf("\nsending handle Name:%s len:%d\n",sendingHandle_name,sendingHandleLen);fflush(stdout);

					uint8_t destHandleLen;
					memcpy(&destHandleLen,&data[2+sendingHandleLen+1],1);
					
					uint8_t destHandle_name[100];
					memcpy(destHandle_name,&data[2+sendingHandleLen+2],destHandleLen);

					printf("dest handle Name:%s len:%d\n",destHandle_name,destHandleLen);fflush(stdout);

					uint8_t tempdestmsgbuff[1024];
					int copyidx=0;
					int mlen=0;
					copyidx=2+sendingHandleLen+2+destHandleLen;
					mlen=numBytes-(2+sendingHandleLen+2+destHandleLen);
					
					printf("copyidx: %d mlen: %d\n",copyidx,mlen);fflush(stdout);
					memcpy(tempdestmsgbuff,&data[copyidx],mlen);



					printf("destination message buffer:%s\n",tempdestmsgbuff);fflush(stdout);

					int socketNumber=getSocketNumber(destHandle_name,destHandleLen);
					if(socketNumber!=-1){
						uint8_t sb[1500];
						memcpy(sb,&currentFlag,1);
						memcpy(&sb[1],sendingHandle_name,sendingHandleLen);
						char *cln=":";
						memcpy(&sb[1+sendingHandleLen],cln,1);
						memcpy(&sb[1+sendingHandleLen+1],tempdestmsgbuff,mlen);

						rt=sendPDU(socketNumber, sb, (1+sendingHandleLen+1+mlen));

						printf("socketNumber:%d\n",rt);
					}else{
						uint8_t sb[1500];
						uint8_t cf=7;
						memcpy(sb, &cf,1);
						///Client with handle clientX does not exist.
						char s1[]="Client with handle ";
						int size_s1=sizeof(s1);
						memcpy(&sb[1],s1,size_s1);
						memcpy(&sb[1+size_s1],destHandle_name,destHandleLen);
						char s2[]=" does not exist.";
						int size_s2=sizeof(s2);
						memcpy(&sb[1+size_s1+destHandleLen],s2,size_s2);
						int szz=1+size_s1+destHandleLen+size_s2;
						rt=sendPDU(csocket, sb, szz);
						printf("socketNumber:%d\n",rt);
					}
			}else{
				for(i=1;i<numBytes;i++){
					printf("%c",data[i]);
				}
				printf("\n");fflush(stdout);
			}
				break;

		case 6:
			int i;
			if(isServer==1)
			{
					printf("Server has recieved a multicast message\n");

					uint8_t sendingHandleLen;
					memcpy(&sendingHandleLen,&data[1],1);
					
					uint8_t sendingHandle_name[100];
					memcpy(sendingHandle_name,&data[2],sendingHandleLen);

					printf("\nsending handle Name:%s len:%d\n",sendingHandle_name,sendingHandleLen);fflush(stdout);
					uint8_t desthandlecount=0;
					memcpy(&desthandlecount,&data[2+sendingHandleLen],1);

					int messageidxfound=0;

					int copyindex=2+sendingHandleLen+1;

					if(messageidxfound==0){
						
						messageidxfound=2+sendingHandleLen+1;

						for(i=0;i<desthandlecount;i++){
							uint8_t tl=data[messageidxfound];
							
							printf("tl:%d\n",tl);fflush(stdout);

							messageidxfound=messageidxfound+tl+1;
							printf("messageidxfound: %d\n",messageidxfound);fflush(stdout);
						}
					}
					uint8_t tempdestmsgbuff[1024];
					int tmlen=numBytes-messageidxfound;
					memcpy(tempdestmsgbuff,&data[messageidxfound],tmlen);
					printf("destination message buffer:%s\n",tempdestmsgbuff);fflush(stdout);


					
					////////
					for(int itr=0;itr<desthandlecount;itr++){
						uint8_t destHandleLen;
						memcpy(&destHandleLen,&data[copyindex],1);
						++copyindex;

						uint8_t destHandle_name[100];						
						memcpy(destHandle_name,&data[copyindex],destHandleLen);
						copyindex=copyindex+destHandleLen;

						//printf("dest handle Name:%s len:%d\n",destHandle_name,destHandleLen);fflush(stdout);						
						//printf("copyidx: %d mlen: %d\n",copyidx,mlen);fflush(stdout);
						
						int socketNumber=getSocketNumber(destHandle_name,destHandleLen);
						if(socketNumber!=-1)
						{
							uint8_t sb[1500];
							memcpy(sb,&currentFlag,1);
							memcpy(&sb[1],sendingHandle_name,sendingHandleLen);
							char *cln=":";
							memcpy(&sb[1+sendingHandleLen],cln,1);
							memcpy(&sb[1+sendingHandleLen+1],tempdestmsgbuff,tmlen);

							rt=sendPDU(socketNumber, sb, (1+sendingHandleLen+1+tmlen));

							printf("socketNumber:%d\n",rt);
						}else{
							uint8_t sb[1500];
							uint8_t cf=7;
							memcpy(sb, &cf,1);
							///Client with handle clientX does not exist.
							char s1[]="Client with handle ";
							int size_s1=sizeof(s1);
							memcpy(&sb[1],s1,size_s1);
							memcpy(&sb[1+size_s1],destHandle_name,destHandleLen);
							char s2[]=" does not exist.";
							int size_s2=sizeof(s2);
							memcpy(&sb[1+size_s1+destHandleLen],s2,size_s2);
							int szz=1+size_s1+destHandleLen+size_s2;
							rt=sendPDU(csocket, sb, szz);
							printf("socketNumber:%d\n",rt);
						}
					}
			}else{
				for(i=1;i<numBytes;i++){
					printf("%c",data[i]);
				}
				printf("\n");fflush(stdout);
			}
			break;
		case 7:
			int i;
			for(i=1;i<numBytes;i++){
				printf("%c",data[i]);
			}
			printf("\n");fflush(stdout);
			break;

		case 8:
			printf("Client is exitting\n");
			fflush(stdout);
			uint8_t sb[500];
			uint8_t cf=9;
			memcpy(sb, &cf,1);
			rt=sendPDU(csocket, sb, 1);
			printf("socketNumber:%d\n",rt);
			break;
		case 9:
			printf("I have got permission from server to exit\n");
			exit(1);
			break;

		case 10:
			int i;
			printf("Client Has requested me for list of users\n");
			int numberClients=0;
			for(int i=0;i<100;i++)
			{
				if(clientinfo_str[i].isUsed==1){
					numberClients++;
				}
			}
			int numberClients_nbo;
			numberClients_nbo=htonl(numberClients);

			uint8_t sb1[500];
			uint8_t cf1=11;
			memcpy(sb1, &cf1,1);
			memcpy(&sb1[1],&numberClients_nbo,4);
			rt=sendPDU(csocket, sb1, 5);
			printf("socketNumber:%d\n",rt);

			for(i=0;i<100;i++)
			{
				if(clientinfo_str[i].isUsed == 1)
				{
					memset(sb1,0,500);
					cf1=12;
					memcpy(sb1,&cf1,1);
					rt=clientinfo_str[i].handlelength;
					//printf("rt:%d\n",rt);
					memcpy(&sb1[1],&rt,4);
					printf("hllll:%s\n",clientinfo_str[i].handle);
					memcpy(&sb1[5],clientinfo_str[i].handle,rt);
//					sb1[5+rt]='\0';
					sendhl=5+rt;

					rt=sendPDU(csocket, sb1,sendhl);			
					usleep(1000);
				}
			}

			memset(sb1,0,500);
			cf=13;
			memcpy(sb1, &cf,1);
			rt=sendPDU(csocket, sb1, 1);
			
			break;

		case 11:
			memcpy(&nc_nbo,&data[1],4);
			nc_hbo=ntohl(nc_nbo);
			printf("Number of clients:%d\n",nc_hbo);
			break;

		case 12:
			memcpy(&lenh, &data[1],4);
			memcpy(hand, &data[5],lenh);

			//printf("lenh:%d\n",lenh);fflush(stdout);
			
			/*for(int i=0;i<lenh;i++){
				printf("%c",data[i]);
			}
			printf("\n");fflush(stdout);*/
			
			printf("\t%s\n",hand);fflush(stdout);
			
			/*
			*/
			break;

		case 13:
			printf("Server has finished sending list of handles\n");
			//addToPollSet(STDIN_FILENO);
			printf("STDIN_FILENO has been added again to Poll\n");fflush(stdout);
			break;

	}
	return result;
}

int getSocketNumber(uint8_t *handlename,uint8_t hl){
	int ret=-1;
	//printf("\nin getsocketNumber handle:%s hl:%d\n",handlename,hl);
	int i;
	for(i=0;i<100;i++)
	{
	//	printf("handle:%s\n",clientinfo_str[i].handle);fflush(stdout);

		if(memcmp(clientinfo_str[i].handle,handlename,hl)==0){
			printf("I have found socket number:%d\n",clientinfo_str[i].socketNumber);
			ret=clientinfo_str[i].socketNumber;
			break;
		}		
	}

	return ret;
}

void sendnegativePacket(int csock)
{
	uint8_t sendBuf[100];   //data buffer
	int sendLen = 0;        //amount of data to send
	uint8_t FLAG=3;

	memcpy(&sendBuf[0], &FLAG,sizeof(uint8_t));

	sendLen=1;

	int rt=sendPDU(csock, sendBuf, sendLen);

	printf("Bytes send from:%d\n", rt);fflush(stdout);	
}

void sendpositivePacket(int csock)
{
	uint8_t sendBuf[100];   //data buffer
	int sendLen = 0;        //amount of data to send
	uint8_t FLAG=2;

	memcpy(&sendBuf[0], &FLAG,sizeof(uint8_t));

	sendLen=1;

	int rt=sendPDU(csock, sendBuf, sendLen);

	printf("Bytes send from:%d\n", rt);fflush(stdout);	
}

int8_t addHandle(uint8_t *clienthandle, uint8_t handlelen,int csocket)
{
	int8_t ret=-1;
	int i;
	for(i=0; i<100;i++){
		if(clientinfo_str[i].isUsed==1){
			if(handlelen==clientinfo_str[i].handlelength){
				if(memcmp(clienthandle,clientinfo_str[i].handle, handlelen)==0){
					return ret;
				}
			}
		}
	}

	for(i=0;i<100;i++){
		if(clientinfo_str[i].isUsed==0)
		{
			clientinfo_str[i].isUsed=1;
			clientinfo_str[i].handlelength=handlelen;
			memcpy(clientinfo_str[i].handle,clienthandle,handlelen);
			clientinfo_str[i].socketNumber=csocket;
			ret=0;
			return ret;
		}
	}
	return ret;
}
