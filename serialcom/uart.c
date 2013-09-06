
#include "uart.h"
#include <stdio.h>      //标准输入输出定义
//#include <stdlib.h>     //标准函数库定义
#include <unistd.h>     //Unix标准函数定义
#include <fcntl.h>
#include <termios.h>    //POSIX中断控制定义
#include <errno.h>      //错误号定义

int32_t open_uart(char *Dev)
{
	int32_t fd=open(Dev,O_RDWR|O_NOCTTY|O_NDELAY);
	if(-1==fd)
	{
		perror("open serail port");
		return -1;
	}
	/*if(fcntl(fd,F_SETFL,0)<0)
	  {
		perror("fcntl F_SETFL\n");
	  }*/
	if(isatty(fd)==0)
	{
		perror("This is not a terminal device");
	}
	return fd;
		
}


int8_t config_uart(int32_t fd,int32_t baud_rate,int32_t data_bits,int32_t stop_bits,uint8_t parity)
{
	struct termios termios_p,termios_save;
	int32_t speed;
	if(tcgetattr(fd,&termios_save)!=0)
	{
		perror("tcgetattr");
		return -1;
	}

	termios_p = termios_save;
	cfmakeraw(&termios_p);
	
	/* termios_p.c_cflag |= (CLOCAL|CREAD); */
	termios_p.c_cflag &= ~CSIZE;
	switch(data_bits)
	{
		case 7:
			termios_p.c_cflag |= CS7;
			break;
		case 8:
			termios_p.c_cflag |= CS8;
			break;
		default:
			termios_p.c_cflag |= CS8;
			break;
	}
	switch(parity)
	{
		case 'n':
		case 'N':
			termios_p.c_cflag &= ~PARENB;
			termios_p.c_iflag &= ~INPCK;
			break;
		case 'o':
		case 'O':
			termios_p.c_cflag |= (PARENB | PARODD);
			termios_p.c_iflag |= INPCK;
			break;
		case 'e':
		case 'E':
			termios_p.c_cflag |= PARENB;
			termios_p.c_cflag &= ~PARODD;    //偶校验
			termios_p.c_iflag |= INPCK;
			break;
		case 's':
		case 'S':
			termios_p.c_cflag &= ~PARENB;
			termios_p.c_cflag &= ~CSTOPB;
			termios_p.c_iflag |= INPCK;
			break;
		default:
			termios_p.c_cflag &= ~PARENB;
			termios_p.c_iflag &= ~INPCK;
			break;
	}
	switch(baud_rate)
	{
		case 9600:
			speed = B9600;
			break;
		case 19200:
			speed = B19200;
			break;
		case 38400:
			speed = B38400;
			break;
		case 57600:
			speed = B57600;
			break;
		case 115200:
			speed = B115200;
			break;
		default:
			speed = B115200;
			break;
	}
	cfsetispeed(&termios_p, B115200);  
	cfsetospeed(&termios_p, B115200);
	switch(stop_bits)
	{
		case 1:
			termios_p.c_cflag &= ~CSTOPB;
			break;
		case 2:
			termios_p.c_cflag |= CSTOPB;
			break;
		default:
			break;
	}

#if 0	
	termios_p.c_cflag |= (CLOCAL|CREAD);
	termios_p.c_iflag = IGNPAR | IGNBRK;
    termios_p.c_oflag &= ~(ONLCR | OCRNL);    
    termios_p.c_iflag &= ~(ICRNL | INLCR);
    termios_p.c_iflag &= ~(IXON | IXOFF | IXANY);     //Disable Software Flow Control
    termios_p.c_cflag &= ~CRTSCTS;                    //Disable hardware flow control
    
  	termios_p.c_lflag &= 0;//~(ICANON | ECHO | ECHOE | ISIG);
	termios_p.c_oflag &= 0;//~OPOST;
#endif
    termios_p.c_cc[VTIME] = 0;        //设置超时
    termios_p.c_cc[VMIN] = 0; 
    
    //Update the Opt and do it now

	tcflush(fd, TCIFLUSH);
	if((tcsetattr(fd,TCSANOW,&termios_p))!=0)
	{
       perror("tcsetattr error");
       return -1;
	}
	printf("Configure UART done.\n");
    return 0;
}


