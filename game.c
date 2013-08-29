#include <stdio.h>
#include <string.h>
#include "joynet.h"
#include "serialize.h"
#include "message.h"
#include "game.h"
#include "game_message.h"

JOYNET_GAME * joynet_current_game = NULL;
JOYNET_GAME * joynet_current_server_game = NULL;

/* sort controllers according to port */
int joynet_qsort_controllers(const void * e1, const void * e2)
{
    JOYNET_CONTROLLER_SORT_DATA * thing1 = (JOYNET_CONTROLLER_SORT_DATA *)e1;
    JOYNET_CONTROLLER_SORT_DATA * thing2 = (JOYNET_CONTROLLER_SORT_DATA *)e2;

    if(thing1->port < 0)
    {
		return 1;
	}
    else if(thing1->port < thing2->port)
	{
        return -1;
    }
    if(thing1->port > thing2->port)
    {
        return 1;
    }
    return 0;
}

void joynet_reset_game(JOYNET_GAME * gp)
{
	int i;
	
	for(i = 0; i < gp->players; i++)
	{
		if(gp->player[i]->playing)
		{
			gp->player[i]->playing = 0;
		}
		strcpy(gp->player[i]->name, "");
		memset(gp->player[i]->selected_content, 0, sizeof(unsigned long) * JOYNET_GAME_MAX_CONTENT_LISTS);
		memset(gp->player[i]->selected_content_index, 0, sizeof(int) * JOYNET_GAME_MAX_CONTENT_LISTS);
	}
	for(i = 0; i < gp->controllers; i++)
	{
		if(gp->type == JOYNET_GAME_TYPE_CONTROLLERS)
		{
			gp->controller[i]->port = -1;
		}
		else if(gp->type == JOYNET_GAME_TYPE_MOUSE)
		{
			gp->mouse[i]->port = -1;
		}
	}
	gp->player_count = 0;
}

/* don't pass a client if you want local play */
JOYNET_GAME * joynet_create_game(char * name, int type, int max_players, int max_controllers, int(*callback)(JOYNET_MESSAGE * mp))
{
	JOYNET_GAME * gp;
	int i;
	
	gp = malloc(sizeof(JOYNET_GAME));
	if(!gp)
	{
		return NULL;
	}
	memset(gp, 0, sizeof(JOYNET_GAME));
	gp->type = type;
	strcpy(gp->name, name);
	gp->state = JOYNET_GAME_STATE_OFF;
	gp->callback = callback;
	for(i = 0; i < max_players; i++)
	{
		gp->player[i] = malloc(sizeof(JOYNET_PLAYER));
		if(!gp->player[i])
		{
			return NULL;
		}
		memset(gp->player[i], 0, sizeof(JOYNET_PLAYER));
		if(type == JOYNET_GAME_TYPE_CONTROLLERS)
		{
			gp->player_controller[i] = malloc(sizeof(JOYNET_CONTROLLER)); // allocated at connect
			if(!gp->player_controller[i])
			{
				return NULL;
			}
			gp->player_controller[i]->port = -1; // not connected yet
		}
		else if(type == JOYNET_GAME_TYPE_MOUSE)
		{
			gp->player_mouse[i] = malloc(sizeof(JOYNET_MOUSE));
			if(!gp->player_mouse[i])
			{
				return NULL;
			}
			gp->player_mouse[i]->port = -1;
		}
		gp->player[i]->options = 0;
		strcpy(gp->player[i]->name, "");
		memset(gp->player[i]->selected_content, 0, sizeof(unsigned long) * JOYNET_GAME_MAX_CONTENT_LISTS);
		memset(gp->player[i]->selected_content_index, 0, sizeof(int) * JOYNET_GAME_MAX_CONTENT_LISTS);
	}
	gp->players = max_players;
	for(i = 0; i < max_controllers; i++)
	{
		if(type == JOYNET_GAME_TYPE_CONTROLLERS)
		{
			gp->controller[i] = malloc(sizeof(JOYNET_CONTROLLER));
			if(!gp->controller[i])
			{
				return NULL;
			}
			gp->controller[i]->port = -1; // not connected yet
		}
		else
		{
			gp->mouse[i] = malloc(sizeof(JOYNET_MOUSE));
			if(!gp->mouse[i])
			{
				return NULL;
			}
			gp->mouse[i]->port = -1;
		}
	}
	gp->serial_data = joynet_create_serial_data();
	if(!gp->serial_data)
	{
		return NULL;
	}
	gp->controllers = max_controllers;
	gp->options = 0;
	gp->client = NULL;
	gp->input_buffer = NULL;
	gp->player_count = 0;
	return gp;
}

