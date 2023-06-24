
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>

#include "cpe464.h"
#include "networks.h"
#include "sendrecvpdu.h"
#include "pollLib.h"

#include "windowing.h"

#define RET_RECVFILE_DATA 1
#define RET_TERMINATION 2
#define RET_RECOVER_PKTS 3

int fsocketId;
struct sockaddr_in6 fserver;		// Supports 4 and 6 but requires IPv6 struct
int temp=0;
uint8_t temp8=0;
int checkArgs(int argc, char * argv[]);
CircularQWindow window;
int file_ptr = 0;


void forkedServer( uint8_t *buf, int recv_len);
int recv_filename(uint8_t *buf, int recv_len, int *file_ptr, uint32_t *buf_size, uint32_t *window_size, CircularQWindow *window);
void exitForkedServer();

int recv_file_data(uint8_t *buf, int *file_ptr, int *data_received, CircularQWindow *window, int *retryCount);
int wait_client_packets(uint8_t *buf, int *file_ptr, CircularQWindow *window);
int client_termination_wait(uint8_t *buf, int *retryCount);

void TxRequestReady(uint32_t seq_num) {
    uint32_t seqNumRR = htonl(seq_num);
    sendBuf((uint8_t *)&seqNumRR, 4, FLAG_RR, seq_num,fsocketId,&fserver);
}

void TxSelectiveReject(uint32_t seq_num) {
    uint32_t seqNumSREJ = htonl(seq_num);
    sendBuf((uint8_t *)&seqNumSREJ, 4, FLAG_SREJ, seq_num,fsocketId,&fserver);
}

float error_percent;

int main(int argc, char **argv) {

    getMemory();

    sendtoErr_init(error_percent, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);

    int portNumber = checkArgs(argc, argv);
    
    fsocketId = udpServerSetup(portNumber);

    uint8_t buf[MAX_RLEN];
    uint8_t flag = 0;
    int seq_num = 0;
    int recv_len = 0;

    while(1) {

	    if(recv_buf(buf, MAX_RLEN, fsocketId, &flag, &seq_num,&temp,&fserver)>0)
        {
            if (fork() == 0) {
                forkedServer(buf, recv_len);
                exit(0);
            }
        }
    }
    return 0;
}

int checkArgs(int argc, char * argv[])
{
    int portNumber = 0;	
	if (argc != 3)
	{
		printf("usage: %s [error_percent]  [my-port]\n", argv[0]);
		exit(1);
	}	
	error_percent=atof(argv[1]);
	portNumber=atoi(argv[2]);
	if(portNumber==0){
		portNumber=40000;
	}

    return portNumber;
}

void forkedServer(uint8_t *buf, int recv_len) 
{
    sendtoErr_init(error_percent, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);    //calling send_err_init for child process
	
    setupPollSet();
	
    printf("Poll setup has been completed at forked server\n");fflush(stdout);
    uint32_t buf_size = 0;
    uint32_t window_size = 0;
    int data_received = 0;
    static int retryCount = 0;

    int retvalue = recv_filename (buf, recv_len, &file_ptr, &buf_size, &window_size, &window);


    while (1) {
        if(retvalue == RET_RECVFILE_DATA)
        {
            retvalue = recv_file_data(buf, &file_ptr, &data_received, &window, &retryCount);
        }
        else if(retvalue == RET_RECOVER_PKTS)
        {
            retvalue = wait_client_packets(buf, &file_ptr, &window);
        }
        else
        {            
            retvalue = client_termination_wait(buf, &retryCount);
        }
    }

    exit(0);
}


int recv_filename(uint8_t *buf, int recv_len, int *file_ptr, uint32_t *buf_size, uint32_t *window_size, CircularQWindow *window) 
{

    char fname[MAX_LEN];
    uint8_t filenameLen;

    memcpy(window_size, &buf[0], 4);
    memcpy(buf_size, &buf[4], 4);
    memcpy(&filenameLen, &buf[8], 1);
    memcpy(fname, &buf[9], filenameLen);

    fname[filenameLen] = '\0';
    close(fsocketId);   //clsing parent socket in child process
    srand(time(NULL));
    int fserverPort=rand()%1000;
    fserverPort=fserverPort+41000;
    fsocketId=udpServerSetup(fserverPort);    
    addToPollSet(fsocketId);
	
    printf("Client Socket has been added to Poll\n");fflush(stdout);
	
    *file_ptr = open(fname, O_CREAT | O_CREAT | O_TRUNC | O_WRONLY, 0777);

    windowreset(window, *window_size);

    temp8=0;

    if (*file_ptr > 0) 
    {
        sendBuf(&temp8, 0, FLAG_FILENAME_SERVER_TO_RCOPY_OK, 0, fsocketId,&fserver);
        return RET_RECVFILE_DATA;
    }
     else if (*file_ptr < 0) 
    {
        sendBuf(&temp8, 0, FLAG_FILENAME_SERVER_TO_RCOPY_NAK, 0,fsocketId,&fserver);
        printf("I could not open the file for writing at server forked process\n");
        exit(1);
    }

    return 0;
}

