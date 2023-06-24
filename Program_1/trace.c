#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <string.h>

#include <pcap.h>
#include "checksum.h"

#define ARP_REQUEST 1   /* ARP Request             */ 
#define ARP_Reply 2     /* ARP Reply               */ 

typedef struct arphdr1{		//Struct defined for arp headers
	u_int16_t htype;    //header type
    u_int16_t ptype;     
    u_char hlen;         
    u_char plen;         
    u_int16_t oper;      //request or reply
    u_char sha[6];       //source mac
    u_char spa[4];       //source ip
    u_char tha[6];       //destination mac
    u_char tpa[4];      //destination ip
}arphdr_t;


struct pseudo_header {		//pseudo header struct for checksum calculation of tcp header
    unsigned int source_address;	//source address for check sum
    unsigned int dest_address;		//desitnation address for check sum
    unsigned char zero;				//reserved for check sum
    unsigned char protocol;			//protocol for checksum calculation
    unsigned short length;	//length of header
    //struct tcphdr tcp;
};

#define PSEUDO_HEADER_SIZE 12	//pseudo header size equal to 12 Bytes

void packetHandler(u_char *userData, const struct pcap_pkthdr* pkthdr, const u_char* packet);


int pnum=0;

char *ptr;	//char pointer for malloc

int main(int argc, char *argv[]){

	ptr = (char*) malloc(65535 * sizeof(char));	//allocate memory of maximum size of packet
	// if memory cannot be allocated
	if(ptr == NULL) {	// if ptr is null, then exit
		printf("Error! memory not allocated.");
		exit(0);
	}

	pcap_t *fp;	//session handle for pcap
	char errbuf[PCAP_ERRBUF_SIZE];	//buffer array for error

	if(argc != 2){	//check for number of arguments passed to this program
		printf("Please provide correct number of argumnets as .pcap file as argument\n");
		printf("usage: %s PCAP_FILENAME\n",argv[0]);fflush(stdout);
		exit(1);	//if its not equal to 2, then exit it
	}

	fp=pcap_open_offline(argv[1],errbuf);	//open pcap file; using pcap open offline;
	if(fp==NULL){
		perror("pcap_open_offline failed:");
		exit(1);
	}

	if(pcap_loop(fp,0,packetHandler,NULL)<0){	//grap all the packets in a loop and call packet handler funcion
		perror("pcap_loop failed:");
		exit(1);
	}

	//pcap_freecode(&fp);
	pcap_close(fp);	//close session handle
	free(ptr);	//free malloced memory
	return 0;
}

	//function for printing ethernet header
void PrintEthernetHeader(const u_char* packet){
	const struct ether_header* ethernetHeader;
	ethernetHeader = (struct ether_header*)packet;	//type cast packet to ethernet header
	printf("\n\tEthernet Header\n");	
	//print destination MAC
	printf("\t\tDest MAC: %x:%x:%x:%x:%x:%x\n",ethernetHeader->ether_dhost[0],ethernetHeader->ether_dhost[1],ethernetHeader->ether_dhost[2],ethernetHeader->ether_dhost[3],ethernetHeader->ether_dhost[4],ethernetHeader->ether_dhost[5]);
	//print source MAC
	printf("\t\tSource MAC: %x:%x:%x:%x:%x:%x\n",ethernetHeader->ether_shost[0],ethernetHeader->ether_shost[1],ethernetHeader->ether_shost[2],ethernetHeader->ether_shost[3],ethernetHeader->ether_shost[4],ethernetHeader->ether_shost[5]);
}