void joynet_setup_game_controllers(JOYNET_GAME * gp, int buttons, int axes, int buffer_frames)
{
	gp->controller_buttons = buttons;
	gp->controller_axes = axes;
	gp->max_buffer_frames = buffer_frames;
}

void joynet_destroy_game(JOYNET_GAME * gp)
{
	int i;
	
	joynet_destroy_serial_data(gp->serial_data);
	for(i = 0; i < gp->players; i++)
	{
		if(gp->player[i])
		{
			free(gp->player[i]);
		}
		if(gp->player_controller[i])
		{
			free(gp->player_controller[i]);
		}
		if(gp->player_mouse[i])
		{
			free(gp->player_mouse[i]);
		}
	}
	for(i = 0; i < gp->controllers; i++)
	{
		if(gp->controller[i])
		{
			free(gp->controller[i]);
		}
		if(gp->mouse[i])
		{
			free(gp->mouse[i]);
		}
	}
	free(gp);
}

void joynet_select_game(JOYNET_GAME * gp)
{
	joynet_current_game = gp;
}

int joynet_open_game_server(JOYNET_GAME * gp, JOYNET_SERVER * sp, int port, int clients)
{
	joynet_current_server_game = gp;
	if(joynet_current_server_game)
	{
		if(!joynet_open_server(sp, port, clients))
		{
			return 0;
		}
	}
	sp->internal_callback = joynet_server_internal_game_callback;
	gp->server = sp;
	return 1;
}

int joynet_connect_to_game_server(JOYNET_GAME * gp, JOYNET_CLIENT * cp, char * address, int port)
{
	char data[256];
	ENetPacket * pp;
	
	if(joynet_connect_client_to_server(cp, address, port))
	{
		joynet_serialize(cp->serial_data, data);
		joynet_putw(cp->serial_data, strlen(gp->name) + 1);
		joynet_write(cp->serial_data, gp->name, strlen(gp->name) + 1);
		pp = joynet_create_packet(JOYNET_GAME_MESSAGE_CHECK_ID, cp->serial_data);
		enet_peer_send(cp->peer, JOYNET_CHANNEL_GAME, pp);
		joynet_reset_game(gp);
		joynet_current_game = gp;
		return 1;
	}
	return 0;
}

void joynet_disconnect_from_game_server(JOYNET_GAME * gp, JOYNET_CLIENT * cp)
{
	joynet_disconnect_client_from_server(cp);
	gp->client = NULL;
	joynet_reset_game(gp);
}

void joynet_connect_to_game(JOYNET_GAME * gp, short controller, short player)
{
	char data[4] = {0};
	ENetPacket * pp;
	int assigned_player = player;
	int i;
	
	if(gp->client)
	{
		joynet_serialize(gp->client->serial_data, data);
		joynet_putw(gp->client->serial_data, controller);
		joynet_putw(gp->client->serial_data, player);
		pp = joynet_create_packet(JOYNET_GAME_MESSAGE_CONNECT, gp->client->serial_data);
		enet_peer_send(gp->client->peer, JOYNET_CHANNEL_GAME, pp);
	}
	else
	{
		if(player < 0)
		{
			for(i = 0; i < gp->players; i++)
			{
				if(!gp->player[i]->playing)
				{
					assigned_player = i;
					break;
				}
			}
		}
		if(assigned_player >= 0)
		{
			if(!gp->player[assigned_player]->playing)
			{
				gp->player[assigned_player]->playing = 1;
				if(gp->type == JOYNET_GAME_TYPE_CONTROLLERS)
				{
					gp->player_controller[assigned_player]->port = assigned_player;
				}
				else if(gp->type == JOYNET_GAME_TYPE_MOUSE)
				{
					gp->player_mouse[assigned_player]->port = assigned_player;
				}
				gp->player[assigned_player]->local = 1;
				gp->player_count++;
				if(gp->type == JOYNET_GAME_TYPE_CONTROLLERS)
				{
					gp->controller[controller]->port = assigned_player;
				}
				else if(gp->type == JOYNET_GAME_TYPE_MOUSE)
				{
					gp->mouse[controller]->port = assigned_player;
				}
				
				/* reset selections to prevent bugs */
				memset(gp->player[assigned_player]->selected_content, 0, sizeof(unsigned long) * JOYNET_GAME_MAX_CONTENT_LISTS);
				memset(gp->player[assigned_player]->selected_content_index, 0, sizeof(int) * JOYNET_GAME_MAX_CONTENT_LISTS);			
				if(gp->callback)
				{
					JOYNET_MESSAGE message;
					
					/* send connect message */
					message.type = JOYNET_GAME_MESSAGE_CONNECT;
					joynet_serialize(gp->serial_data, data);
					joynet_putw(gp->serial_data, controller);
					joynet_putw(gp->serial_data, assigned_player);
					message.data = data;
					message.data_size = 4;
					message.event = NULL;
					gp->callback(&message);
					
					/* send add player message */
					message.type = JOYNET_GAME_MESSAGE_ADD_PLAYER;
					joynet_serialize(gp->serial_data, data);
					joynet_putw(gp->serial_data, assigned_player);
					joynet_putw(gp->serial_data, 2);
					joynet_write(gp->serial_data, "", 2);
					message.data = data;
					message.data_size = 6;
					message.event = NULL;
					gp->callback(&message);
				}
			}
		}
	}
}