void exitForkedServer(){
    close(file_ptr);
    clearContents(&window);
    exit(0);
}

int recv_file_data(uint8_t *buf, int *file_ptr, int *data_received, CircularQWindow *window, int *retryCount) {

    int returnValue = RET_RECVFILE_DATA;
    uint8_t flag = 0;
    int seq_num = 0;
    uint8_t packet[MAX_LEN];
    uint32_t data_len = 0;
    int i;


    if (*retryCount > 10) {
        printf("Server waited for clients; but didnot recieved. so quitting now\n");fflush(stdout);
        exitForkedServer();
    }

	int pollfd_sfd=-1;
	pollfd_sfd=pollCall(1000);
	if(pollfd_sfd != fsocketId){
		(*retryCount)++;
		return RET_RECVFILE_DATA;
	}
    *retryCount = 0;
    data_len = recv_buf(packet, MAX_LEN, fsocketId, &flag, &seq_num,&temp,&fserver);
    if (data_len == -1) {
        return RET_RECVFILE_DATA;
    }

    if (flag == FLAG_EOF) {
        returnValue = RET_TERMINATION;
    } else if (flag == FLAG_DATA_PACKET) {
        
        if (seq_num == window->lower) {
            insertIntoWindow(window, packet, data_len, seq_num);
            for (i = window->lower; i < window->upper; i++) {
                if (window->occupied[i % window->win_sz] == 0) {
                    break;
                }

                loadFromWindow(window, packet, &data_len, i);
                write(*file_ptr, packet, data_len);
            }
            moveWindow(window, i);
            TxRequestReady(window->lower);
        } else if (seq_num < window->lower) {
            TxRequestReady(window->lower);
        } 
        else if (seq_num > window->upper) 
        {
            exitForkedServer();
        } 
        else
        {
            insertIntoWindow(window, packet, data_len, seq_num);
            window->current = window->lower;

            TxSelectiveReject(window->lower);
            returnValue = RET_RECOVER_PKTS;
        }
    }
    return returnValue;
}

int wait_client_packets(uint8_t *buf, int *file_ptr, CircularQWindow *window) {
    uint32_t data_len = 0;
    int i;
    uint8_t flag = 0;
    int seq_num = 0;
    uint8_t packet[MAX_LEN];

    int pollfd_sfd=-1;
	pollfd_sfd=pollCall(10000);
	if(pollfd_sfd != fsocketId){
		printf("server is not reciveing after waiting 10Seconds timeout");
		exitForkedServer();
	}

	data_len = recv_buf(packet, MAX_LEN, fsocketId, &flag, &seq_num,&temp,&fserver);

    if (data_len == -1) {
        return RET_RECOVER_PKTS;
    } else if (seq_num >= window->lower && seq_num <= window->upper) {
        insertIntoWindow(window, packet, data_len, seq_num);
        for (i = window->lower; i <= window->upper + 1; i++) {
            int index = i % window->win_sz;
            if (window->occupied[index] == 0) {
                window->current = i;
                break;
            }
        }

        for (i = window->lower; i < window->current; i++) {
            loadFromWindow(window, packet, &data_len, i);
            write(*file_ptr, packet, data_len);
        }

        moveWindow(window, window->current);
        TxRequestReady(window->lower);


        return RET_RECVFILE_DATA;
    }

    TxRequestReady(window->current);
    return RET_RECOVER_PKTS;
}

int client_termination_wait(uint8_t *buf, int *retryCount) 
{
    int returnValue = RET_TERMINATION;
    //uint8_t packet[MAX_LEN];
    uint8_t flag = 0;
    int seq_num = 0;
    uint32_t data_len = 0;

    sendBuf(buf, 0, FLAG_END_CONNECTION, 0,fsocketId,&fserver);
    int pollfd_sfd=-1;
	pollfd_sfd=pollCall(1000);
	if(pollfd_sfd == fsocketId){
	

        data_len = recv_buf(buf, MAX_LEN, fsocketId, &flag, &seq_num,&temp,&fserver);
        if (data_len == -1) {

        } else if (flag == FLAG_END_CONNECTION) {
            printf("Client exited - server now exiting\n");
            exitForkedServer();
        }
    } else {
        (*retryCount)++;
    }

    return returnValue;
}