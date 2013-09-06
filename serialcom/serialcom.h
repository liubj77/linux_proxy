/***********************/
#ifndef SERIELCOM_H
#define SERIELCOM_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include "hdlc.h"

struct serial_state_t  //serial communication status variables
{
	int32_t SockServerfd;  //server socket fd
	struct sockaddr_in addr_local,addr_server;
	int32_t comfd;       //serial port fd;
	uint8_t clientseq;
	uint8_t commandtype;
	uint8_t commandack;
};

static int8_t hdlc_q_byte(struct hdlc_buff_t* lm, uint8_t byte, uint16_t max_size);

#endif