void joynet_disconnect_from_game(JOYNET_GAME * gp, short controller, short player)
{
	char data[4] = {0};
	ENetPacket * pp;
	
	if(gp->client)
	{
		joynet_serialize(gp->client->serial_data, data);
		joynet_putw(gp->client->serial_data, controller);
		if(player < 0)
		{
			if(gp->type == JOYNET_GAME_TYPE_CONTROLLERS)
			{
				joynet_putw(gp->client->serial_data, gp->controller[controller]->port);
			}
			else if(gp->type == JOYNET_GAME_TYPE_MOUSE)
			{
				joynet_putw(gp->client->serial_data, gp->mouse[controller]->port);
			}
		}
		else
		{
			joynet_putw(gp->client->serial_data, player);
		}
		pp = joynet_create_packet(JOYNET_GAME_MESSAGE_DISCONNECT, gp->client->serial_data);
		enet_peer_send(gp->client->peer, JOYNET_CHANNEL_GAME, pp);
	}
	else
	{
		JOYNET_MESSAGE message;
		
		gp->player[player]->playing = 0;
		if(gp->type == JOYNET_GAME_TYPE_CONTROLLERS)
		{
			gp->controller[controller]->port = -1;
		}
		else if(gp->type == JOYNET_GAME_TYPE_MOUSE)
		{
			gp->mouse[controller]->port = -1;
		}
		gp->player_count--;
		if(gp->callback)
		{
			
			/* disconnect player */
			joynet_serialize(gp->serial_data, data);
			joynet_putw(gp->serial_data, controller);
			joynet_putw(gp->serial_data, player);
			message.type = JOYNET_GAME_MESSAGE_DISCONNECT;
			message.data = data;
			message.data_size = joynet_get_serial_size(gp->serial_data);
			message.event = NULL;
			gp->callback(&message);
			
			/* remove player */
			joynet_serialize(gp->serial_data, data);
			joynet_putw(gp->serial_data, player);
			message.type = JOYNET_GAME_MESSAGE_REMOVE_PLAYER;
			message.data = data;
			message.data_size = joynet_get_serial_size(gp->serial_data);
			message.event = NULL;
			gp->callback(&message);
		}
	}
}

void joynet_watch_game(JOYNET_GAME * gp)
{
	ENetPacket * pp;
	if(gp->client)
	{
		pp = joynet_create_packet(JOYNET_GAME_MESSAGE_SPECTATE, NULL);
		enet_peer_send(gp->client->peer, JOYNET_CHANNEL_GAME, pp);
	}
	else
	{
	}
}

void joynet_leave_game(JOYNET_GAME * gp)
{
	ENetPacket * pp;
	if(gp->client)
	{
		pp = joynet_create_packet(JOYNET_GAME_MESSAGE_LEAVE, NULL);
		enet_peer_send(gp->client->peer, JOYNET_CHANNEL_GAME, pp);
	}
	else
	{
	}
}

void joynet_add_game_option(JOYNET_GAME * gp, int * op)
{
	gp->option[gp->options] = op;
	gp->options++;
}

void joynet_add_player_option(JOYNET_GAME * gp, int player, int * op)
{
	gp->player[player]->option[gp->player[player]->options] = op;
	gp->player[player]->options++;
}

/* high level functions */
void joynet_update_game_option(JOYNET_GAME * gp, int * option)
{
	char data[1024] = {0};
	ENetPacket * pp;
	int i, c = 0;
	
	if(gp->client && gp->client->master)
	{
		for(i = 0; i < gp->options; i++)
		{
			if(gp->option[i] == option)
			{
				c = i;
				break;
			}
		}
		if(i < gp->options)
		{
			joynet_serialize(gp->client->serial_data, data);
			joynet_putw(gp->client->serial_data, c);
			joynet_putl(gp->client->serial_data, *gp->option[c]);
			pp = joynet_create_packet(JOYNET_GAME_MESSAGE_UPDATE_OPTION, gp->client->serial_data);
			enet_peer_send(gp->client->peer, JOYNET_CHANNEL_GAME, pp);
		}
	}
	else
	{
	}
}

