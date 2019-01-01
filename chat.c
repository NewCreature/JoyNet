#include <stdio.h>
#include <string.h>
#include "joynet.h"
#include "message.h"
#include "serialize.h"

ENetPacket * joynet_create_chat_packet(JOYNET_SERIAL_DATA * dp, char * name, char * text)
{
	char data[1024] = {0};
	short name_length = strlen(name) + 1;
	short message_length = strlen(text) + 1;

	joynet_serialize(dp, data);
	joynet_putw(dp, name_length);
	joynet_putw(dp, message_length);
	joynet_write(dp, name, name_length);
	joynet_write(dp, text, message_length);

	return joynet_create_packet(JOYNET_CHAT_MESSAGE_TALK, dp);
}


void joynet_handle_server_chat_message(JOYNET_SERVER * sp, JOYNET_MESSAGE * mp)
{
	switch(mp->type)
	{
		case JOYNET_CHAT_MESSAGE_SET_NAME:
		{
			int client = joynet_get_client_from_peer(sp, mp->event->peer);
			ENetPacket * packet;

			/* send SET_NAME message back to client to finalize the change */
			strcpy(sp->client[client]->screen_name, mp->data);
			packet = joynet_build_packet(JOYNET_CHAT_MESSAGE_SET_NAME, mp->data, strlen(mp->data) + 1);
			enet_peer_send(sp->client[client]->peer, JOYNET_CHANNEL_CHAT, packet);
			break;
		}
		case JOYNET_CHAT_MESSAGE_SET_GROUP:
		{
			int client = joynet_get_client_from_peer(sp, mp->event->peer);
			ENetPacket * packet;

			joynet_serialize(sp->serial_data, mp->data);
			joynet_getw(sp->serial_data, &sp->client[client]->group);

			/* send SET_GROUP message back to client to finalize the change */
			packet = joynet_build_packet(JOYNET_CHAT_MESSAGE_SET_GROUP, mp->data, sizeof(short));
			enet_peer_send(sp->client[client]->peer, JOYNET_CHANNEL_CHAT, packet);
			break;
		}
		case JOYNET_CHAT_MESSAGE_TALK:
		case JOYNET_CHAT_MESSAGE_GROUP_TALK:
		{
			int client = joynet_get_client_from_peer(sp, mp->event->peer);
			ENetPacket * packet;
			int i;

			/* TALK message is different for client, contains user name and message */
			packet = joynet_create_chat_packet(sp->serial_data, sp->client[client]->screen_name, mp->data);
			for(i = 0; i < sp->max_clients; i++)
			{
				if((sp->client[i]->peer) && ((mp->type != JOYNET_CHAT_MESSAGE_GROUP_TALK) || (sp->client[i]->group == sp->client[client]->group)))
				{
					enet_peer_send(sp->client[i]->peer, JOYNET_CHANNEL_CHAT, packet);
				}
			}
			break;
		}
		case JOYNET_CHAT_MESSAGE_VOIP:
		case JOYNET_CHAT_MESSAGE_GROUP_VOIP:
		{
			int client = joynet_get_client_from_peer(sp, mp->event->peer);
			ENetPacket * packet;
			char data[4096] = {0};
			int i;

			/* send client number along with data so we can render audio to different streams */
			joynet_serialize(sp->serial_data, data);
			joynet_putw(sp->serial_data, client);
			joynet_write(sp->serial_data, mp->data, mp->data_size);
			packet = joynet_create_packet(JOYNET_CHAT_MESSAGE_VOIP, sp->serial_data);
			for(i = 0; i < sp->max_clients; i++)
			{
				if(i != client && sp->client[i]->peer && ((mp->type != JOYNET_CHAT_MESSAGE_GROUP_VOIP) || (sp->client[i]->group == sp->client[client]->group)))
				{
					enet_peer_send(sp->client[i]->peer, JOYNET_CHANNEL_CHAT, packet);
				}
			}
			break;
		}
	}
}

void joynet_handle_client_chat_message(JOYNET_CLIENT * cp, JOYNET_MESSAGE * mp)
{
	switch(mp->type)
	{
		case JOYNET_CHAT_MESSAGE_SET_NAME:
		{
			strcpy(cp->screen_name, mp->data);
			break;
		}
		case JOYNET_CHAT_MESSAGE_SET_GROUP:
		{
			joynet_serialize(cp->serial_data, mp->data);
			joynet_getw(cp->serial_data, &cp->group);
			break;
		}
		case JOYNET_CHAT_MESSAGE_TALK:
		case JOYNET_CHAT_MESSAGE_GROUP_TALK:
		{
			short name_length;
			short message_length;
			char user[128] = {0};
			char message[512] = {0};

			/* TALK message is different for client, contains user name and message */
			joynet_serialize(cp->serial_data, mp->data);
			joynet_getw(cp->serial_data, &name_length);
			joynet_getw(cp->serial_data, &message_length);
			joynet_read(cp->serial_data, user, name_length);
			joynet_read(cp->serial_data, message, message_length);
			if(cp->chat_callback)
			{
				cp->chat_callback(user, message);
			}
			break;
		}
		case JOYNET_CHAT_MESSAGE_VOIP:
		case JOYNET_CHAT_MESSAGE_GROUP_VOIP:
		{
			short client;
			char voip_data[4096] = {0};

			joynet_serialize(cp->serial_data, mp->data);
			joynet_getw(cp->serial_data, &client);
			joynet_read(cp->serial_data, voip_data, mp->data_size - sizeof(short));
			if(cp->voip_callback)
			{
				cp->voip_callback(client, voip_data, mp->data_size - sizeof(short));
			}
			break;
		}
	}
}
