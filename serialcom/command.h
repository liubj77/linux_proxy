/*****************************/
#ifndef COMMAND_H
#define COMMAND_H

uint8_t hello[]={0x7e, 0x0, 0x1, 0x0, 0x3, 0x4, 0x0, 0x0, 0xf7, 0xce, 0x7e};
uint8_t subscribe[]={0x2, 0x16, 0x1, 0x8, 0x0, 0x0, 0x0, 0x12,0x0,0x0,0x0,0x12};  //receive just motes' data notification 
uint8_t ackNotification[]={0x3,0x14,0x1,0x1,0x0};

#endif