void joynet_update_game_options(JOYNET_GAME * gp)
{
	char data[1024] = {0};
	ENetPacket * pp;
	int i;
	
	if(gp->client && gp->client->master)
	{
		joynet_serialize(gp->client->serial_data, data);
		joynet_putw(gp->client->serial_data, gp->options);
		for(i = 0; i < gp->options; i++)
		{
			joynet_putl(gp->client->serial_data, *gp->option[i]);
		}
		pp = joynet_create_packet(JOYNET_GAME_MESSAGE_UPDATE_OPTIONS, gp->client->serial_data);
		enet_peer_send(gp->client->peer, JOYNET_CHANNEL_GAME, pp);
	}
	else
	{
	}
}

/* high level functions */
void joynet_update_player_options(JOYNET_GAME * gp, int player)
{
	char data[1024] = {0};
	ENetPacket * pp;
	int i;
	
	if(gp->client && gp->player[player]->local)
	{
		joynet_serialize(gp->client->serial_data, data);
		joynet_putw(gp->client->serial_data, player);
		joynet_putw(gp->client->serial_data, strlen(gp->player[player]->name) + 1);
		joynet_write(gp->client->serial_data, gp->player[player]->name, strlen(gp->player[player]->name) + 1);
		joynet_putw(gp->client->serial_data, gp->player[player]->options);
		for(i = 0; i < gp->player[player]->options; i++)
		{
			joynet_putl(gp->client->serial_data, *gp->player[player]->option[i]);
		}
		pp = joynet_create_packet(JOYNET_GAME_MESSAGE_UPDATE_PLAYER_OPTIONS, gp->client->serial_data);
		enet_peer_send(gp->client->peer, JOYNET_CHANNEL_GAME, pp);
	}
	else
	{
		JOYNET_MESSAGE message;
		if(gp->callback)
		{
			joynet_serialize(gp->serial_data, data);
			joynet_putw(gp->serial_data, player);
			joynet_putw(gp->serial_data, strlen(gp->player[player]->name) + 1);
			joynet_write(gp->serial_data, gp->player[player]->name, strlen(gp->player[player]->name) + 1);
			joynet_putw(gp->serial_data, gp->player[player]->options);
			for(i = 0; i < gp->player[player]->options; i++)
			{
				joynet_putl(gp->serial_data, *gp->player[player]->option[i]);
			}
			message.type = JOYNET_GAME_MESSAGE_UPDATE_PLAYER_OPTIONS;
			message.data = data;
			message.data_size = joynet_get_serial_size(gp->serial_data);
			message.event = NULL;
			gp->callback(&message);
		}
	}
}

void joynet_reset_game_content(JOYNET_GAME * gp, int list)
{
	if(gp->local_content_list[list])
	{
		gp->local_content_list[list]->count = 0;
	}
	if(gp->content_list[list])
	{
		gp->content_list[list]->count = 0;
	}
}

/* add content to the specified (local) list
 * content list uploaded on player connection
 * content list should mirror local content list initially */
void joynet_add_game_content(JOYNET_GAME * gp, int list, unsigned long hash)
{
	if(!gp->local_content_list[list])
	{
		gp->local_content_list[list] = malloc(sizeof(JOYNET_CONTENT_LIST));
		if(!gp->local_content_list[list])
		{
			return;
		}
		gp->local_content_list[list]->count = 0;
	}
	gp->local_content_list[list]->hash[gp->local_content_list[list]->count] = hash;
	gp->local_content_list[list]->count++;
	if(!gp->content_list[list])
	{
		gp->content_list[list] = malloc(sizeof(JOYNET_CONTENT_LIST));
		if(!gp->content_list[list])
		{
			return;
		}
		gp->content_list[list]->count = 0;
	}
	gp->content_list[list]->hash[gp->content_list[list]->count] = hash;
	gp->content_list[list]->count++;
}

void joynet_select_game_content(JOYNET_GAME * gp, int player, int list, unsigned long hash)
{
	char data[6];
	ENetPacket * pp;
	int i;
	
	if(gp->client)
	{
		joynet_serialize(gp->client->serial_data, data);
		joynet_putw(gp->client->serial_data, player);
		joynet_putw(gp->client->serial_data, list);
		joynet_putl(gp->client->serial_data, hash);
		pp = joynet_create_packet(JOYNET_GAME_MESSAGE_SELECT_PLAYER_CONTENT, gp->client->serial_data);
		enet_peer_send(gp->client->peer, JOYNET_CHANNEL_GAME, pp);
	}
	else
	{
		JOYNET_MESSAGE message;
		
		gp->player[player]->selected_content[list] = hash;
		for(i = 0; i < gp->local_content_list[list]->count; i++)
		{
			if(gp->local_content_list[list]->hash[i] == hash)
			{
				gp->player[player]->selected_content_index[list] = i;
				break;
			}
		}
		if(gp->callback)
		{
			joynet_serialize(gp->serial_data, data);
			joynet_putw(gp->serial_data, player);
			joynet_putw(gp->serial_data, list);
			joynet_putl(gp->serial_data, hash);
			message.type = JOYNET_GAME_MESSAGE_SELECT_PLAYER_CONTENT;
			message.data = data;
			message.data_size = joynet_get_serial_size(gp->serial_data);
			message.event = NULL;
			gp->callback(&message);
		}
	}
}

