#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
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

#include "cpe464.h"
#include "networks.h"
#include "sendrecvpdu.h"
#include "pollLib.h"

#include "windowing.h"

#define RET_FILE_DATA 1
#define RET_SERVER_DATA 2
#define RET_ACK_DATA 3

char from_filename[100];
char to_filename[100];
uint32_t window_size;
uint32_t buffer_size;
float error_percent;
char remote_machine[50];
uint32_t remote_port;
int file_ptr = 0;
int new_sn = 1;
int retryCount = 0;
int previous_sn = 0;


int fsocketId;
struct sockaddr_in6 fserver;		// Supports 4 and 6 but requires IPv6 struct
CircularQWindow window;
int GetDataAcknowlegement();
void checkArgs(int argc, char * argv[]);
int sendFileBuffer(int file_ptr, int *seq_num, CircularQWindow *window, int buf_size, int *previous_sn);
int HandleServerResponse(CircularQWindow *window);


int main(int argc, char **argv) {

	setupPollSet();
	printf("Poll setup has been completed\n");fflush(stdout);

    getMemory();

    uint8_t flag = 0;
    int seq_num = 0;
    uint8_t packet[MAX_RLEN];
    uint8_t buf[MAX_LEN];
    uint8_t filenameLen = strlen(to_filename);
    uint32_t ret_recv = 0;


    file_ptr = 0;
    new_sn = 1;
    retryCount = 0;
    previous_sn = 0;

    fsocketId=0;
    checkArgs(argc, argv);
    file_ptr = open(from_filename, O_RDONLY);
  	if(file_ptr==-1){
  		printf("Error: file <%s> not found.\n",from_filename);fflush(stdout);
  		exit(1);
  	}  
    sendErr_init(error_percent, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);
    windowreset(&window, window_size);


    if (fsocketId > 0) {
        close(fsocketId);
    }

	int clientSocket=udpClientSetup(remote_machine, remote_port, &fsocketId,&fserver);	
    
    if (clientSocket < 0) 
    {
        printf("Error in opening client socekt\n");
        fflush(stdout);
        exit(1);
    } 
    else 
    {
        addToPollSet(clientSocket);
        printf("Client Socket has been added to Poll\n");fflush(stdout);
    }
    
    flag = 0;
    seq_num = 0;
    memset(packet,0,sizeof(packet));
    memset(buf,0,sizeof(buf));    
    filenameLen = strlen(to_filename);
    ret_recv = 0;


    memcpy(&buf[0], &window_size, 4);
    memcpy(&buf[4], &buffer_size,4);
    memcpy(&buf[8], &filenameLen, 1);
    memcpy(&buf[9], to_filename, filenameLen);

	int pollfd_sfd;

    int sendSize=9+filenameLen;
	
	int i;
	for(i=0;i<10;i++){
		pollfd_sfd=-1;
        sendBuf(buf, sendSize, FLAG_FILENAME_RCOPY_TO_SERVER, 0, fsocketId,&fserver);
		
		pollfd_sfd=pollCall(1000);

		if(pollfd_sfd==fsocketId)
		{	
            int a=0;	
			ret_recv = recv_buf(packet, MAX_RLEN, fsocketId, &flag, &seq_num,&(a),&fserver);
			if (ret_recv == -1) {
			    printf("CRC ERROR Occured on filename reception packet\n");fflush(stdout);
			} 
            else if (flag == FLAG_FILENAME_SERVER_TO_RCOPY_NAK) 
            {
			    printf("Error on open of output file on server: %s\n", to_filename);
			    fflush(stdout);exit(1);
			    
			} else if (flag == FLAG_FILENAME_SERVER_TO_RCOPY_OK) {
			    break;
			}
		}
    }


    int retvalue=RET_FILE_DATA;

    while (1) {
        if(retvalue==RET_FILE_DATA){
            retvalue = sendFileBuffer(file_ptr, &new_sn, &window, buffer_size, &previous_sn);
        }
        else if(retvalue==RET_SERVER_DATA){
            retvalue = HandleServerResponse(&window);
        }
        else{
            retvalue = GetDataAcknowlegement();
        }
    }
    return 0;
}


