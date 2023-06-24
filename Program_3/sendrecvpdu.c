
#include "sendrecvpdu.h"
#include "networks.h"
#include "safeUtil.h"
#include "checksum.h"
#include <stdio.h>


void getMemory(){
	// Allocate memory dynamically
    memBuf = (uint8_t*) malloc(MEMBUFSIZE);

	// Check if memory was allocated successfully
    if (memBuf == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }
}


int sendBuf(uint8_t *buf, uint32_t len, uint8_t flag, uint32_t seq_num, int sockid,void *dAddr)
{
	//struct sockaddr *destAddr = (struct sockaddr *)dAddr;
	memset(memBuf,0,MEMBUFSIZE);

	uint32_t sentLen = 0;
	uint32_t sendingLen = 0;

	if (len > 0) {
		memcpy(&memBuf[7], buf, len);
	}

	uint16_t checksum = 0;

	sendingLen=len+HEADER_SIZE;
	
	seq_num = htonl(seq_num);
	memcpy(&memBuf[0], &seq_num, sizeof(seq_num));
	memcpy(&memBuf[4], &checksum, sizeof(checksum));
	memcpy(&memBuf[6], &flag, sizeof(flag));
	
	checksum = in_cksum((unsigned short *) memBuf, sendingLen);

	memcpy(&memBuf[4], &checksum, sizeof(checksum));

	sentLen = SendviaErrLibrary(memBuf, sendingLen, sockid,dAddr);

	return sentLen;
}

int recv_buf(uint8_t *buf, int len, int sk_num, uint8_t *flag, int *seq_num, int *clen,void *dAddr) {

	//struct sockaddr *destAddr = (struct sockaddr *)dAddr;
	char data_buf[MAX_LEN];
	int recv_len = 0;
	int dataLen = 0;

	int rlen=0;
	recv_len = RecvviaErrLibrary(sk_num, data_buf, len, dAddr,&rlen);

//	connection->len = rlen;
	*clen=rlen;
	
	uint16_t rxchecksum=0;
	memcpy(&rxchecksum,&data_buf[4],sizeof(rxchecksum));
	
	uint16_t temp_cs=0;
	memcpy(&data_buf[4],&temp_cs,sizeof(temp_cs));
	
	temp_cs=in_cksum((unsigned short*) data_buf, recv_len);

//	printf("Rxd CS:%u\n",temp_cs);fflush(stdout);

	if(temp_cs != rxchecksum){
		return CRC_ERROR;
	}
	
	memcpy(flag, &data_buf[6],1);
	memcpy(seq_num, &data_buf[0],4);
	*seq_num=ntohl(*seq_num);
	dataLen=recv_len-HEADER_SIZE;


	if (dataLen > 0) {
		memcpy(buf, data_buf + HEADER_SIZE, recv_len);
	}

	return dataLen;
}
