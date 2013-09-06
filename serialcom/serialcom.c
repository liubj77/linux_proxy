#include "serialcom.h"
#include "hdlc.h"
#include "uart.h"
#include "command.h"

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <netdb.h>
#include <net/if.h>
//#include <net/if_arp.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <sys/stat.h>

#define SERVER_PORT 2080

pthread_t notif_thread;
pthread_mutex_t m_lock;
//int32_t fd;
struct serial_state_t *ptr_state=NULL;
/*************************************************************************/
void *recv_thread(void *arg);
int connect_server(char * arg1,char * arg2);   //arg1:server IP   arg2: interface. eg."eth0"
//static int8_t hdlc_q_byte(struct hdlc_buff_t* lm, uint8_t byte, uint16_t max_size);
/*************************************************************************/

int main( int argc, char *argv[] )
{
	ptr_state = (struct serial_state_t*)malloc(sizeof(struct serial_state_t));
	//ptr_state->commandack=1;
#if 0	
	if(argc != 3)
	{
		printf("use: %s <serverIP><interface> format!!!\n",argv[0]);
		exit(1);
	}
#endif
#if 0
	/****connect to the server******/

	struct hostent *h;
	char server_ip[40];
	if((h=gethostbyname(argv[1])) == NULL)
	{
		fprintf(stderr,"\nCan not get the server IP!\n");
		exit(1);
	}
	strncpy(server_ip,inet_ntoa(*((struct in_addr *)h->h_addr)),40);
	printf("\nThe distance server IP Address: %s\n",server_ip);
#endif
#if 1
	char device[32];
	strncpy(device,argv[2],32);
	if(!connect_server(argv[1],device))
	{
		perror("Connect server failed\n");
		exit(1);
	}
#endif
	/****open and config uart************/
	ptr_state->comfd=open_uart("/dev/ttyUSB3");
	if(config_uart(ptr_state->comfd,115200,8,1,'N') == -1)
	{
		perror("Configure UART error.\n");
		exit(1);
	}
	
	/***** create thread to deal with data **/
	pthread_mutex_init(&m_lock,NULL);//initialize mutex lock
	void *thread_result;
	int result = pthread_create(&notif_thread,NULL,recv_thread,ptr_state);
	if(result != 0)
	{
		perror("Notification thread create failed!\n");
	}	
	/*opened the uart port and start to receive data*/
	printf("Start send and receive data.\n");
		
	printf("Waiting notification thread to finish...\n");
	/*wait until the receive thread return*/
	result = pthread_join(notif_thread,&thread_result);
	if(result != 0)
	{
		perror("Thread join failed!\n");
		exit(EXIT_FAILURE);	//EXITFAILURE: 1
	}
	return 0;
	
}	
	
	
	