void checkArgs(int argc, char * argv[])
{
	
	if (argc != 8)
	{
		printf("usage: %s [from_filename] [to_filename] [window_size] [buffer_size] [error_percent] [remote-machine] [remote-port]\n", argv[0]);
		exit(1);
	}	
	strcpy(from_filename,argv[1]);
	strcpy(to_filename,argv[2]);
	window_size=atoi(argv[3]);
	buffer_size=atoi(argv[4]);
    if((buffer_size < 1) || (buffer_size>1400)){
        buffer_size=1400;
    }
	error_percent=atof(argv[5]);
	strcpy(remote_machine,argv[6]);
	remote_port=atoi(argv[7]);

}

int sendFileBuffer(int file_ptr, int *seq_num, CircularQWindow *window, int buf_size, int *previous_sn) {

    uint8_t buf[MAX_LEN];
    uint32_t num_bytes_read = 0;
    
    int ret = -1;

    if (window->current == window->upper) {
        return RET_ACK_DATA;
    }

    num_bytes_read = read(file_ptr, buf, buf_size);
    if(num_bytes_read<0){
        printf("there is some error while reading file\n");fflush(stdout);
        exit(1);
    }

    if(num_bytes_read==0){
        *previous_sn = *seq_num;
        ret = RET_ACK_DATA;
    }else{
        insertIntoWindow(window, buf, num_bytes_read, *seq_num);
        window->current++;
        sendBuf(buf, num_bytes_read, FLAG_DATA_PACKET, *seq_num,fsocketId,&fserver);
        (*seq_num)++;
        if (num_bytes_read < buf_size) {
            *previous_sn =  *seq_num;
        }
        ret = RET_SERVER_DATA;        
    }
    return ret;
}

void exit_client(){
    char buf[MAX_LEN];
    memset(buf, 0,sizeof(buf));
    sendBuf((uint8_t *)buf, 0, FLAG_EOF, 0,fsocketId,&fserver);
    memset(buf, 0,sizeof(buf));
    sendBuf((uint8_t *)buf, 0,  FLAG_END_CONNECTION, 0,fsocketId,&fserver);
    exit(0);
}

int HandleServerResponse(CircularQWindow *window) {
    int num_bytes_read = 0;
    uint32_t buf_size = 0;
    int a=0;
    uint8_t buf[MAX_LEN];
    uint8_t packet[MAX_RLEN];
    uint8_t flag = 0;
    int seq_num = 0;

	int pollfd_sfd;
	pollfd_sfd=-1;
	pollfd_sfd=pollCall(1000);
	if(pollfd_sfd!=fsocketId)
	{
		return RET_FILE_DATA;
	}	


    num_bytes_read = recv_buf(packet, MAX_RLEN, fsocketId, &flag, &seq_num,&(a),&fserver);

    if (num_bytes_read == -1) {
        return RET_ACK_DATA;
    }

    if (flag == FLAG_SREJ) {
        
        memset(buf,0,sizeof(buf));

        loadFromWindow(window, buf, &buf_size, seq_num);

        sendBuf(buf, buf_size, FLAG_DATA_PACKET, seq_num,fsocketId,&fserver);
    }
    
    if (flag == FLAG_RR) {
        if (seq_num == previous_sn) {
            exit_client();
        } else if (seq_num >= window->lower) {
            moveWindow(window, seq_num);
        }
    } 

    return RET_SERVER_DATA;
}

int GetDataAcknowlegement() {

    uint32_t buf_size = 0;
    uint8_t buf[MAX_LEN];
    memset(buf,0,sizeof(buf));
 
    if (retryCount > 10) {
        printf("Quitting after sending packet maximumum number of times\n");
        clearContents(&window);
        exit(1);
    }
	int pollfd_sfd=-1;
	pollfd_sfd=pollCall(1000);
	if(pollfd_sfd != fsocketId)
	{
		(retryCount) += 1;
		if (previous_sn == window.lower) 
        {
		    return RET_ACK_DATA;
		}
		loadFromWindow(&window, buf, &buf_size, window.lower);
		sendBuf(buf, buf_size, FLAG_DATA_PACKET, window.lower,fsocketId,(struct sockaddr *)&fserver);
		return RET_ACK_DATA;
    }
    retryCount = 0;
    return RET_SERVER_DATA;
}