void joynet_start_game(JOYNET_GAME * gp)
{
	ENetPacket * pp;
	int i;
	
	if(gp->client)
	{
		if(gp->client->master && gp->state == JOYNET_GAME_STATE_OFF)
		{
			joynet_update_game_options(gp);
			for(i = 0; i < gp->players; i++)
			{
				if(gp->player[i]->playing)
				{
					joynet_update_player_options(gp, i);
				}
			}
			pp = joynet_create_packet(JOYNET_GAME_MESSAGE_START, NULL);
			enet_peer_send(gp->client->peer, JOYNET_CHANNEL_GAME, pp);
		}
	}
	else
	{
		JOYNET_MESSAGE message;
		
		if(gp->state == JOYNET_GAME_STATE_OFF)
		{
			switch(gp->type)
			{
				case JOYNET_GAME_TYPE_MOUSE:
				{
					gp->input_buffer = malloc(sizeof(JOYNET_INPUT_BUFFER));
					if(gp->input_buffer)
					{
						gp->input_buffer->frame_size = gp->controller_axes * 2 + (gp->controller_buttons > 0 ? 1 : 0);
						gp->input_buffer->data = malloc(gp->input_buffer->frame_size * gp->max_buffer_frames);
					}
					break;
				}
				case JOYNET_GAME_TYPE_CONTROLLERS:
				{
					
					/* create input buffer */
					gp->input_buffer = malloc(sizeof(JOYNET_INPUT_BUFFER));
					if(gp->input_buffer)
					{
						int bsize = 0;
						if(gp->controller_buttons > 0)
						{
							bsize++;
						}
						if(gp->controller_buttons > 8)
						{
							bsize++;
						}
						gp->input_buffer->frame_size = gp->player_count * (gp->controller_axes + bsize);
						gp->input_buffer->data = malloc(gp->input_buffer->frame_size * gp->max_buffer_frames);
					}
					
					if(gp->type == JOYNET_GAME_TYPE_CONTROLLERS)
					{
						for(i = 0; i < gp->controllers; i++)
						{
							gp->controller_sort_data[i].port = gp->controller[i]->port;
							gp->controller_sort_data[i].index = i;
						}
					}
					else if(gp->type == JOYNET_GAME_TYPE_MOUSE)
					{
						for(i = 0; i < gp->controllers; i++)
						{
							gp->controller_sort_data[i].port = gp->mouse[i]->port;
							gp->controller_sort_data[i].index = i;
						}
					}
					qsort(gp->controller_sort_data, gp->controllers, sizeof(JOYNET_CONTROLLER_SORT_DATA), joynet_qsort_controllers);
					break;
				}
			}
			gp->input_buffer->frames = 0;
			gp->input_buffer->read_pos = 0;
			gp->input_buffer->write_pos = 0;
			gp->input_buffer->previous_write_pos = 0;
			gp->input_buffer->filled_frames = 0;
			gp->input_buffer->read_frames = 0;
			gp->state = JOYNET_GAME_STATE_PLAYING;
			if(gp->callback)
			{
				message.type = JOYNET_GAME_MESSAGE_START;
				message.data = NULL;
				message.data_size = 0;
				message.event = NULL;
				gp->callback(&message);
			}
		}
	}
}

void joynet_pause_game(JOYNET_GAME * gp)
{
	ENetPacket * pp;
	if(gp->client)
	{
		if(gp->client->master && gp->state == JOYNET_GAME_STATE_PLAYING)
		{
			pp = joynet_create_packet(JOYNET_GAME_MESSAGE_PAUSE, NULL);
			enet_peer_send(gp->client->peer, JOYNET_CHANNEL_GAME, pp);
		}
	}
	else
	{
		JOYNET_MESSAGE message;
		
		if(gp->state == JOYNET_GAME_STATE_PLAYING)
		{
			gp->state = JOYNET_GAME_STATE_PAUSED;
			if(gp->callback)
			{
				message.type = JOYNET_GAME_MESSAGE_PAUSE;
				message.data = NULL;
				message.data_size = 0;
				message.event = NULL;
				gp->callback(&message);
			}
		}
	}
}