//function to print UDP header
void PrintUDPHeader(const u_char* packet)
{
	printf("\n\tUDP Header");

	const struct udphdr* udpHeader;
	//type cast packet to ipheader; after size of ether header
	struct iphdr *ipHeader=(struct iphdr*)(packet+sizeof(struct ethhdr));
	//type cast packet to udpheader after size of ether + ip header 
	udpHeader=(struct udphdr*)(packet+sizeof(struct ether_header)+(ipHeader->ihl * 4));
	
	//get source port
	u_int32_t sourcePort=ntohs(udpHeader->source);
	//get destination port
	u_int32_t destPort=ntohs(udpHeader->dest);
	printf("\n\t\tSource Port: : %d ",sourcePort);
	printf("\n\t\tDest Port: : %d \n",destPort);
}

//fuction to print icmp header
void PrintICMPHeader(const u_char* packet)
{
	printf("\n\tICMP Header");

	struct icmphdr* icmpHeader;
	struct iphdr *ipHeader=(struct iphdr*)(packet+sizeof(struct ethhdr));
	//type cast packet to icmp header after size of ether header + ip header
	icmpHeader=(struct icmphdr*)(packet+sizeof(struct ether_header)+(ipHeader->ihl * 4));
	
	//if its ICMP request
	if(icmpHeader->type == ICMP_ECHO){
		printf("\n\t\tType: Request\n");
	}else if(icmpHeader->type == ICMP_ECHOREPLY){		//if its icmp reply
		printf("\n\t\tType: Reply\n");
	}else{	//else print just type
		printf("\n\t\tType: %d\n", icmpHeader->type);
	}
}

//function to calculate tcp checksum
void calculate_tcp_checksum(const struct iphdr* ip, struct tcphdr* tcp) {
	
	//get original checksum as placed in tcp header
	u_int16_t rx_csum=ntohs(tcp->th_sum);

	//printf("\nrx_csum:%x",rx_csum);
	//zero the checksum value
	tcp->check=0;
	tcp->th_sum=0;

	//create object of pseudo header
    struct pseudo_header pseudo_header1;
    unsigned short tcp_length = ntohs(ip->tot_len) - (ip->ihl * 4);
    unsigned short checksum;

	//clear pseudo header
    memset(&pseudo_header1, 0, sizeof(struct pseudo_header));
	//clear buffer for memory as malloced
	memset(ptr, 0, 65535);
	//save source address
    pseudo_header1.source_address = ip->saddr;
	//save desitnation address
    pseudo_header1.dest_address = ip->daddr;
    pseudo_header1.zero = 0;
	//save protocol
    pseudo_header1.protocol = IPPROTO_TCP;
    pseudo_header1.length = htons(tcp_length);

    //memcpy(&pseudo_header1.tcp, tcp, tcp_length);
	//copy pseudo header to start of memory location
	memcpy(ptr,&pseudo_header1,12);
	//copty tcp header to malloc memory location + size of psuedu header
	memcpy(ptr+12,tcp,tcp_length);
	//caluculate check sum
    checksum = in_cksum((unsigned short*)ptr, PSEUDO_HEADER_SIZE + tcp_length);
	checksum=ntohs(checksum);
	//compare both checksum values
	if(checksum==rx_csum)
	{
		printf("\n\t\tChecksum: Correct (0x%x)",rx_csum);
	}else{
		printf("\n\t\tChecksum: Incorrect (0x%x)",rx_csum);
	}
	printf("\n");
}

