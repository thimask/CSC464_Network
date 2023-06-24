
#ifndef SENDRXPDU1_H
#define SENDRXPDU1_H

#include <stdint.h>

#define HEADER_SIZE 7
#define CRC_ERROR -1

#define MEMBUFSIZE 1800

uint8_t *memBuf;
void getMemory();

int sendBuf(uint8_t *buf, uint32_t len, uint8_t flag, uint32_t seq_num, int sockid, void *dAddr);
int recv_buf(uint8_t *buf, int len, int sk_num, uint8_t *flag, int *seq_num, int *clen, void *dAddr);


#endif