void joynet_resume_game(JOYNET_GAME * gp)
{
	ENetPacket * pp;
	if(gp->client)
	{
		if(gp->client->master && gp->state == JOYNET_GAME_STATE_PAUSED)
		{
			pp = joynet_create_packet(JOYNET_GAME_MESSAGE_RESUME, NULL);
			enet_peer_send(gp->client->peer, JOYNET_CHANNEL_GAME, pp);
		}
	}
	else
	{
		JOYNET_MESSAGE message;
		
		if(gp->state == JOYNET_GAME_STATE_PAUSED)
		{
			gp->state = JOYNET_GAME_STATE_PLAYING;
			if(gp->callback)
			{
				message.type = JOYNET_GAME_MESSAGE_RESUME;
				message.data = NULL;
				message.data_size = 0;
				message.event = NULL;
				gp->callback(&message);
			}
		}
	}
}

void joynet_end_game(JOYNET_GAME * gp)
{
	ENetPacket * pp;
	if(gp->client)
	{
		if(gp->client->master && (gp->state == JOYNET_GAME_STATE_PAUSED || gp->state == JOYNET_GAME_STATE_PLAYING))
		{
			pp = joynet_create_packet(JOYNET_GAME_MESSAGE_END, NULL);
			enet_peer_send(gp->client->peer, JOYNET_CHANNEL_GAME, pp);
		}
	}
	else
	{
		JOYNET_MESSAGE message;
		
		if(gp->state == JOYNET_GAME_STATE_PLAYING || gp->state == JOYNET_GAME_STATE_PAUSED)
		{
			gp->state = JOYNET_GAME_STATE_OFF;
			if(gp->callback)
			{
				message.type = JOYNET_GAME_MESSAGE_END;
				message.data = NULL;
				message.data_size = 0;
				message.event = NULL;
				gp->callback(&message);
			}
		}
	}
}

void joynet_select_player(JOYNET_GAME * gp, int player)
{
	char data[2];
	ENetPacket * pp;
	if(gp->client)
	{
		if(gp->client->master && gp->state == JOYNET_GAME_STATE_PLAYING)
		{
			joynet_serialize(gp->client->serial_data, data);
			joynet_putw(gp->client->serial_data, player);
			pp = joynet_create_packet(JOYNET_GAME_MESSAGE_SELECT_PLAYER, gp->client->serial_data);
			enet_peer_send(gp->client->peer, JOYNET_CHANNEL_GAME, pp);
		}
	}
	else
	{
		JOYNET_MESSAGE message;
		
		if(gp->callback)
		{
			message.type = JOYNET_GAME_MESSAGE_SELECT_PLAYER;
			message.data = NULL;
			message.data_size = 0;
			message.event = NULL;
			gp->callback(&message);
			gp->current_player = player;
		}
	}
}

/* construct one frame of input to be sent to the server */
int joynet_encode_game_input(JOYNET_GAME * gp, char * buffer)
{
	joynet_serialize(gp->serial_data, buffer);
	int i, j;
	
	switch(gp->type)
	{
		case JOYNET_GAME_TYPE_MOUSE:
		{
			
			/* see if any of the local players are the current player */
			if(gp->controller_axes > 0)
			{
				joynet_putw(gp->serial_data, gp->mouse[0]->x);
			}
			if(gp->controller_axes > 1)
			{
				joynet_putw(gp->serial_data, gp->mouse[0]->y);
			}
			if(gp->controller_axes > 2)
			{
				joynet_putw(gp->serial_data, gp->mouse[0]->z);
			}
			if(gp->controller_buttons > 0)
			{
				joynet_putc(gp->serial_data, gp->mouse[0]->b);
			}
			return 1;
		}
		case JOYNET_GAME_TYPE_CONTROLLERS:
		{
			
			unsigned char axis_data;
			char cbits = 1;
			
			for(i = 0; i < gp->controllers; i++)
			{
				if(gp->controller[gp->controller_sort_data[i].index]->port >= 0)
				{
					for(j = 0; j < gp->controller_axes; j++)
					{
						axis_data = (unsigned char)((gp->controller[gp->controller_sort_data[i].index]->axis[j] + 1.0) * 127.5);
						joynet_putc(gp->serial_data, axis_data);
					}
					cbits = 1;
					if(gp->controller_buttons > 0)
					{
						gp->controller[gp->controller_sort_data[i].index]->bits[0] = 0;
						for(j = 0; j < gp->controller_buttons; j++)
						{
							if(gp->controller[gp->controller_sort_data[i].index]->button[j])
							{
								gp->controller[gp->controller_sort_data[i].index]->bits[0] |= cbits;
							}
							cbits = cbits << 1;
						}
						joynet_putc(gp->serial_data, gp->controller[gp->controller_sort_data[i].index]->bits[0]);
//						printf("bits = %d\n", gp->controller[i]->bits[0]);
					}
					cbits = 1;
					if(gp->controller_buttons > 8)
					{
						gp->controller[gp->controller_sort_data[i].index]->bits[1] = 0;
						for(j = 0; j < gp->controller_buttons; j++)
						{
							if(gp->controller[gp->controller_sort_data[i].index]->button[j])
							{
								gp->controller[gp->controller_sort_data[i].index]->bits[1] |= cbits;
							}
							cbits = cbits << 1;
						}
						joynet_putc(gp->serial_data, gp->controller[gp->controller_sort_data[i].index]->bits[1]);
					}
				}
			}
			return 1;
		}
	}
	return 0;
}