void *recv_thread(void *arg)
{
	int send_bytes;
	/*rxbuff_hdlc hold the packet with no flag,txbuff_hdlc hold the whole packet with flag*/
	struct hdlc_buff_t    	rxbuff_hdlc,txbuff_hdlc,txbuff_sock;
	uint8_t   				is_hdlc_packet = 0;
	uint8_t 				byte = 0;
	int						rtn = 0,ret = 0,wcnt = 0;
	uint8_t					rxbuff_no_flag[MAX_PACKET_BYTE_SIZE_HDLC];
	uint8_t 				rxbuff_sock[MAX_PACKET_BYTE_SIZE_HDLC];
	uint16_t				bytes_written;
	int32_t				maxfd;
	/*packet_seq shoule be a global variable*******/
	//static uint8_t			packet_seq = 0;
	struct serial_state_t *ptmp_state=arg;
	memset(&rxbuff_hdlc,  0, sizeof(struct hdlc_buff_t));//receive hex data from manager
	memset(&txbuff_hdlc,  0, sizeof(struct hdlc_buff_t));//send hex data to manager
	memset(&txbuff_sock,  0, sizeof(struct hdlc_buff_t));//transfer socket command data via serial port
	memset(rxbuff_no_flag,0, sizeof(rxbuff_no_flag));
	memset(rxbuff_sock,	  0, sizeof(rxbuff_sock));//socket receive data
		
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	fd_set rdfds;
	struct stat tstat;
	
	maxfd = (ptmp_state->comfd > ptmp_state->SockServerfd ? ptmp_state->comfd:ptmp_state->SockServerfd)+1;
	while(1)
	{
			
		FD_ZERO(&rdfds);
		if(-1 != fstat(ptmp_state->comfd,&tstat))
			FD_SET(ptmp_state->comfd,&rdfds);//add comfd
		if(-1 != fstat(ptmp_state->SockServerfd,&tstat))
		FD_SET(ptmp_state->SockServerfd,&rdfds);//add serverfd
		ret=select(maxfd,&rdfds,NULL,NULL,&tv);
		if(ret<0)
		{
			/*receive serial data error ,close the port***********/
			perror("select");
		}
		else if(ret==0)
		{
			sleep(1);
			continue;
		}
		else
		{
			if(FD_ISSET(ptmp_state->comfd,&rdfds))
			{
				int n = read(ptmp_state->comfd,&byte,sizeof(byte));
#if 1
				//printf("%02x ",byte);
				//fflush(stdout);	
#endif
			/******** packet the receive byte into a whole hdlc format *****/
				is_hdlc_packet = hdlc_q_byte(&rxbuff_hdlc, byte, MAX_PACKET_BYTE_SIZE_HDLC);
				if(!is_hdlc_packet)
					continue;
				else
				{
					//printf("\n");
					//fflush(stdout);
					/*receive a whole hdlc packet and check fcs*/
					rtn = hdlc_pkdecode(rxbuff_hdlc.data, rxbuff_hdlc.len, rxbuff_no_flag,
												 MAX_PACKET_BYTE_SIZE_HDLC, &bytes_written);
					if(rtn!=0)   //the received hdlc packet has something wrong
						continue;
					/*if rtn equals 1,that means the serial has received a packet without wrong*/
					else
					{
#if 0
						/*stdout in order to see the result*/
						printf("Received: %s---%s--",rxbuff_no_flag,rxbuff_hdlc.data);
						fflush(stdout);
#endif
						/******** store client sequence,not consider data notification **********/
						//ptmp_state->clientseq = rxbuff_no_flag[2];
						/*receive the right packet ,and rxbuff_no_flag_space is a buffer without 0x7e,0x7d,and crc */
						if(rxbuff_no_flag[1]==3)  //3:MgrHello
						{
							/*this is a MgrHello packet,then send hello packet*/
							wcnt = write(ptmp_state->comfd,hello,sizeof(hello));
							send_bytes = send(ptmp_state->SockServerfd,rxbuff_hdlc.data,rxbuff_hdlc.len,0);

						//	sleep(1); //usleep(500000); //sleep 500ms
						}
						else if(rxbuff_no_flag[1]==2)   //HelloResponse
						{
							/*this is a hello response packet,get packetId			*/
							/* build the hdlc packet ,and then send subscribe packet*/
							ptmp_state->clientseq = rxbuff_no_flag[2];
							subscribe[2]= ptmp_state->clientseq + 1;
							hdlc_pkgen(NULL,0,subscribe,sizeof(subscribe),txbuff_hdlc.data,
												MAX_PACKET_BYTE_SIZE_HDLC,&txbuff_hdlc.len);
							
							ptmp_state->commandtype = 22; ///subscribe command
							//pthread_mutex_lock(&m_lock);
							wcnt = write(ptmp_state->comfd,txbuff_hdlc.data,txbuff_hdlc.len);
							//pthread_mutex_unlock(&m_lock);
#if 0
							/*stdout in order to see the result*/
							printf("Translate:subscribe %d--%d-",wcnt,txbuff_hdlc.len);
							fflush(stdout);
#endif
						}
						else if(rxbuff_no_flag[1]==ptmp_state->commandtype)//command response
						{
							ptmp_state->clientseq = rxbuff_no_flag[2];
							ptmp_state->commandack=1;
							if(ptmp_state->commandtype != 22 && ptmp_state->commandtype != 0x2c)  //非subscribe命令相应时，进行转发
							{
								//pthread_mutex_lock(&m_lock);
								//ptmp_state->commandack=1;
								//pthread_mutex_unlock(&m_lock);
								/*if the packet is the last command ack,sent back to the server*/
								send_bytes = send(ptmp_state->SockServerfd,rxbuff_hdlc.data,rxbuff_hdlc.len,0);	
								if(send_bytes == -1)
								{
									perror("data packets send to the server failed!\n");
								}
								//else
								//{
									//printf("data packets send to the server success!\n");
								//}
							}
						}
						else if(rxbuff_no_flag[1]==20)
						{
							/** deal with data notification**/
							//ackNotification[2] = rxbuff_no_flag[2];
						//	hdlc_pkgen(NULL,0,ackNotification,sizeof(ackNotification),txbuff_hdlc.data,
							//		MAX_PACKET_BYTE_SIZE_HDLC,&txbuff_hdlc.len);
						//	pthread_mutex_lock(&m_lock);
							//wcnt = write(ptmp_state->comfd,txbuff_hdlc.data,txbuff_hdlc.len);
						//	pthread_mutex_unlock(&m_lock);
							//usleep(700000);
							if(rxbuff_no_flag[4] == 1)
							{
								if(rxbuff_no_flag[9]==3 || rxbuff_no_flag[9]==4||rxbuff_no_flag[9]==5||rxbuff_no_flag[9]==10||rxbuff_no_flag[9]==11)
								{
									send_bytes = send(ptmp_state->SockServerfd,rxbuff_hdlc.data,rxbuff_hdlc.len,0);	
									if(send_bytes == -1)
									{
										perror("data packets send to the server failed!\n");
									}
									//else
									//{
										//printf("data packets send to the server success!\n");
									//}
								}
							}
							if(rxbuff_no_flag[4] == 4) //data && event notification
							{
								if(rxbuff_no_flag[3] != 0x34)//if 0x34,it is unknown data notification,just ignore it
								{
									send_bytes = send(ptmp_state->SockServerfd,rxbuff_hdlc.data,rxbuff_hdlc.len,0);	
									if(send_bytes == -1)
									{
										perror("data packets send to the server failed!\n");
									}
									//else
									//{
										//printf("data packets send to the server success!\n");
									//}
								}
							}
						}
					}
				}	
			}
			else if(FD_ISSET(ptmp_state->SockServerfd,&rdfds))
			{
				memset(rxbuff_sock,0,MAX_PACKET_BYTE_SIZE_HDLC);
				int	recv_bytes = recv(ptmp_state->SockServerfd,rxbuff_sock,sizeof(rxbuff_sock)-1,0);
				if(recv_bytes != -1)
				{
					if(rxbuff_sock[1] !=0x2c){
					/**** receive commands from the server****/
						if(ptmp_state->commandack==1)   //only response a command one time
						{
							/*
							if(rxbuff_sock[1] == ptmp_state->commandtype)   //if repeat the last command
								rxbuff_sock[2]=(ptmp_state->clientseq)%256;
							else 
							*/
							/* in the newer version,every command sequence number show add one. */
							rxbuff_sock[2]=(ptmp_state->clientseq + 1)%256;
						
							ptmp_state->commandtype = rxbuff_sock[1];

							hdlc_pkgen(NULL,0,rxbuff_sock,recv_bytes,txbuff_sock.data,
													MAX_PACKET_BYTE_SIZE_HDLC,&txbuff_sock.len);
					
							pthread_mutex_lock(&m_lock);
							ptmp_state->commandack=0;
							wcnt = write(ptmp_state->comfd,txbuff_sock.data,txbuff_sock.len);
							pthread_mutex_unlock(&m_lock);		
						}	
					}
					else	
					/* receive data from the server */
					{
						if(ptmp_state->commandack==1)
						{
							rxbuff_sock[2]=(ptmp_state->clientseq + 1)%256;
							ptmp_state->commandtype = rxbuff_sock[1];
							hdlc_pkgen(NULL,0,rxbuff_sock,recv_bytes,txbuff_sock.data,
													MAX_PACKET_BYTE_SIZE_HDLC,&txbuff_sock.len);
							wcnt = write(ptmp_state->comfd,txbuff_sock.data,txbuff_sock.len);
						}
					}	
				}
				else if(recv_bytes == 0)
				{
					perror("SockServerfd receive data failed,SockServerfd closed\n");
					close(ptmp_state->SockServerfd);
				}
				else
				{
					perror("SockServerfd channel can not be reached,receive data failed!\n");
					close(ptmp_state->SockServerfd);
				}
			}
		}
	}
}

