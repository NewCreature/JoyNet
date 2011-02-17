#include <string.h>
#include "message.h"
#include "serialize.h"

void joynet_decode_message(JOYNET_MESSAGE * mp, ENetEvent * ep)
{
	mp->type = ep->packet->data[0];
	mp->data = ((char *)(ep->packet->data)) + 1;
	mp->data_size = ep->packet->dataLength - 1;
	mp->event = ep;
}

ENetPacket * joynet_create_packet(int type, JOYNET_SERIAL_DATA * dp)
{
	ENetPacket * pp = NULL;
	if(dp)
	{
		pp = enet_packet_create(dp->data, joynet_get_serial_size(dp) + 1, ENET_PACKET_FLAG_RELIABLE);
		memcpy(&(pp->data[1]), dp->data, joynet_get_serial_size(dp));
	}
	else
	{
		pp = enet_packet_create(NULL, 1, ENET_PACKET_FLAG_RELIABLE);
	}
	pp->data[0] = type;
	return pp;
}

ENetPacket * joynet_build_packet(int type, char * data, int data_size)
{
	ENetPacket * pp = NULL;
	if(data)
	{
		if(data_size > 0)
		{
			pp = enet_packet_create(data, data_size + 1, ENET_PACKET_FLAG_RELIABLE);
			memcpy(&(pp->data[1]), data, data_size);
		}
		else
		{
			pp = enet_packet_create(data, 1, ENET_PACKET_FLAG_RELIABLE);
		}
	}
	else
	{
		pp = enet_packet_create(data, 1, ENET_PACKET_FLAG_RELIABLE);
	}
	pp->data[0] = type;
	return pp;
}
