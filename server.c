#include <stdio.h>
#include <string.h>
#include "enet/include/enet/enet.h"
#include "joynet.h"
#include "serialize.h"
#include "message.h"
#include "chat.h"

/* server functions */
JOYNET_SERVER * joynet_create_server(void)
{
	JOYNET_SERVER * sp;
	int i;

	sp = malloc(sizeof(JOYNET_SERVER));
	if(!sp)
	{
		return NULL;
	}
	sp->serial_data = joynet_create_serial_data();
	if(!sp->serial_data)
	{
		return NULL;
	}
	strcpy(sp->id, "");
	sp->client = NULL;
	sp->global_callback = NULL;
	sp->internal_callback = NULL;
	for(i = 0; i < JOYNET_MAX_CHANNELS; i++)
	{
		sp->channel_callback[i] = NULL;
	}
	return sp;
}

void joynet_destroy_server(JOYNET_SERVER * sp)
{
	if(sp)
	{
		joynet_destroy_serial_data(sp->serial_data);
		free(sp);
	}
}

/* open a server */
int joynet_open_server(JOYNET_SERVER * sp, int port, int connections)
{
	int i;

	sp->address.host = ENET_HOST_ANY;
	sp->address.port = port;
	sp->host = enet_host_create(&sp->address, connections, 0, 0, 0);
	if(!sp->host)
	{
		return 0;
	}
	sp->client = malloc(sizeof(JOYNET_CLIENT *) * connections);
	for(i = 0; i < connections; i++)
	{
		sp->client[i] = joynet_create_client();
		if(!sp->client[i])
		{
			return 0;
		}
	}
	sp->max_clients = connections;
	return 1;
}

/* close a server, disconnecting all clients and freeing up memory used by them */
void joynet_close_server(JOYNET_SERVER * sp)
{
	int i;

	for(i = 0; i < sp->max_clients; i++)
	{
		if(sp->client[i]->peer)
		{
			enet_peer_disconnect(sp->client[i]->peer, JOYNET_DISCONNECT_SERVER_CLOSED);
		}
	}
	enet_host_flush(sp->host);
	for(i = 0; i < sp->max_clients; i++)
	{
		joynet_destroy_client(sp->client[i]);
	}
	free(sp->client);
	enet_host_destroy(sp->host);
}

void joynet_set_server_id(JOYNET_SERVER * sp, char * id)
{
	strcpy(sp->id, id);
}

static int joynet_server_find_free_client(JOYNET_SERVER * sp)
{
	int i;

	for(i = 0; i < sp->max_clients; i++)
	{
		if(!sp->client[i]->peer)
		{
			return i;
		}
	}
	return -1;
}

void joynet_poll_server(JOYNET_SERVER * sp)
{
	if(sp && sp->host)
	{
		while(enet_host_service(sp->host, &sp->event, 0))
		{
			switch(sp->event.type)
			{
				/* a connection has been received */
				case ENET_EVENT_TYPE_CONNECT:
				{
					int client = joynet_server_find_free_client(sp);

					if(client >= 0)
					{
						sp->client[client]->peer = sp->event.peer;
					}
					sp->client[client]->peer->data = malloc(sizeof(int));
					if(!sp->client[client]->peer->data)
					{
						/* need error system */
					}
					else
					{
						joynet_serialize(sp->serial_data, sp->client[client]->peer->data);
						joynet_putl(sp->serial_data, client);
					}
					sp->client[client]->user = client;
					strcpy(sp->client[client]->screen_name, "");
					sp->client[client]->master = 0;
					sp->client[client]->playing = 0;
					sp->client[client]->spectating = 0;
					break;
				}

				/* client disconnected */
				case ENET_EVENT_TYPE_DISCONNECT:
				{
					int client = joynet_get_client_from_peer(sp, sp->event.peer);
					sp->client[client]->peer = NULL;
					break;
				}

				/* packet received */
				case ENET_EVENT_TYPE_RECEIVE:
				{
					JOYNET_MESSAGE message;
					joynet_decode_message(&message, &sp->event);
					switch(sp->event.channelID)
					{
						case JOYNET_CHANNEL_SYSTEM:
						{
							break;
						}
						case JOYNET_CHANNEL_CHAT:
						{
							joynet_handle_server_chat_message(sp, &message);
							break;
						}
						case JOYNET_CHANNEL_GAME:
						{
							joynet_handle_server_game_message(sp, &message);
							break;
						}
					}

					if(sp->channel_callback[sp->event.channelID])
					{
						sp->channel_callback[sp->event.channelID](&message);
					}
					break;
				}
				default:
				{
					break;
				}
			}
			if(sp->internal_callback)
			{
				sp->internal_callback(&sp->event);
			}
			if(sp->global_callback)
			{
				sp->global_callback(&sp->event);
			}
		}
	}
}

int joynet_get_client_from_peer(JOYNET_SERVER * sp, ENetPeer * pp)
{
	int32_t client;

	joynet_serialize(sp->serial_data, pp->data);
	joynet_getl(sp->serial_data, &client);

	return client;
}

void joynet_set_server_channel_callback(JOYNET_SERVER * sp, int channel, int(*callback)(JOYNET_MESSAGE * mp))
{
	sp->channel_callback[channel] = callback;
}

void joynet_set_server_global_callback(JOYNET_SERVER * sp, int(*callback)(ENetEvent * ep))
{
	sp->global_callback = callback;
}
