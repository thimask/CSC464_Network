#ifndef __SAFEUTIL_H__
#define __SAFEUTIL_H__

#include <stdint.h>
#include <netinet/in.h>
struct sockaddr;

int safeRecvfrom(int socketNum, void * buf, int len, int flags, struct sockaddr *srcAddr, int * addrLen);
int safeSendto(int socketNum, void * buf, int len, int flags, struct sockaddr *srcAddr, int addrLen);
int safeRecv(int socketNum, void * buf, int len, int flags);
int safeSend(int socketNum, void * buf, int len, int flags);

void * srealloc(void *ptr, size_t size);
void * sCalloc(size_t nmemb, size_t size);


int SendviaErrLibrary(uint8_t *packet, uint32_t len, int socketNumber, struct sockaddr *destAddr);

int RecvviaErrLibrary(int sk_num, char *data_buf, int len, struct sockaddr *destAddr, int *rlen);

#endif