//function to print tcp header
void PrintTCPHeader(const u_char* packet)
{
	printf("\n\tTCP Header");

	struct tcphdr* tcpHeader;
	struct iphdr *ipHeader=(struct iphdr*)(packet+sizeof(struct ethhdr));
	//type cast packet to tcp header  after size of ether header + ip header
	tcpHeader=(struct tcphdr*)(packet+sizeof(struct ether_header)+(ipHeader->ihl * 4));
	
	//get source port
	u_int32_t sourcePort=ntohs(tcpHeader->source);
	u_int32_t destPort=ntohs(tcpHeader->dest);

	//if port 80; then print HTTP
	if(sourcePort==80){
		printf("\n\t\tSource Port:  HTTP ");
	}else{
		printf("\n\t\tSource Port: : %d ",sourcePort);
	}

	if(destPort==80){
		printf("\n\t\tDest Port:  HTTP ");
	}else{
		printf("\n\t\tDest Port: : %d ",destPort);
	}
	//print sequence number
	printf("\n\t\tSequence Number: %u ",ntohl(tcpHeader->th_seq));
	unsigned int acknumber=ntohl(tcpHeader->th_ack);

//print ack flag; if set, then print ack number else print ack number not valid
	if(tcpHeader->th_flags & TH_ACK){
			printf("\n\t\tACK Number: %u ",acknumber);
			printf("\n\t\tACK Flag: Yes");
	}else{
			printf("\n\t\tACK Number: <not valid>");
			printf("\n\t\tACK Flag: No");
	}

//if syn flag is set, then print SYN flag set
	if(tcpHeader->th_flags & TH_SYN){
			printf("\n\t\tSYN Flag: Yes");
	}else{
			printf("\n\t\tSYN Flag: No");
	}
//if rst flag is set, then print rst flag set
	if(tcpHeader->th_flags & TH_RST){
			printf("\n\t\tRST Flag: Yes");
	}else{
			printf("\n\t\tRST Flag: No");
	}

//if fin flag is set, then print fin flag is set
		if(tcpHeader->th_flags & TH_FIN){
			printf("\n\t\tFIN Flag: Yes");
	}else{
			printf("\n\t\tFIN Flag: No");
	}
	
	//print window size
	printf("\n\t\tWindow Size: %u",ntohs(tcpHeader->th_win));

	//function to calculate tcp checksum
	calculate_tcp_checksum(ipHeader, tcpHeader);
	//tcpHeader->check=in_cksum((unsigned short *)tcpHeader,(ntohs(ipHeader->tot_len)-(ipHeader->ihl * 4)));
	//tcpHeader->check=in_cksum((unsigned short *)ipHeader,56);
	//printf("\ncorrect_csum:%x",ntohs(tcpHeader->check));
/*
	ipHeader->check=0;

	u_int16_t c_csum=in_cksum((unsigned short *)ipHeader,header_length);

	if(rx_csum==c_csum){
		printf("\n\t\tChecksum: Correct (0x%x)",rx_csum);
	}else{
		printf("\n\t\tChecksum: Incorrect (0x%x)",rx_csum);
	}
	*/
}

