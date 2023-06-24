#ifndef SENDRXPDU_H
#define SENDRXPDU_H

uint8_t *memBuf;

#define MEMBUFSIZE 2500
#define HEADERSIZE 2
#define CHATHEADERSIZE 3

int sendPDU(int clientSocket, uint8_t * dataBuffer, uint16_t lengthOfData);
int recvPDU(int socketNumber, uint8_t * dataBuffer, int bufferSize);

void getMemory();
#endif
