#include <string.h>
#include <stdio.h>
#include "enet-1.2/include/enet/enet.h"
#include "joynet.h"
#include "serialize.h"
#include "message.h"
#include "chat.h"
#include "game_message.h"

/* client functions */
JOYNET_CLIENT * joynet_create_client(void)
{
	JOYNET_CLIENT * cp;
	int i;
	
	cp = malloc(sizeof(JOYNET_CLIENT));
	if(!cp)
	{
		return NULL;
	}
	cp->serial_data = joynet_create_serial_data();
	if(!cp->serial_data)
	{
		return NULL;
	}
	cp->host = NULL;
	cp->peer = NULL;
	strcpy(cp->screen_name, "");
	cp->user = -1; // -1 means not connected
	cp->group = 0;
	cp->master = 0;
	cp->playing = 0;
	cp->spectating = 0;
	cp->global_callback = NULL;
	for(i = 0; i < JOYNET_MAX_CHANNELS; i++)
	{
		cp->channel_callback[i] = NULL;
	}
	return cp;
}

void joynet_destroy_client(JOYNET_CLIENT * cp)
{
	joynet_destroy_serial_data(cp->serial_data);
	free(cp);
}

int joynet_connect_client_to_server(JOYNET_CLIENT * cp, char * host, int port)
{
	ENetEvent event;
	
	if(enet_address_set_host(&cp->address, host) < 0)
	{
		return 0;
	}
	cp->address.port = port;
	cp->host = enet_host_create(NULL, 1, 0, 0);
	if(!cp->host)
	{
		return 0;
	}
	cp->peer = enet_host_connect(cp->host, &cp->address, 4);
	if(!cp->peer)
	{
		return 0;
	}
	if(enet_host_service(cp->host, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
    {
		return 1;
    }
    else
    {
        enet_peer_reset(cp->peer);
    }
	return 0;
}

void joynet_disconnect_client_from_server(JOYNET_CLIENT * cp)
{
	if(cp->peer)
	{
		enet_peer_disconnect(cp->peer, JOYNET_DISCONNECT_CLIENT_CLOSED);
		enet_host_flush(cp->host);
		enet_host_destroy(cp->host);
	}
}

void joynet_set_client_group(JOYNET_CLIENT * cp, short group)
{
	char data[2];
	ENetPacket * pp;
	
	if(cp->peer)
	{
		joynet_serialize(cp->serial_data, data);
		joynet_putw(cp->serial_data, group);
		pp = joynet_create_packet(JOYNET_CHAT_MESSAGE_SET_GROUP, cp->serial_data);
		enet_peer_send(cp->peer, JOYNET_CHANNEL_CHAT, pp);
	}
}

void joynet_set_client_screen_name(JOYNET_CLIENT * cp, char * screen_name)
{
	if(cp->peer)
	{
		ENetPacket * pp = joynet_build_packet(JOYNET_CHAT_MESSAGE_SET_NAME, screen_name, strlen(screen_name) + 1);
		enet_peer_send(cp->peer, JOYNET_CHANNEL_CHAT, pp);
	}
}

void joynet_poll_client(JOYNET_CLIENT * cp)
{
	ENetEvent event;
	
	if(cp && cp->host && cp->peer)
	{
		while(enet_host_service(cp->host, &event, 0))
		{
			switch(event.type)
			{
				/* data has been received */
				case ENET_EVENT_TYPE_RECEIVE:
				{
					JOYNET_MESSAGE message;
					joynet_decode_message(&message, &event);
				
					switch(event.channelID)
					{
						case JOYNET_CHANNEL_SYSTEM:
						{
							break;
						}
						case JOYNET_CHANNEL_CHAT:
						{
							joynet_handle_client_chat_message(cp, &message);
							break;
						}
						case JOYNET_CHANNEL_GAME:
						{
							joynet_handle_client_game_message(cp, &message);
							break;
						}
					}
					if(cp->channel_callback[event.channelID])
					{
						cp->channel_callback[event.channelID](&message);
					}
					break;
				}
			   
				case ENET_EVENT_TYPE_DISCONNECT:
				{
					break;
				}
				default:
				{
					break;
				}
			}
			if(cp->global_callback)
			{
				cp->global_callback(&event);
			}
		}
	}
}

void joynet_send_client_chat(JOYNET_CLIENT * cp, char * message, int group)
{
	if(cp->peer)
	{
		ENetPacket * packet;
		if(group)
		{
			packet = joynet_build_packet(JOYNET_CHAT_MESSAGE_GROUP_TALK, message, strlen(message) + 1);
		}
		else
		{
			packet = joynet_build_packet(JOYNET_CHAT_MESSAGE_TALK, message, strlen(message) + 1);
		}
		enet_peer_send(cp->peer, JOYNET_CHANNEL_CHAT, packet);
		enet_host_flush(cp->host);
	}
}

void joynet_set_client_global_callback(JOYNET_CLIENT * cp, int(*callback)(ENetEvent * ep))
{
	cp->global_callback = callback;
}

void joynet_set_client_channel_callback(JOYNET_CLIENT * cp, int channel, int(*callback)(JOYNET_MESSAGE * mp))
{
	cp->channel_callback[channel] = callback;
}

void joynet_set_client_chat_callback(JOYNET_CLIENT * cp, int(*callback)(char * user, char * message))
{
	cp->chat_callback = callback;
}