//fucntion to print IP header
void PrintIpHeader(const u_char* packet){
	
	//struct ip* ipHeader;
	//type cast packet to ip header after size of ether header
	struct iphdr *ipHeader=(struct iphdr*)(packet+sizeof(struct ethhdr));
	//ipHeader=(struct ip*)(packet+sizeof(struct ether_header));

	printf("\t\tType: IP\n");fflush(stdout);
	printf("\n\tIP Header");fflush(stdout);

	//print header length
	int header_length=ipHeader->ihl*4;
	printf("\n\t\tHeader Len: %d (bytes)",header_length);fflush(stdout);
	
	//print type of service
	u_int8_t tpe_service;
	tpe_service=ipHeader->tos;
	printf("\n\t\tTOS: 0x%x ",tpe_service);fflush(stdout);

//print ttl
	u_int8_t tpe_ttl=ipHeader->ttl;
	printf("\n\t\tTTL: %d ",tpe_ttl);fflush(stdout);

//print pdu length
	int len;
	len=ntohs(ipHeader->tot_len);
	printf("\n\t\tIP PDU Len: %d (bytes)",len);fflush(stdout);

//print protocol and go to respective function
	u_int8_t proto=ipHeader->protocol;

	//printf("proto:%d\n",proto);
	if(proto==IPPROTO_UDP)	//if udp packet
	{
		printf("\n\t\tProtocol: UDP");fflush(stdout);	
	}
	else if(proto==IPPROTO_ICMP)
	{
		//if icmp packet
		printf("\n\t\tProtocol: ICMP");fflush(stdout);
	}else if(proto==IPPROTO_TCP)
	{
		//if tcp packet
		printf("\n\t\tProtocol: TCP");fflush(stdout);
	}else{
		printf("\n\t\tProtocol: Unknown");fflush(stdout);
	}
	
	//get original checksum
	u_int16_t rx_csum=(ipHeader->check);
	//set checksum of packet to zero
	ipHeader->check=0;
	//calculate check sum of packet
	u_int16_t c_csum=in_cksum((unsigned short *)ipHeader,header_length);

	//if calculated checksum and original are same
	if(rx_csum==c_csum){
		printf("\n\t\tChecksum: Correct (0x%x)",rx_csum);
	}else{
		printf("\n\t\tChecksum: Incorrect (0x%x)",rx_csum);
	}

	//print sender ip
	struct in_addr ip_addr;
	ip_addr.s_addr=ipHeader->saddr;
	printf("\n\t\tSender IP: %s",inet_ntoa(ip_addr));

	//print destination ip
	ip_addr.s_addr=ipHeader->daddr;
	printf("\n\t\tDest IP: %s\n",inet_ntoa(ip_addr));

	//if udp packet
	if(proto==IPPROTO_UDP)
	{	//then print udp header
		PrintUDPHeader(packet);
	}
	else if(proto==IPPROTO_ICMP)
	{
		//if icmp packet, then print icmp header
		PrintICMPHeader(packet);
	}
	else if(proto==IPPROTO_TCP)
	{
		//if tcp packet, then print tcp header
		PrintTCPHeader(packet);
	}
}

//function to print ARP header
void PrintArpHeader(const u_char* packet){
	
	arphdr_t *arpheader=NULL;

	printf("\t\tType: ARP\n");fflush(stdout);
	printf("\n\tARP header\n");

	//get arp header 
	arpheader=(struct arphdr1 *)(packet+14);

	//if its a arp request or arp reply
	printf("\t\tOpcode: %s", (ntohs(arpheader->oper) == ARP_REQUEST)? "Request" : "Reply");

//print sender mac
	printf("\n\t\tSender MAC: "); 
	for(int i=0; i<6;i++){
		printf("%x", arpheader->sha[i]);
		if(i!=5){
			printf(":");
		}
	}

//print sender ip
	printf("\n\t\tSender IP: "); 

	for(int i=0; i<4;i++){
		printf("%d", arpheader->spa[i]);
		if(i!=3){
			printf(".");
		}
	}

//print destination mac
	printf("\n\t\tTarget MAC: "); 
	for(int i=0; i<6;i++){
		printf("%x", arpheader->tha[i]);
		if(i!=5){
			printf(":");
		}
	}

//print destination ip
	printf("\n\t\tTarget IP: "); 

	for(int i=0; i<4;i++){
		printf("%d", arpheader->tpa[i]);
		if(i!=3){
			printf(".");
		}
	}
	printf("\n\n");
}

//packet handler function called from pcap_loop
void packetHandler(u_char *userData, const struct pcap_pkthdr* pkthdr, const u_char* packet){
	
	const struct ether_header* ethernetHeader;
	//const struct ip* ipHeader;
	//const struct tcphdr* tcpHeader;
	//const struct udphdr* udpHeader;
	ethernetHeader = (struct ether_header*)packet;

	printf("\nPacket number: %d  Frame Len: %d\n",++pnum,pkthdr->len);

	//print erthernet header
	PrintEthernetHeader(packet);

	//if its arp packet
	if(ntohs(ethernetHeader->ether_type) == ETHERTYPE_ARP)
	{
		PrintArpHeader(packet);
	}
	//if its ip packet
	else if(ntohs(ethernetHeader->ether_type) == ETHERTYPE_IP)
	{
		PrintIpHeader(packet);
	}


}