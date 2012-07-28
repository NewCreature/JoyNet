#ifndef JOYNET_MESSAGE_H
#define JOYNET_MESSAGE_H

#include "enet-1.3.1/include/enet/enet.h"
#include "joynet.h"

void joynet_decode_message(JOYNET_MESSAGE * mp, ENetEvent * ep);
ENetPacket * joynet_create_packet(int type, JOYNET_SERIAL_DATA * dp);
ENetPacket * joynet_build_packet(int type, char * data, int data_size);

#endif
