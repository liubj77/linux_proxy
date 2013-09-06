#ifndef HDLC_H
#define HDLC_H

#include <stdint.h>

#define HDLC_FLAG 	 	0x7e
#define HDLC_SPACE   	0x7d
#define HDLC_MASK   	0x20

#define MAX_PACKET_BYTE_SIZE_HDLC 256

/**** define hdlc packet struct****/
struct hdlc_buff_t
{
    uint16_t    len;
    uint16_t    idx;
    uint8_t     data[MAX_PACKET_BYTE_SIZE_HDLC];
};

int8_t hdlc_pkdecode(uint8_t* buf_src, uint16_t size_src, 
                          uint8_t* buf_dest, uint16_t size_of_dest,
                          uint16_t* bytes_written);

int8_t hdlc_pkgen(uint8_t* prefix, uint16_t size,
                       uint8_t* buf, uint16_t size_buf, 
                       uint8_t* buf_dest, uint16_t buf_dest_size,
                       uint16_t* bytes_written);


#endif