/* decode one frame of input received from the server */
void joynet_decode_game_input(JOYNET_GAME * gp)
{
}

/* return how many frames we have left to process */
int joynet_game_input_frames(JOYNET_GAME * gp)
{
	return gp->input_buffer->frames;
}

/* encode and send one frame of input data */
/* if process is true, input will be sent (online play) */
void joynet_update_game(JOYNET_GAME * gp, int process)
{
	ENetPacket * pp;
	char data[1024];
	if(gp->client)
	{
		if(process)
		{
			if(joynet_encode_game_input(gp, data))
			{
				pp = joynet_create_packet(JOYNET_GAME_MESSAGE_INPUT, gp->serial_data);
				enet_peer_send(gp->client->peer, JOYNET_CHANNEL_GAME, pp);
			}
		}
	}
	else
	{
		if(joynet_encode_game_input(gp, data))
		{
			memcpy(&(gp->input_buffer->data[gp->input_buffer->write_pos]), data, gp->input_buffer->frame_size);
//			printf("data - %d - %d\n", data[0], data[1]);
			gp->input_buffer->write_pos += gp->input_buffer->frame_size;
			if(gp->input_buffer->write_pos >= gp->input_buffer->frame_size * gp->max_buffer_frames)
			{
				gp->input_buffer->write_pos = 0;
			}
			gp->input_buffer->frames++;
		}
		else
		{
			printf("failed to encode\n");
		}
	}
}

/* if we have new input send INPUT message
 * send NO_INPUT message when no new input has been received
 * call this once per tick */
void joynet_update_game_server(JOYNET_SERVER * sp, JOYNET_GAME * gp)
{
	ENetPacket * pp;
	char data[1024];
	int i, j;
	
	if(gp->state == JOYNET_GAME_STATE_PLAYING)
	{
		switch(gp->type)
		{
			case JOYNET_GAME_TYPE_MOUSE:
			{
				if(gp->received_input)
				{
					joynet_serialize(sp->serial_data, data);
					if(joynet_current_server_game->controller_axes > 0)
					{
						joynet_putw(sp->serial_data, joynet_current_server_game->player_mouse[joynet_current_server_game->current_player]->x);
					}
					if(joynet_current_server_game->controller_axes > 1)
					{
						joynet_putw(sp->serial_data, joynet_current_server_game->player_mouse[joynet_current_server_game->current_player]->y);
					}
					if(joynet_current_server_game->controller_axes > 2)
					{
						joynet_putw(sp->serial_data, joynet_current_server_game->player_mouse[joynet_current_server_game->current_player]->z);
					}
					if(joynet_current_server_game->controller_buttons > 0)
					{
						joynet_putc(sp->serial_data, joynet_current_server_game->player_mouse[joynet_current_server_game->current_player]->b);
					}
					for(i = 0; i < sp->max_clients; i++)
					{
						if(sp->client[i]->peer && (sp->client[i]->playing || sp->client[i]->spectating == 1))
						{
							pp = joynet_create_packet(JOYNET_GAME_MESSAGE_INPUT, sp->serial_data);
							enet_peer_send(sp->client[i]->peer, JOYNET_CHANNEL_GAME, pp);
						}
					}
				}
				else
				{
					for(i = 0; i < sp->max_clients; i++)
					{
						if(sp->client[i]->peer && (sp->client[i]->playing || sp->client[i]->spectating == 1))
						{
							pp = joynet_create_packet(JOYNET_GAME_MESSAGE_NO_INPUT, NULL);
							enet_peer_send(sp->client[i]->peer, JOYNET_CHANNEL_GAME, pp);
						}
					}
				}
				break;
			}
			case JOYNET_GAME_TYPE_CONTROLLERS:
			{
				unsigned char axis_data;
				char cbits = 1;
				joynet_serialize(sp->serial_data, data);
				for(i = 0; i < joynet_current_server_game->players; i++)
				{
					if(joynet_current_server_game->player[i]->playing)
					{
						for(j = 0; j < joynet_current_server_game->controller_axes; j++)
						{
							axis_data = (unsigned char)((joynet_current_server_game->player_controller[i]->axis[j] + 1.0) * 127.5);
							joynet_putc(sp->serial_data, axis_data);
						}
						if(joynet_current_server_game->controller_buttons > 0)
						{
							joynet_putc(sp->serial_data, joynet_current_server_game->player_controller[i]->bits[0]);
						}
						if(joynet_current_server_game->controller_buttons > 8)
						{
							joynet_current_server_game->player_controller[i]->bits[1] = 0;
							for(j = 0; j < 8; j++)
							{
								if(joynet_current_server_game->player_controller[i]->button[i])
								{
									joynet_current_server_game->player_controller[i]->bits[1] |= cbits;
								}
								cbits = cbits << 1;
							}
							joynet_putc(sp->serial_data, joynet_current_server_game->player_controller[i]->bits[1]);
						}
					}
				}
				for(i = 0; i < sp->max_clients; i++)
				{
					if(sp->client[i]->peer && (sp->client[i]->playing || sp->client[i]->spectating == 1))
					{
						pp = joynet_create_packet(JOYNET_GAME_MESSAGE_INPUT, sp->serial_data);
						enet_peer_send(sp->client[i]->peer, JOYNET_CHANNEL_GAME, pp);
					}
				}
				break;
			}
		}
		gp->received_input = 0;
	}
}

