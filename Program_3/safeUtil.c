#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <netinet/in.h>
#include <netdb.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "safeUtil.h"

#ifdef __LIBCPE464_
#include "cpe464.h"
#endif


int SendviaErrLibrary(uint8_t *packet, uint32_t len, int socketNumber, struct sockaddr *destAddr) {
	
	int send_len = 0;

	if ((send_len = sendtoErr(socketNumber, packet, len, 0, destAddr, sizeof(struct sockaddr_in6))) < 0) {
		perror("in send_buf(), sendto() call");
		exit(-1);
	}

	return send_len;
}

int RecvviaErrLibrary(int sk_num, char *data_buf, int len, struct sockaddr *destAddr, int *rlen) {
	int recv_len = 0;
	uint32_t remote_len = sizeof(struct sockaddr_in6);

	if ((recv_len = recvfrom(sk_num, data_buf, len, 0, destAddr, &remote_len)) < 0) {
		perror("recv_buf, recvfrom");
		exit(-1);
	}

	*rlen = remote_len;

	return recv_len;

}

int safeRecvfrom(int socketNum, void * buf, int len, int flags, struct sockaddr *srcAddr, int * addrLen)
{
	int returnValue = 0;
	if ((returnValue = recvfrom(socketNum, buf, (size_t) len, flags, srcAddr, (socklen_t *) addrLen)) < 0)
	{
		perror("recvfrom: ");
		exit(-1);
	}
	
	return returnValue;
}

int safeSendto(int socketNum, void * buf, int len, int flags, struct sockaddr *srcAddr, int addrLen)
{
	int returnValue = 0;
	if ((returnValue = sendto(socketNum, buf, (size_t) len, flags, srcAddr, (socklen_t) addrLen)) < 0)
	{
		perror("sendto: ");
		exit(-1);
	}
	
	return returnValue;
}

int safeRecv(int socketNum, void * buf, int len, int flags)
{
	int returnValue = 0;
	if ((returnValue = recv(socketNum, buf, (size_t) len, flags)) < 0)
	{
		perror("recv: ");
		exit(-1);
	}
	
	return returnValue;
}

int safeSend(int socketNum, void * buf, int len, int flags)
{
	int returnValue = 0;
	if ((returnValue = send(socketNum, buf, (size_t) len, flags)) < 0)
	{
		perror("send: ");
		exit(-1);
	}
	
	return returnValue;
}

void * srealloc(void *ptr, size_t size)
{
	void * returnValue = NULL;
	
	if ((returnValue = realloc(ptr, size)) == NULL)
	{
		printf("Error on realloc (tried for size: %d\n", (int) size);
		exit(-1);
	}
	
	return returnValue;
} 

void * sCalloc(size_t nmemb, size_t size)
{
	void * returnValue = NULL;
	if ((returnValue = calloc(nmemb, size)) == NULL)
	{
		perror("calloc");
		exit(-1);
	}
	return returnValue;
}