//**** check whether all bytes of the hdlc package already received ****//
static int8_t hdlc_q_byte(struct hdlc_buff_t* lm, uint8_t byte, uint16_t max_size)
{
    int8_t is_hdlc_packet=0;
    if((byte==HDLC_FLAG) && (lm->idx==0))
    {
    	/*start of HDLC packet*/
        lm->data[lm->idx++] = byte;
    } 
    else if((HDLC_FLAG==byte) && (0<lm->idx))
    {
        /* End of HDLC packet */
        if ( HDLC_FLAG == lm->data[0] ) 
        {
            /* HDLC packet appears to be valid. Lets continue. */
            lm->data[lm->idx++] = byte;
            lm->len = lm->idx;
            is_hdlc_packet = 1;
        }
        lm->idx = 0;
    } 
    else 
    {
        /* Middle of packet */
        if ( lm->idx < max_size )
            lm->data[lm->idx++] = byte;
        else 
        {
            /* packet too big! Lets start over. */
            lm->idx = 0;
        }
    }

    return is_hdlc_packet;
}

/*****connect server through specified interface****/
int connect_server(char * arg1,char * arg2)   //arg1:server IP   arg2: interface. eg."eth0"
{
	char *local_ip,*server_ip;
    struct sockaddr_in *my_ip;
    struct sockaddr_in myip;
    my_ip = &myip;
    struct ifreq ifr;
    int sock;
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
       return 0;
    }
    strcpy(ifr.ifr_name, arg2);
    if(ioctl(sock, SIOCGIFADDR, &ifr) < 0)
    {
        return 0;
    }
    my_ip->sin_addr = ((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr;
    local_ip = inet_ntoa(my_ip->sin_addr);//get ppp0/eth0 ip,that is 3G ip
    printf("%s ip:%s\n",arg2,local_ip);
    close(sock);
    
    //dust_inst_obj->addr_local
    bzero(&ptr_state->addr_local,sizeof(ptr_state->addr_local));

	ptr_state->addr_local.sin_family = AF_INET;
	ptr_state->addr_local.sin_port = htons(SERVER_PORT);
	ptr_state->addr_local.sin_addr.s_addr = inet_addr(local_ip);
	bzero(&(ptr_state->addr_local.sin_zero),8);

	if((ptr_state->SockServerfd = socket(AF_INET,SOCK_STREAM,0)) < 0)	//create control socket
	{
		printf("Create server_connect socket error!\n");
		return 0;
	}
	if(-1 == bind(ptr_state->SockServerfd, (struct sockaddr*)&ptr_state->addr_local, sizeof(struct sockaddr)))
	{
		printf("Bind to local %s IP failed!\n",arg2);
	}
	
	////try to connect to the distant server through 3G
//	if((dust_inst_obj->SockServerfd = socket(AF_INET,SOCK_STREAM,0)) < 0)	//create control socket
//	{
//		printf("Create server_connect socket error!\n");
//		return 1;
//	}
	printf("\nTrying to connect to the distant server through %s...\n",arg2);
	
	server_ip = arg1;
	bzero(&ptr_state->addr_server,sizeof(ptr_state->addr_server));
	ptr_state->addr_server.sin_family = AF_INET;
	ptr_state->addr_server.sin_port = htons(SERVER_PORT);
	ptr_state->addr_server.sin_addr.s_addr = inet_addr(server_ip);
	bzero(&(ptr_state->addr_server.sin_zero),8);
	
	if(connect(ptr_state->SockServerfd, (struct sockaddr*)&ptr_state->addr_server, sizeof(struct sockaddr)) != -1)
	{
		printf("\n####Connect to the distant server success!!####\n\n");
		return 1;
	}
	else
	{
		perror("####Connect to the distant server failed!!####\n");
		close(ptr_state->SockServerfd);
		return 0;
	}
	
	return 1;
}	