/* decode one input frame and update player controllers to reflect this */   
void joynet_game_logic(JOYNET_GAME * gp)
{
	if(gp->input_buffer->frames > 0)
	{
		switch(gp->type)
		{
			case JOYNET_GAME_TYPE_MOUSE:
			{
				joynet_serialize(gp->serial_data, &gp->input_buffer->data[gp->input_buffer->read_pos]);
				if(gp->controller_axes > 0)
				{
					joynet_getw(gp->serial_data, &gp->player_mouse[0]->x);
				}
				if(gp->controller_axes > 1)
				{
					joynet_getw(gp->serial_data, &gp->player_mouse[0]->y);
				}
				if(gp->controller_axes > 2)
				{
					joynet_getw(gp->serial_data, &gp->player_mouse[0]->z);
				}
				if(gp->controller_buttons > 0)
				{
					joynet_getc(gp->serial_data, &gp->player_mouse[0]->b);
				}
				gp->input_buffer->read_pos += joynet_get_serial_size(gp->serial_data);
				if(gp->input_buffer->read_pos >= gp->max_buffer_frames * gp->input_buffer->frame_size)
				{
					gp->input_buffer->read_pos = 0;
				}
				break;
			}
			case JOYNET_GAME_TYPE_CONTROLLERS:
			{
				unsigned char axis_data;
				int i, j;
				
				for(i = 0; i < gp->players; i++)
				{
					if(gp->player[i]->playing)
					{
						joynet_serialize(gp->serial_data, &gp->input_buffer->data[gp->input_buffer->read_pos]);
						if(gp->controller_axes > 0)
						{
							joynet_getc(gp->serial_data, (char *)(&axis_data));
							gp->player_controller[i]->axis[0] = (float)axis_data / 127.5 - 1.0;
						}
						if(gp->controller_axes > 1)
						{
							joynet_getc(gp->serial_data, (char *)(&axis_data));
							gp->player_controller[i]->axis[1] = (float)axis_data / 127.5 - 1.0;
						}
						if(gp->controller_axes > 2)
						{
							joynet_getc(gp->serial_data, (char *)(&axis_data));
							gp->player_controller[i]->axis[2] = (float)axis_data / 127.5 - 1.0;
						}
						if(gp->controller_buttons > 0)
						{
							joynet_getc(gp->serial_data, &gp->player_controller[i]->bits[0]);
							for(j = 0; j < 8; j++)
							{
								gp->player_controller[i]->button[j] = ((gp->player_controller[i]->bits[0] >> j) & 1);
							}
//								printf("bits = %d\n", gp->player[i]->controller->bits[0]);
						}
						if(gp->controller_buttons > 8)
						{
							joynet_getc(gp->serial_data, &gp->player_controller[i]->bits[1]);
							for(j = 0; j < 8; j++)
							{
								gp->player_controller[i]->button[j + 8] = ((gp->player_controller[i]->bits[1] >> j) & 1);
							}
						}
						gp->input_buffer->read_pos += joynet_get_serial_size(gp->serial_data);
						if(gp->input_buffer->read_pos >= gp->max_buffer_frames * gp->input_buffer->frame_size)
						{
							gp->input_buffer->read_pos = 0;
						}
					}
				}
				break;
			}
		}
		gp->input_buffer->read_frames++;
		gp->input_buffer->frames--;
	}
}
