#ifndef __HELPER_H__
#define __HELPER_H__

#include <stdint.h>

struct ClientInfo {
  char handle[100];
  uint8_t isUsed;
  int socketNumber;
  int handlelength;
};


struct ClientInfo clientinfo_str[100];

void initializeHelper();

uint8_t currentFlag;

int16_t checkFlag(uint8_t * data,int csocket,int numbytes);

int8_t addHandle(uint8_t *clienthandle, uint8_t handlelen,int csocket);

void sendpositivePacket(int csocket);
void sendnegativePacket(int csock);

int getSocketNumber(uint8_t *handlename,uint8_t hl);

int isServer;

#endif
