#include <stdio.h>
#include <string.h>
#include "joynet.h"
#include "game_message.h"
#include "serialize.h"

static int joynet_content_check(int player, int list, unsigned long hash)
{
	int i;
	
	if(joynet_current_server_game->server_content_list[player][list] && joynet_current_server_game->server_content_list[player][list]->count)
	{
		for(i = 0; i < joynet_current_server_game->server_content_list[player][list]->count; i++)
		{
			if(joynet_current_server_game->server_content_list[player][list]->hash[i] == hash)
			{
				return 1;
			}
		}
	}
	return 0;
}

static int joynet_content_check_all(int list, unsigned long hash)
{
	int i;
	
	for(i = 0; i < joynet_current_server_game->players; i++)
	{
		if(joynet_current_server_game->player[i]->playing && !joynet_content_check(i, list, hash))
		{
			return 0;
		}
	}
	return 1;
}

static int joynet_content_check_master(int list, unsigned long hash)
{
	int i;
	
	if(joynet_current_server_game->server_master_content_list[list] && joynet_current_server_game->server_master_content_list[list]->count)
	{
		for(i = 0; i < joynet_current_server_game->server_master_content_list[list]->count; i++)
		{
			if(joynet_current_server_game->server_master_content_list[list]->hash[i] == hash)
			{
				return 1;
			}
		}
	}
	return 0;
}

/* scan all players' content and make master lists */
static void joynet_build_master_content_list(int i)
{
	int j, k;
	
	if(joynet_current_server_game->server_master_content_list[i])
	{
		joynet_current_server_game->server_master_content_list[i]->count = 0; // reset counter
		for(j = 0; j < joynet_current_server_game->players; j++)
		{
			if(joynet_current_server_game->player[j]->playing && joynet_current_server_game->server_content_list[j][i] && joynet_current_server_game->server_content_list[j][i]->count)
			{
				for(k = 0; k < joynet_current_server_game->server_content_list[j][i]->count; k++)
				{
					if(joynet_content_check_all(i, joynet_current_server_game->server_content_list[j][i]->hash[k]) && !joynet_content_check_master(i, joynet_current_server_game->server_content_list[j][i]->hash[k]))
					{
						joynet_current_server_game->server_master_content_list[i]->hash[joynet_current_server_game->server_master_content_list[i]->count] = joynet_current_server_game->server_content_list[j][i]->hash[k];
						joynet_current_server_game->server_master_content_list[i]->count++;
					}
				}
			}
		}
	}
}

int joynet_server_internal_game_callback(ENetEvent * ep)
{
	switch(ep->type)
	{
		case ENET_EVENT_TYPE_DISCONNECT:
		{
			int client = joynet_get_client_from_peer(joynet_current_server_game->server, ep->peer);
			ENetPacket * pp;
			char data[4];
			int i, j;
			if(joynet_current_server_game)
			{
				if(joynet_current_server_game->server->client[client]->playing)
				{
					for(i = 0; i < joynet_current_server_game->players; i++)
					{
						if(joynet_current_server_game->player[i]->playing && joynet_current_server_game->player[i]->client == client)
						{
							joynet_serialize(joynet_current_server_game->server->serial_data, data);
							joynet_putw(joynet_current_server_game->server->serial_data, i);
							for(j = 0; j < joynet_current_server_game->server->max_clients; j++)
							{
								if(j != client && joynet_current_server_game->server->client[j]->peer && (joynet_current_server_game->server->client[j]->playing || joynet_current_server_game->server->client[j]->spectating))
								{
									pp = joynet_create_packet(JOYNET_GAME_MESSAGE_REMOVE_PLAYER, joynet_current_server_game->server->serial_data);
									enet_peer_send(joynet_current_server_game->server->client[j]->peer, JOYNET_CHANNEL_GAME, pp);
								}
							}
							joynet_current_server_game->player[i]->playing = 0;
							joynet_current_server_game->player_count--;
						}
					}
				}
			}
			if(joynet_current_server_game->player_count <= 0)
			{
				joynet_current_server_game->state = JOYNET_GAME_STATE_OFF;
			}
			else
			{
				if(joynet_current_server_game->server->client[client]->master)
				{
					for(i = 0; i < joynet_current_server_game->players; i++)
					{
						if(joynet_current_server_game->player[i]->playing)
						{
							pp = joynet_create_packet(JOYNET_GAME_MESSAGE_ASSIGN_MASTER, NULL);
							enet_peer_send(joynet_current_server_game->server->client[joynet_current_server_game->player[i]->client]->peer, JOYNET_CHANNEL_GAME, pp);
							break;
						}
					}
				}
			}
			joynet_current_server_game->server->client[client]->playing = 0;
			break;
		}
		default:
		{
			break;
		}
	}
	return 1;
}

void joynet_handle_server_game_message(JOYNET_SERVER * sp, JOYNET_MESSAGE * mp)
{
	char data[1024] = {0};
	ENetPacket * pp;
	int i, j;

	switch(mp->type)
	{
		case JOYNET_GAME_MESSAGE_CHECK_ID:
		{
			short length;
			joynet_serialize(sp->serial_data, mp->data);
			joynet_getw(sp->serial_data, &length);
			joynet_read(sp->serial_data, data, length);
			if(!memcmp(data, joynet_current_server_game->name, length))
			{
				pp = joynet_build_packet(JOYNET_GAME_MESSAGE_CHECK_ID, mp->data, mp->data_size);
				enet_peer_send(mp->event->peer, JOYNET_CHANNEL_GAME, pp);
			}
			else
			{
				joynet_serialize(sp->serial_data, data);
				joynet_putw(sp->serial_data, strlen(joynet_current_server_game->name) + 1);
				joynet_write(sp->serial_data, joynet_current_server_game->name, strlen(joynet_current_server_game->name) + 1);
				pp = joynet_create_packet(JOYNET_GAME_MESSAGE_CHECK_ID_FAIL, sp->serial_data);
				enet_peer_send(mp->event->peer, JOYNET_CHANNEL_GAME, pp);
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_CONNECT:
		{
			int client = joynet_get_client_from_peer(sp, mp->event->peer);
			short player;
			int assigned_player = -1;
			short controller;
			int confail = 0;
			
			if(joynet_current_server_game->state == JOYNET_GAME_STATE_OFF)
			{
				joynet_serialize(sp->serial_data, mp->data);
				joynet_getw(sp->serial_data, &controller);
				joynet_getw(sp->serial_data, &player);
				/* see if this controller is already used (prevent double-connected controllers) */
				i = 0;
				if(joynet_current_server_game->type == JOYNET_GAME_TYPE_CONTROLLERS)
				{
					for(i = 0; i < joynet_current_server_game->players; i++)
					{
						if(joynet_current_server_game->player[i]->playing && joynet_current_server_game->player_controller[i]->backport == controller && joynet_current_server_game->player[i]->client == client)
						{
							break;
						}
					}
				}
				else if(joynet_current_server_game->type == JOYNET_GAME_TYPE_MOUSE)
				{
					for(i = 0; i < joynet_current_server_game->players; i++)
					{
						if(joynet_current_server_game->player[i]->playing && joynet_current_server_game->player_mouse[i]->backport == controller && joynet_current_server_game->player[i]->client == client)
						{
//							break;
						}
					}
				}
				if(i >= joynet_current_server_game->players)
				{
					if(player < 0)
					{
						for(i = 0; i < joynet_current_server_game->players; i++)
						{
							if(!joynet_current_server_game->player[i]->playing)
							{
								assigned_player = i;
								break;
							}
						}
					}
					else
					{
						assigned_player = player;
					}
					if(assigned_player >= 0 && !joynet_current_server_game->player[assigned_player]->playing)
					{
						joynet_current_server_game->player[assigned_player]->playing = 1;
						if(joynet_current_server_game->type == JOYNET_GAME_TYPE_CONTROLLERS)
						{
							memset(joynet_current_server_game->player_controller[assigned_player], 0, sizeof(JOYNET_CONTROLLER));
							joynet_current_server_game->player_controller[assigned_player]->port = assigned_player;
							joynet_current_server_game->player_controller[assigned_player]->backport = controller;
						}
						else if(joynet_current_server_game->type == JOYNET_GAME_TYPE_MOUSE)
						{
							memset(joynet_current_server_game->player_mouse[assigned_player], 0, sizeof(JOYNET_MOUSE));
							joynet_current_server_game->player_mouse[assigned_player]->port = assigned_player;
							joynet_current_server_game->player_mouse[assigned_player]->backport = controller;
						}
						
						/* reset selections to prevent bugs */
						memset(joynet_current_server_game->player[assigned_player]->selected_content, 0, sizeof(unsigned long) * JOYNET_GAME_MAX_CONTENT_LISTS);
						memset(joynet_current_server_game->player[assigned_player]->selected_content_index, 0, sizeof(int) * JOYNET_GAME_MAX_CONTENT_LISTS);
						
						joynet_serialize(sp->serial_data, data);
						joynet_write(sp->serial_data, mp->data, sizeof(short));
						joynet_putw(sp->serial_data, assigned_player);
						pp = joynet_create_packet(JOYNET_GAME_MESSAGE_CONNECT, sp->serial_data);
						enet_peer_send(mp->event->peer, JOYNET_CHANNEL_GAME, pp);
						sp->client[client]->playing++;
						
						/* first player connected gets to be the master */
						if(joynet_current_server_game->player_count == 0)
						{
							sp->client[client]->master = 1;
							pp = joynet_create_packet(JOYNET_GAME_MESSAGE_ASSIGN_MASTER, NULL);
							enet_peer_send(mp->event->peer, JOYNET_CHANNEL_GAME, pp);
						}
						joynet_current_server_game->player_count++;
						
						/* add player to all connected players */
						joynet_serialize(sp->serial_data, data);
						joynet_putw(sp->serial_data, assigned_player);
						joynet_putw(sp->serial_data, strlen(sp->client[client]->screen_name) + 1);
						joynet_write(sp->serial_data, sp->client[client]->screen_name, strlen(sp->client[client]->screen_name) + 1);
						for(i = 0; i < sp->max_clients; i++)
						{
							if(i != client && sp->client[i]->peer && (sp->client[i]->playing || sp->client[i]->spectating))
							{
								pp = joynet_create_packet(JOYNET_GAME_MESSAGE_ADD_PLAYER, sp->serial_data);
								enet_peer_send(sp->client[i]->peer, JOYNET_CHANNEL_GAME, pp);
							}
						}
						joynet_current_server_game->player[assigned_player]->client = client;
						
						/* add all players to connected player */
						for(i = 0; i < joynet_current_server_game->players; i++)
						{
							if(joynet_current_server_game->player[i]->playing)
							{
								joynet_serialize(sp->serial_data, data);
								joynet_putw(sp->serial_data, i);
								joynet_putw(sp->serial_data, strlen(sp->client[joynet_current_server_game->player[i]->client]->screen_name) + 1);
								joynet_write(sp->serial_data, sp->client[client]->screen_name, strlen(sp->client[joynet_current_server_game->player[i]->client]->screen_name) + 1);
								pp = joynet_create_packet(JOYNET_GAME_MESSAGE_ADD_PLAYER, sp->serial_data);
								enet_peer_send(sp->client[client]->peer, JOYNET_CHANNEL_GAME, pp);
							}
						}
						
						/* update options */
						joynet_serialize(sp->serial_data, data);
						joynet_putw(sp->serial_data, joynet_current_server_game->options);
						for(i = 0; i < joynet_current_server_game->options; i++)
						{
							joynet_putl(sp->serial_data, joynet_current_server_game->server_option[i]);
						}
						pp = joynet_create_packet(JOYNET_GAME_MESSAGE_UPDATE_OPTIONS, sp->serial_data);
						enet_peer_send(sp->client[client]->peer, JOYNET_CHANNEL_GAME, pp);
						
						/* update player options */
						for(j = 0; j < joynet_current_server_game->players; j++)
						{
							if(joynet_current_server_game->player[j]->playing)
							{
								joynet_serialize(sp->serial_data, data);
								joynet_putw(sp->serial_data, j);
								joynet_putw(sp->serial_data, strlen(joynet_current_server_game->player[j]->name) + 1);
								joynet_write(sp->serial_data, joynet_current_server_game->player[j]->name, strlen(joynet_current_server_game->player[j]->name) + 1);
								joynet_putw(sp->serial_data, joynet_current_server_game->player[j]->options);
								for(i = 0; i < joynet_current_server_game->player[j]->options; i++)
								{
									joynet_putl(sp->serial_data, joynet_current_server_game->player[j]->server_option[i]);
								}
								pp = joynet_create_packet(JOYNET_GAME_MESSAGE_UPDATE_PLAYER_OPTIONS, sp->serial_data);
								enet_peer_send(sp->client[client]->peer, JOYNET_CHANNEL_GAME, pp);
							}
						}
					}
					else
					{
						confail = 1;
					}
				}
				else
				{
					confail = 1;
				}
			}
			else
			{
				confail = 1;
			}
			if(confail)
			{
				pp = joynet_build_packet(JOYNET_GAME_MESSAGE_CONNECT_FAIL, mp->data, mp->data_size);
				enet_peer_send(mp->event->peer, JOYNET_CHANNEL_GAME, pp);
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_DISCONNECT:
		{
			int client = joynet_get_client_from_peer(sp, mp->event->peer);
			short player;
			short controller;
			int oldsize;
			char * cdata;
			int unmaster = 1;
			
			/* disconnect the controller */
			joynet_serialize(sp->serial_data, mp->data);
			joynet_getw(sp->serial_data, &controller);
			joynet_getw(sp->serial_data, &player);
			if(joynet_current_server_game->player[player]->playing)
			{
				joynet_current_server_game->player[player]->playing = 0;
				joynet_current_server_game->player_count--;
			}
			joynet_serialize(sp->serial_data, data);
			joynet_putw(sp->serial_data, controller);
			pp = joynet_create_packet(JOYNET_GAME_MESSAGE_DISCONNECT, sp->serial_data);
			enet_peer_send(mp->event->peer, JOYNET_CHANNEL_GAME, pp);
			
			/* remove master status if necessary */
			if(sp->client[client]->master)
			{
				for(i = 0; i < joynet_current_server_game->players; i++)
				{
					if(joynet_current_server_game->player[i]->playing && joynet_current_server_game->player[i]->client == client)
					{
						unmaster = 0;
					}
				}
				if(unmaster)
				{
					sp->client[client]->master = 0;
					for(i = 0; i < joynet_current_server_game->players; i++)
					{
						if(joynet_current_server_game->player[i]->playing)
						{
							sp->client[joynet_current_server_game->player[i]->client]->master = 1;
							pp = joynet_create_packet(JOYNET_GAME_MESSAGE_ASSIGN_MASTER, NULL);
							enet_peer_send(joynet_current_server_game->server->client[joynet_current_server_game->player[i]->client]->peer, JOYNET_CHANNEL_GAME, pp);
							break;
						}
					}
				}
			}
			
			/* remove player from clients */
			joynet_serialize(sp->serial_data, data);
			joynet_putw(sp->serial_data, player);
			for(i = 0; i < sp->max_clients; i++)
			{
				if(sp->client[i]->peer && (sp->client[i]->playing || sp->client[i]->spectating))
				{
					pp = joynet_create_packet(JOYNET_GAME_MESSAGE_REMOVE_PLAYER, sp->serial_data);
					enet_peer_send(sp->client[i]->peer, JOYNET_CHANNEL_GAME, pp);
				}
			}
			sp->client[client]->playing--;
			
			/* rebuild content lists */
			for(i = 0; i < JOYNET_GAME_MAX_CONTENT_LISTS; i++)
			{
				if(joynet_current_server_game->server_master_content_list[i])
				{
					oldsize = joynet_current_server_game->server_master_content_list[i]->count;
					joynet_build_master_content_list(i);
					
					/* upload new content list if there are changes */
					if(joynet_current_server_game->server_master_content_list[i]->count != oldsize)
					{
						cdata = malloc(sizeof(unsigned long) * joynet_current_server_game->server_master_content_list[i]->count + 32);
						joynet_serialize(sp->serial_data, cdata);
						joynet_putw(sp->serial_data, i);
						joynet_putw(sp->serial_data, joynet_current_server_game->server_master_content_list[i]->count);
						for(j = 0; j < joynet_current_server_game->server_master_content_list[i]->count; j++)
						{
							joynet_putl(sp->serial_data, joynet_current_server_game->server_master_content_list[i]->hash[j]);
						}
						for(j = 0; j < sp->max_clients; j++)
						{
							if(sp->client[j]->peer && (sp->client[j]->playing || sp->client[j]->spectating))
							{
								pp = joynet_create_packet(JOYNET_GAME_MESSAGE_UPLOAD_PLAYER_CONTENT, sp->serial_data);
								enet_peer_send(sp->client[j]->peer, JOYNET_CHANNEL_GAME, pp);
							}
						}
						free(cdata);
					}
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_ADD_PLAYER:
		{
			break;
		}
		case JOYNET_GAME_MESSAGE_UPDATE_PLAYER_OPTIONS:
		{
			long option;
			short count;
			short length;
			short player;
			
			/* store options */
			joynet_serialize(sp->serial_data, mp->data);
			joynet_getw(sp->serial_data, &player);
			joynet_getw(sp->serial_data, &length);
			joynet_read(sp->serial_data, joynet_current_server_game->player[player]->name, length);
			joynet_getw(sp->serial_data, &count);
			joynet_current_server_game->player[player]->options = count;
			for(i = 0; i < joynet_current_server_game->player[player]->options; i++)
			{
				joynet_getl(sp->serial_data, &option);
				joynet_current_server_game->player[player]->server_option[i] = option;
			}
			
			/* send to all players/specators */
			for(i = 0; i < sp->max_clients; i++)
			{
				if(sp->client[i]->peer && (sp->client[i]->playing || sp->client[i]->spectating))
				{
					pp = joynet_build_packet(JOYNET_GAME_MESSAGE_UPDATE_PLAYER_OPTIONS, mp->data, mp->data_size);
					enet_peer_send(sp->client[i]->peer, JOYNET_CHANNEL_GAME, pp);
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_UPDATE_OPTION:
		{
			long option;
			short opt_num;
			
			/* store option */
			joynet_serialize(sp->serial_data, mp->data);
			joynet_getw(sp->serial_data, &opt_num);
			joynet_getl(sp->serial_data, &option);
			joynet_current_server_game->server_option[opt_num] = option;
			
			/* send to all players/specators */
			for(i = 0; i < sp->max_clients; i++)
			{
				if(sp->client[i]->peer && (sp->client[i]->playing || sp->client[i]->spectating))
				{
					pp = joynet_build_packet(JOYNET_GAME_MESSAGE_UPDATE_OPTION, mp->data, mp->data_size);
					enet_peer_send(sp->client[i]->peer, JOYNET_CHANNEL_GAME, pp);
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_UPDATE_OPTIONS:
		{
			long option;
			short count;
			
			/* store options */
			joynet_serialize(sp->serial_data, mp->data);
			joynet_getw(sp->serial_data, &count);
			joynet_current_server_game->options = count;
			
			for(i = 0; i < joynet_current_server_game->options; i++)
			{
				joynet_getl(sp->serial_data, &option);
				joynet_current_server_game->server_option[i] = option;
			}
			
			/* send to all players/specators */
			for(i = 0; i < sp->max_clients; i++)
			{
				if(sp->client[i]->peer && (sp->client[i]->playing || sp->client[i]->spectating))
				{
					pp = joynet_build_packet(JOYNET_GAME_MESSAGE_UPDATE_OPTIONS, mp->data, mp->data_size);
					enet_peer_send(sp->client[i]->peer, JOYNET_CHANNEL_GAME, pp);
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_SPECTATE:
		{
			int client = joynet_get_client_from_peer(sp, mp->event->peer);
			char * cdata;
			
			if(joynet_current_server_game->state == JOYNET_GAME_STATE_OFF)
			{
				sp->client[client]->spectating = 1;
			}
			else
			{
				sp->client[client]->spectating = 2;
			}
			pp = joynet_create_packet(JOYNET_GAME_MESSAGE_SPECTATE, NULL);
			enet_peer_send(sp->client[client]->peer, JOYNET_CHANNEL_GAME, pp);
			
			/* send content lists */
			for(j = 0; j < JOYNET_GAME_MAX_CONTENT_LISTS; j++)
			{
				if(joynet_current_server_game->server_master_content_list[j])
				{
					cdata = malloc(sizeof(unsigned long) * joynet_current_server_game->server_master_content_list[j]->count + 32);
					joynet_serialize(sp->serial_data, cdata);
					joynet_putw(sp->serial_data, j);
					joynet_putw(sp->serial_data, joynet_current_server_game->server_master_content_list[j]->count);
					for(i = 0; i < joynet_current_server_game->server_master_content_list[j]->count; i++)
					{
						joynet_putl(sp->serial_data, joynet_current_server_game->server_master_content_list[j]->hash[i]);
					}
					pp = joynet_create_packet(JOYNET_GAME_MESSAGE_UPLOAD_PLAYER_CONTENT, sp->serial_data);
					enet_peer_send(sp->client[client]->peer, JOYNET_CHANNEL_GAME, pp);
					free(cdata);
				}
			}
			
			/* add players to spectator */
			for(i = 0; i < joynet_current_server_game->players; i++)
			{
				if(joynet_current_server_game->player[i]->playing)
				{
					joynet_serialize(sp->serial_data, data);
					joynet_putw(sp->serial_data, i);
					joynet_putw(sp->serial_data, strlen(sp->client[joynet_current_server_game->player[i]->client]->screen_name) + 1);
					joynet_write(sp->serial_data, sp->client[client]->screen_name, strlen(sp->client[joynet_current_server_game->player[i]->client]->screen_name) + 1);
					pp = joynet_create_packet(JOYNET_GAME_MESSAGE_ADD_PLAYER, sp->serial_data);
					enet_peer_send(sp->client[client]->peer, JOYNET_CHANNEL_GAME, pp);
					
					/* send player content selections */
					for(j = 0; j < JOYNET_GAME_MAX_CONTENT_LISTS; j++)
					{
						if(joynet_current_server_game->server_master_content_list[j])
						{
							joynet_serialize(sp->serial_data, data);
							joynet_putw(sp->serial_data, i);
							joynet_putw(sp->serial_data, j);
							joynet_putl(sp->serial_data, joynet_current_server_game->player[i]->selected_content[j]);
							pp = joynet_create_packet(JOYNET_GAME_MESSAGE_SELECT_PLAYER_CONTENT, sp->serial_data);
							enet_peer_send(sp->client[client]->peer, JOYNET_CHANNEL_GAME, pp);
						}
					}
					
					/* send player options */
					joynet_serialize(sp->serial_data, data);
					joynet_putw(sp->serial_data, i);
					joynet_putw(sp->serial_data, strlen(joynet_current_server_game->player[i]->name) + 1);
					joynet_write(sp->serial_data, joynet_current_server_game->player[i]->name, strlen(joynet_current_server_game->player[i]->name) + 1);
					joynet_putw(sp->serial_data, joynet_current_server_game->player[i]->options);
					for(j = 0; j < joynet_current_server_game->player[i]->options; j++)
					{
						joynet_putl(sp->serial_data, joynet_current_server_game->player[i]->server_option[j]);
					}
					pp = joynet_create_packet(JOYNET_GAME_MESSAGE_UPDATE_PLAYER_OPTIONS, sp->serial_data);
					enet_peer_send(sp->client[client]->peer, JOYNET_CHANNEL_GAME, pp);
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_LEAVE:
		{
			int client = joynet_get_client_from_peer(sp, mp->event->peer);
			sp->client[client]->spectating = 0;
			break;
		}
		case JOYNET_GAME_MESSAGE_UPLOAD_PLAYER_CONTENT:
		{
			char * cdata;
			short player;
			short list;
			short items;
			int oldsize = 0;
			joynet_serialize(sp->serial_data, mp->data);
			joynet_getw(sp->serial_data, &player);
			joynet_getw(sp->serial_data, &list);
			joynet_getw(sp->serial_data, &items);
			if(!joynet_current_server_game->server_content_list[player][list])
			{
				joynet_current_server_game->server_content_list[player][list] = malloc(sizeof(JOYNET_CONTENT_LIST));
			}
			for(i = 0; i < items; i++)
			{
				joynet_getl(sp->serial_data, (long *)(&joynet_current_server_game->server_content_list[player][list]->hash[i]));
			}
			joynet_current_server_game->server_content_list[player][list]->count = items;
			
			/* build master content list */
			if(!joynet_current_server_game->server_master_content_list[list])
			{
				joynet_current_server_game->server_master_content_list[list] = malloc(sizeof(JOYNET_CONTENT_LIST));
			}
			else
			{
				oldsize = joynet_current_server_game->server_master_content_list[list]->count;
			}
			joynet_build_master_content_list(list);

			/* upload master content list to all players and spectators */
			if(joynet_current_server_game->server_master_content_list[list]->count != oldsize)
			{
				cdata = malloc(sizeof(unsigned long) * joynet_current_server_game->server_master_content_list[list]->count + 32);
				joynet_serialize(sp->serial_data, cdata);
				joynet_putw(sp->serial_data, list);
				joynet_putw(sp->serial_data, joynet_current_server_game->server_master_content_list[list]->count);
				for(i = 0; i < joynet_current_server_game->server_master_content_list[list]->count; i++)
				{
					joynet_putl(sp->serial_data, joynet_current_server_game->server_master_content_list[list]->hash[i]);
				}
				for(j = 0; j < sp->max_clients; j++)
				{
					if(sp->client[j]->peer && (sp->client[j]->playing || sp->client[j]->spectating))
					{
						pp = joynet_create_packet(JOYNET_GAME_MESSAGE_UPLOAD_PLAYER_CONTENT, sp->serial_data);
						enet_peer_send(sp->client[j]->peer, JOYNET_CHANNEL_GAME, pp);
					}
				}
				free(cdata);
				
				/* adjust selections to ensure players have selected content that everyone has */
				for(i = 0; i < joynet_current_server_game->players; i++)
				{
					if(joynet_current_server_game->player[i]->playing)
					{
						/* only change selection if something was selected before and it's not available any more */
						if(joynet_current_server_game->player[i]->selected_content[list] != 0)
						{
							for(j = 0; j < joynet_current_server_game->server_master_content_list[list]->count; j++)
							{
								if(joynet_current_server_game->player[i]->selected_content[list] == joynet_current_server_game->server_master_content_list[list]->hash[j])
								{
									break;
								}
							}
		//					printf("player %d selected content %d: %lu\n", i, list, joynet_current_server_game->player[i]->selected_content[list]);
							/* fix me: this causes selections to reset to 0 when they shouldn't */
							if(j == joynet_current_server_game->server_master_content_list[list]->count)
							{
								joynet_serialize(sp->serial_data, data);
								joynet_putw(sp->serial_data, i);
								joynet_putw(sp->serial_data, list);
								joynet_putl(sp->serial_data, joynet_current_server_game->server_master_content_list[list]->hash[0]);
								for(j = 0; j < sp->max_clients; j++)
								{
									if(sp->client[j]->peer && (sp->client[j]->playing || sp->client[j]->spectating))
									{
										pp = joynet_create_packet(JOYNET_GAME_MESSAGE_SELECT_PLAYER_CONTENT, sp->serial_data);
										enet_peer_send(sp->client[j]->peer, JOYNET_CHANNEL_GAME, pp);
									}
								}
							}
							else if(joynet_current_server_game->player[i]->selected_content[list] != 0)
							{
								joynet_serialize(sp->serial_data, data);
								joynet_putw(sp->serial_data, i);
								joynet_putw(sp->serial_data, list);
								joynet_putl(sp->serial_data, joynet_current_server_game->player[i]->selected_content[list]);
								for(j = 0; j < sp->max_clients; j++)
								{
									if(sp->client[j]->peer && (sp->client[j]->playing || sp->client[j]->spectating))
									{
										pp = joynet_create_packet(JOYNET_GAME_MESSAGE_SELECT_PLAYER_CONTENT, sp->serial_data);
										enet_peer_send(sp->client[j]->peer, JOYNET_CHANNEL_GAME, pp);
									}
								}
							}
						}
					}
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_SELECT_PLAYER_CONTENT:
		{
			short player;
			short list;
			unsigned long hash;
			
			joynet_serialize(sp->serial_data, mp->data);
			joynet_getw(sp->serial_data, &player);
			joynet_getw(sp->serial_data, &list);
			joynet_getl(sp->serial_data, (long *)(&hash));
			
			/* see if the requested content is in the master list */
			for(i = 0; i < joynet_current_server_game->server_master_content_list[list]->count; i++)
			{
				if(joynet_current_server_game->server_master_content_list[list]->hash[i] == hash)
				{
					joynet_current_server_game->player[player]->selected_content[list] = hash;
					for(j = 0; j < sp->max_clients; j++)
					{
						if(sp->client[j]->peer && (sp->client[j]->playing || sp->client[j]->spectating))
						{
							pp = joynet_build_packet(JOYNET_GAME_MESSAGE_SELECT_PLAYER_CONTENT, mp->data, mp->data_size);
							enet_peer_send(sp->client[j]->peer, JOYNET_CHANNEL_GAME, pp);
						}
					}
					break;
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_START:
		{
			if(joynet_current_server_game->state == JOYNET_GAME_STATE_OFF)
			{
				joynet_current_server_game->state = JOYNET_GAME_STATE_PLAYING;
				joynet_current_server_game->received_input = 0;
				for(i = 0; i < sp->max_clients; i++)
				{
					if(sp->client[i]->peer && (sp->client[i]->playing || sp->client[i]->spectating))
					{
						pp = joynet_create_packet(JOYNET_GAME_MESSAGE_START, NULL);
						enet_peer_send(sp->client[i]->peer, JOYNET_CHANNEL_GAME, pp);
					}
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_PAUSE:
		{
			if(joynet_current_server_game->state == JOYNET_GAME_STATE_PLAYING)
			{
				joynet_current_server_game->state = JOYNET_GAME_STATE_PAUSED;
				for(i = 0; i < sp->max_clients; i++)
				{
					if(sp->client[i]->peer && (sp->client[i]->playing || sp->client[i]->spectating == 1))
					{
						pp = joynet_create_packet(JOYNET_GAME_MESSAGE_PAUSE, NULL);
						enet_peer_send(sp->client[i]->peer, JOYNET_CHANNEL_GAME, pp);
					}
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_RESUME:
		{
			if(joynet_current_server_game->state == JOYNET_GAME_STATE_PAUSED)
			{
				joynet_current_server_game->state = JOYNET_GAME_STATE_PLAYING;
				for(i = 0; i < sp->max_clients; i++)
				{
					if(sp->client[i]->peer && (sp->client[i]->playing || sp->client[i]->spectating == 1))
					{
						pp = joynet_create_packet(JOYNET_GAME_MESSAGE_RESUME, NULL);
						enet_peer_send(sp->client[i]->peer, JOYNET_CHANNEL_GAME, pp);
					}
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_END:
		{
			if(joynet_current_server_game->state == JOYNET_GAME_STATE_PLAYING || joynet_current_server_game->state == JOYNET_GAME_STATE_PAUSED)
			{
				joynet_current_server_game->state = JOYNET_GAME_STATE_OFF;
				for(i = 0; i < sp->max_clients; i++)
				{
					if(sp->client[i]->peer && (sp->client[i]->playing || sp->client[i]->spectating == 1))
					{
						pp = joynet_create_packet(JOYNET_GAME_MESSAGE_END, NULL);
						enet_peer_send(sp->client[i]->peer, JOYNET_CHANNEL_GAME, pp);
					}
					if(sp->client[i]->spectating == 2)
					{
						sp->client[i]->spectating = 1;
					}
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_INPUT:
		{
			int client = joynet_get_client_from_peer(sp, mp->event->peer);
			short x, y, z;
			char b, d;
			
			switch(joynet_current_server_game->type)
			{
				case JOYNET_GAME_TYPE_MOUSE:
				{
					/* controls must come from client of current player */
					if(joynet_current_server_game->player[joynet_current_server_game->current_player]->client == client)
					{
						joynet_serialize(sp->serial_data, mp->data);
						if(joynet_current_server_game->controller_axes > 0)
						{
							joynet_getw(sp->serial_data, &x);
							if(x != joynet_current_server_game->player_mouse[joynet_current_server_game->current_player]->x)
							{
								joynet_current_server_game->player_mouse[joynet_current_server_game->current_player]->x = x;
								d = 1;
							}
						}
						if(joynet_current_server_game->controller_axes > 1)
						{
							joynet_getw(sp->serial_data, &y);
							if(y != joynet_current_server_game->player_mouse[joynet_current_server_game->current_player]->y)
							{
								joynet_current_server_game->player_mouse[joynet_current_server_game->current_player]->y = y;
								d = 1;
							}
						}
						if(joynet_current_server_game->controller_axes > 2)
						{
							joynet_getw(sp->serial_data, &z);
							if(z != joynet_current_server_game->player_mouse[joynet_current_server_game->current_player]->z)
							{
								joynet_current_server_game->player_mouse[joynet_current_server_game->current_player]->z = z;
								d = 1;
							}
						}
						if(joynet_current_server_game->controller_buttons > 0)
						{
							joynet_getc(sp->serial_data, &b);
							if(b != joynet_current_server_game->player_mouse[joynet_current_server_game->current_player]->b)
							{
								joynet_current_server_game->player_mouse[joynet_current_server_game->current_player]->b = b;
								d = 1;
							}
						}
						if(d)
						{
							joynet_current_server_game->received_input = 1;
						}
						break;
					}
					break;
				}
				case JOYNET_GAME_TYPE_CONTROLLERS:
				{
					unsigned char axis_data;
					joynet_serialize(sp->serial_data, mp->data);
					for(i = 0; i < joynet_current_server_game->players; i++)
					{
						/* update controllers related to this client */
						if(joynet_current_server_game->player[i]->playing && joynet_current_server_game->player[i]->client == client)
						{
							for(j = 0; j < joynet_current_server_game->controller_axes; j++)
							{
								joynet_getc(sp->serial_data, (char *)(&axis_data));
								joynet_current_server_game->player_controller[i]->axis[j] = (float)axis_data / 127.5 - 1.0;
							}
							if(joynet_current_server_game->controller_buttons > 0)
							{
								joynet_getc(sp->serial_data, &joynet_current_server_game->player_controller[i]->bits[0]);
								for(j = 0; j < 8; j++)
								{
									joynet_current_server_game->player_controller[i]->button[j] = ((joynet_current_server_game->player_controller[i]->bits[0] >> j) & 1);
								}
							}
							if(joynet_current_server_game->controller_buttons > 8)
							{
								joynet_getc(sp->serial_data, &joynet_current_server_game->player_controller[i]->bits[1]);
								for(j = 0; j < 8; j++)
								{
									joynet_current_server_game->player_controller[i]->button[j] = ((joynet_current_server_game->player_controller[i]->bits[1] >> j) & 1);
								}
							}
							joynet_current_server_game->received_input = 1;
						}
					}
					break;
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_INPUT_OVERFLOW:
		{
			int client = joynet_get_client_from_peer(sp, mp->event->peer);
			ENetPacket * pp;
			char data[4];
			int i, j;
			for(i = 0; i < joynet_current_server_game->players; i++)
			{
				/* update controllers related to this client */
				if(joynet_current_server_game->player[i]->playing && joynet_current_server_game->player[i]->client == client)
				{
					joynet_serialize(joynet_current_server_game->server->serial_data, data);
					joynet_putw(joynet_current_server_game->server->serial_data, i);
					for(j = 0; j < joynet_current_server_game->server->max_clients; j++)
					{
						if(j != client && joynet_current_server_game->server->client[j]->peer && (joynet_current_server_game->server->client[j]->playing || joynet_current_server_game->server->client[j]->spectating))
						{
							pp = joynet_create_packet(JOYNET_GAME_MESSAGE_REMOVE_PLAYER, joynet_current_server_game->server->serial_data);
							enet_peer_send(joynet_current_server_game->server->client[j]->peer, JOYNET_CHANNEL_GAME, pp);
						}
					}
					joynet_current_server_game->player[i]->playing = 0;
					joynet_current_server_game->player_count--;
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_SELECT_PLAYER:
		{
			short player;

			joynet_serialize(sp->serial_data, mp->data);
			joynet_getw(sp->serial_data, &player);
			joynet_current_server_game->current_player = player;
			switch(joynet_current_server_game->type)
			{
				/* clear buttons in mouse-based games to prevent clicks from current player's previous turn
				 * from happening in this turn */
				case JOYNET_GAME_TYPE_MOUSE:
				{
					joynet_current_server_game->player_mouse[joynet_current_server_game->current_player]->b = 0;
					joynet_current_server_game->received_input = 1; // force server to send current input
					break;
				}
			}
			if(joynet_current_server_game->player[player]->playing)
			{
				for(i = 0; i < sp->max_clients; i++)
				{
					if(sp->client[i]->peer && (sp->client[i]->playing || sp->client[i]->spectating == 1))
					{
						pp = joynet_build_packet(JOYNET_GAME_MESSAGE_SELECT_PLAYER, mp->data, mp->data_size);
						enet_peer_send(sp->client[i]->peer, JOYNET_CHANNEL_GAME, pp);
					}
				}
			}
			break;
		}
	}
}

void joynet_handle_client_game_message(JOYNET_CLIENT * cp, JOYNET_MESSAGE * mp)
{
	int i, j;
	char * data;
	
	switch(mp->type)
	{
		case JOYNET_GAME_MESSAGE_CHECK_ID:
		{
			joynet_current_game->client = cp; // correct server, play online
			if(joynet_current_game->callback)
			{
				joynet_set_client_channel_callback(joynet_current_game->client, JOYNET_CHANNEL_GAME, joynet_current_game->callback);
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_CHECK_ID_FAIL:
		{
			/* set callback so the failure notice can be processed through the custom callback */
			if(joynet_current_game->callback)
			{
				joynet_set_client_channel_callback(cp, JOYNET_CHANNEL_GAME, joynet_current_game->callback);
			}
			joynet_current_game->client = NULL; // incorrect server, play locally
			break;
		}
		case JOYNET_GAME_MESSAGE_CONNECT:
		{
			ENetPacket * pp;
			
			if(joynet_current_game->state == JOYNET_GAME_STATE_OFF)
			{
				short controller;
				short port;
						
				joynet_serialize(cp->serial_data, mp->data);
				joynet_getw(cp->serial_data, &controller);
				joynet_getw(cp->serial_data, &port);
				if(joynet_current_game->type == JOYNET_GAME_TYPE_CONTROLLERS)
				{
					joynet_current_game->controller[controller]->port = port;
				}
				else if(joynet_current_game->type == JOYNET_GAME_TYPE_MOUSE)
				{
					joynet_current_game->mouse[controller]->port = port;
				}
				
				/* upload content lists */
				for(i = 0; i < JOYNET_GAME_MAX_CONTENT_LISTS; i++)
				{
					if(joynet_current_game->local_content_list[i] && joynet_current_game->local_content_list[i]->count)
					{
						data = malloc(sizeof(unsigned long) * joynet_current_game->local_content_list[i]->count + 32);
						joynet_serialize(cp->serial_data, data);
						joynet_putw(cp->serial_data, port);
						joynet_putw(cp->serial_data, i);
						joynet_putw(cp->serial_data, joynet_current_game->local_content_list[i]->count);
						for(j = 0; j < joynet_current_game->local_content_list[i]->count; j++)
						{
							joynet_putl(cp->serial_data, joynet_current_game->local_content_list[i]->hash[j]);
						}
						pp = joynet_create_packet(JOYNET_GAME_MESSAGE_UPLOAD_PLAYER_CONTENT, cp->serial_data);
						enet_peer_send(joynet_current_game->client->peer, JOYNET_CHANNEL_GAME, pp);
						free(data);
					}
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_DISCONNECT:
		{
//			short player;
			short controller;
			int unmaster = 1;
			
			/* disconnect the controller */
			joynet_serialize(cp->serial_data, mp->data);
			joynet_getw(cp->serial_data, &controller);
//			joynet_getw(cp->serial_data, &player);
			if(joynet_current_game->type == JOYNET_GAME_TYPE_CONTROLLERS)
			{
				joynet_current_game->controller[controller]->port = -1;
			}
			else if(joynet_current_game->type == JOYNET_GAME_TYPE_MOUSE)
			{
				joynet_current_game->mouse[controller]->port = -1;
			}
			
			/* remove master status if necessary */
			if(cp->master)
			{
				if(joynet_current_game->type == JOYNET_GAME_TYPE_CONTROLLERS)
				{
					for(i = 0; i < joynet_current_game->controllers; i++)
					{
						if(i != controller && joynet_current_game->controller[i]->port >= 0)
						{
							unmaster = 0;
						}
					}
				}
				else if(joynet_current_game->type == JOYNET_GAME_TYPE_MOUSE)
				{
					for(i = 0; i < joynet_current_game->controllers; i++)
					{
						if(i != controller && joynet_current_game->mouse[i]->port >= 0)
						{
							unmaster = 0;
						}
					}
				}
				if(unmaster)
				{
					cp->master = 0;
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_ADD_PLAYER:
		{
			short name_length;
			short player;
			
			joynet_serialize(cp->serial_data, mp->data);
			joynet_getw(cp->serial_data, &player);
			joynet_getw(cp->serial_data, &name_length);
			joynet_read(cp->serial_data, joynet_current_game->player[player]->name, name_length);
			
			/* do not attempt connection of slot already used */
			if(!joynet_current_game->player[player]->playing)
			{
				joynet_current_game->player[player]->playing = 1;
				joynet_current_game->player_count++;
				switch(joynet_current_game->type)
				{
					case JOYNET_GAME_TYPE_MOUSE:
					{
						memset(joynet_current_game->player_mouse[player], 0, sizeof(JOYNET_MOUSE));
						joynet_current_game->player_mouse[player]->port = player;
						for(i = 0; i < joynet_current_game->controllers; i++)
						{
							if(joynet_current_game->mouse[i]->port == player)
							{
								joynet_current_game->player[player]->local = 1;
								break;
							}
						}
						if(i == joynet_current_game->controllers)
						{
							joynet_current_game->player[player]->local = 0;
						}
						break;
					}
					case JOYNET_GAME_TYPE_CONTROLLERS:
					{
						memset(joynet_current_game->player_controller[player], 0, sizeof(JOYNET_CONTROLLER));
						joynet_current_game->player_controller[player]->port = player;
						for(i = 0; i < joynet_current_game->controllers; i++)
						{
							if(joynet_current_game->controller[i]->port == player)
							{
								joynet_current_game->player[player]->local = 1;
								break;
							}
						}
						if(i == joynet_current_game->controllers)
						{
							joynet_current_game->player[player]->local = 0;
						}
						break;
					}
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_REMOVE_PLAYER:
		{
			short player;
			JOYNET_INPUT_BUFFER * newbuffer;
			
			joynet_serialize(cp->serial_data, mp->data);
			joynet_getw(cp->serial_data, &player);
			if(joynet_current_game->player[player]->playing)
			{
				joynet_current_game->player[player]->playing = 0;
				joynet_current_game->player_count--;
				switch(joynet_current_game->type)
				{
					case JOYNET_GAME_TYPE_MOUSE:
					{
						break;
					}
					case JOYNET_GAME_TYPE_MICE:
					{
						newbuffer = malloc(sizeof(JOYNET_INPUT_BUFFER));
						if(newbuffer)
						{
							newbuffer->frame_size = joynet_current_game->controller_axes * 2 + (joynet_current_game->controller_buttons > 0 ? 1 : 0);
							joynet_current_game->input_buffer->data = malloc(joynet_current_game->input_buffer->frame_size * joynet_current_game->max_buffer_frames);
							for(i = 0; i < joynet_current_game->input_buffer->frames; i++)
							{
							}
						}
						break;
					}
					case JOYNET_GAME_TYPE_CONTROLLERS:
					{
						char idata[256] = {0};
						
						/* if we are playing, resize input buffer */
						if(joynet_current_game->state == JOYNET_GAME_STATE_PLAYING && joynet_current_game->input_buffer)
						{
							newbuffer = malloc(sizeof(JOYNET_INPUT_BUFFER));
							if(newbuffer)
							{
								int bsize = 0;
								if(joynet_current_game->controller_buttons > 0)
								{
									bsize++;
								}
								if(joynet_current_game->controller_buttons > 8)
								{
									bsize++;
								}
								newbuffer->frame_size = joynet_current_game->player_count * (joynet_current_game->controller_axes + bsize);
								newbuffer->data = malloc(newbuffer->frame_size * joynet_current_game->max_buffer_frames);
								newbuffer->frames = 0;
								newbuffer->read_pos = 0;
								newbuffer->write_pos = 0;
								newbuffer->previous_write_pos = 0;
								
								/* copy old input data to new buffer */
								for(i = 0; i < joynet_current_game->input_buffer->frames; i++)
								{
									for(j = 0; j < joynet_current_game->players; j++)
									{
										/* read one controller's worth of data */
										if(j == player || joynet_current_game->player[j]->playing)
										{
											joynet_serialize(cp->serial_data, idata);
											joynet_read(cp->serial_data, &(joynet_current_game->input_buffer->data[joynet_current_game->input_buffer->read_pos]), joynet_current_game->controller_axes + bsize);
											joynet_current_game->input_buffer->read_pos += joynet_current_game->controller_axes + bsize;
											if(joynet_current_game->input_buffer->read_pos >= joynet_current_game->input_buffer->frame_size * joynet_current_game->max_buffer_frames)
											{
												joynet_current_game->input_buffer->read_pos = 0;
											}
										}
										
										/* put data into new buffer */
										if(j != player && joynet_current_game->player[j]->playing)
										{
											joynet_serialize(cp->serial_data, &newbuffer->data[newbuffer->write_pos]);
											joynet_write(cp->serial_data, idata, joynet_current_game->controller_axes + bsize);
											newbuffer->write_pos += joynet_current_game->controller_axes + bsize;
										}
									}
								}
								free(joynet_current_game->input_buffer);
								joynet_current_game->input_buffer = newbuffer;
							}
						}
						break;
					}
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_ASSIGN_MASTER:
		{
			joynet_current_game->client->master = 1;
			break;
		}
		case JOYNET_GAME_MESSAGE_UPDATE_PLAYER_OPTIONS:
		{
			long option;
			short player;
			short length;
			short options;
			
			joynet_serialize(cp->serial_data, mp->data);
			joynet_getw(cp->serial_data, &player);
			joynet_getw(cp->serial_data, &length);
			joynet_read(cp->serial_data, joynet_current_game->player[player]->name, length);
			joynet_getw(cp->serial_data, &options);
			for(i = 0; i < options; i++)
			{
				joynet_getl(cp->serial_data, &option);
				*joynet_current_game->player[player]->option[i] = option;
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_UPDATE_OPTION:
		{
			short opt_num;
			
			joynet_serialize(cp->serial_data, mp->data);
			joynet_getw(cp->serial_data, &opt_num);
			joynet_getl(cp->serial_data, (long *)joynet_current_game->option[opt_num]);
			break;
		}
		case JOYNET_GAME_MESSAGE_UPDATE_OPTIONS:
		{
			long option;
			short count;
			
			joynet_serialize(cp->serial_data, mp->data);
			joynet_getw(cp->serial_data, &count);
			for(i = 0; i < count; i++)
			{
				joynet_getl(cp->serial_data, &option);
				*joynet_current_game->option[i] = option;
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_UPLOAD_PLAYER_CONTENT:
		{
			short list;
			short items;
			joynet_serialize(cp->serial_data, mp->data);
			joynet_getw(cp->serial_data, &list);
			joynet_getw(cp->serial_data, &items);
			if(!joynet_current_game->content_list[list])
			{
				joynet_current_game->content_list[list] = malloc(sizeof(JOYNET_CONTENT_LIST));
			}
			for(i = 0; i < items; i++)
			{
				joynet_getl(cp->serial_data, (long *)(&joynet_current_game->content_list[list]->hash[i]));
			}
			joynet_current_game->content_list[list]->count = items;
			break;
		}
		case JOYNET_GAME_MESSAGE_SELECT_PLAYER_CONTENT:
		{
			short player;
			short list;
			unsigned long hash;
			
			joynet_serialize(cp->serial_data, mp->data);
			joynet_getw(cp->serial_data, &player);
			joynet_getw(cp->serial_data, &list);
			joynet_getl(cp->serial_data, (long *)(&hash));
			
			/* see if the requested content is in the master list */
			joynet_current_game->player[player]->selected_content[list] = hash;
			for(i = 0; i < joynet_current_game->local_content_list[list]->count; i++)
			{
				if(joynet_current_game->local_content_list[list]->hash[i] == hash)
				{
					joynet_current_game->player[player]->selected_content_index[list] = i;
					break;
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_SPECTATE:
		{
			joynet_current_game->client->spectating = 1;
			break;
		}
		case JOYNET_GAME_MESSAGE_LEAVE:
		{
			joynet_current_game->client->spectating = 0;
			break;
		}
		case JOYNET_GAME_MESSAGE_START:
		{
			joynet_current_game->state = JOYNET_GAME_STATE_PLAYING;
			switch(joynet_current_game->type)
			{
				case JOYNET_GAME_TYPE_MOUSE:
				{
					joynet_current_game->input_buffer = malloc(sizeof(JOYNET_INPUT_BUFFER));
					if(joynet_current_game->input_buffer)
					{
						joynet_current_game->input_buffer->frame_size = joynet_current_game->controller_axes * 2 + (joynet_current_game->controller_buttons > 0 ? 1 : 0);
						joynet_current_game->input_buffer->data = malloc(joynet_current_game->input_buffer->frame_size * joynet_current_game->max_buffer_frames);
					}
					break;
				}
				case JOYNET_GAME_TYPE_CONTROLLERS:
				{
					joynet_current_game->input_buffer = malloc(sizeof(JOYNET_INPUT_BUFFER));
					if(joynet_current_game->input_buffer)
					{
						int bsize = 0;
						if(joynet_current_game->controller_buttons > 0)
						{
							bsize++;
						}
						if(joynet_current_game->controller_buttons > 8)
						{
							bsize++;
						}
						joynet_current_game->input_buffer->frame_size = joynet_current_game->player_count * (joynet_current_game->controller_axes + bsize);
						joynet_current_game->input_buffer->data = malloc(joynet_current_game->input_buffer->frame_size * joynet_current_game->max_buffer_frames);
					}
					if(joynet_current_game->type == JOYNET_GAME_TYPE_CONTROLLERS)
					{
						for(i = 0; i < joynet_current_game->controllers; i++)
						{
							joynet_current_game->controller_sort_data[i].port = joynet_current_game->controller[i]->port;
							joynet_current_game->controller_sort_data[i].index = i;
						}
					}
					else if(joynet_current_game->type == JOYNET_GAME_TYPE_MOUSE)
					{
						for(i = 0; i < joynet_current_game->controllers; i++)
						{
							joynet_current_game->controller_sort_data[i].port = joynet_current_game->mouse[i]->port;
							joynet_current_game->controller_sort_data[i].index = i;
						}
					}
					qsort(joynet_current_game->controller_sort_data, joynet_current_game->controllers, sizeof(JOYNET_CONTROLLER_SORT_DATA), joynet_qsort_controllers);
					break;
				}
			}
			joynet_current_game->input_buffer->frames = 0;
			joynet_current_game->input_buffer->read_pos = 0;
			joynet_current_game->input_buffer->write_pos = 0;
			joynet_current_game->input_buffer->previous_write_pos = 0;
			joynet_current_game->input_buffer->filled_frames = 0;
			joynet_current_game->input_buffer->read_frames = 0;
			break;
		}
		case JOYNET_GAME_MESSAGE_PAUSE:
		{
			joynet_current_game->state = JOYNET_GAME_STATE_PAUSED;
			joynet_current_game->input_buffer->frames = 0;
			joynet_current_game->input_buffer->read_pos = 0;
			joynet_current_game->input_buffer->write_pos = 0;
			joynet_current_game->input_buffer->previous_write_pos = 0;
			break;
		}
		case JOYNET_GAME_MESSAGE_RESUME:
		{
			joynet_current_game->state = JOYNET_GAME_STATE_PLAYING;
			joynet_current_game->input_buffer->frames = 0;
			joynet_current_game->input_buffer->read_pos = 0;
			joynet_current_game->input_buffer->write_pos = 0;
			joynet_current_game->input_buffer->previous_write_pos = 0;
			break;
		}
		case JOYNET_GAME_MESSAGE_END:
		{
			joynet_current_game->state = JOYNET_GAME_STATE_OFF;
			if(joynet_current_game->input_buffer)
			{
				free(joynet_current_game->input_buffer);
				joynet_current_game->input_buffer = NULL;
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_INPUT:
		{
			ENetPacket * pp;
			
			switch(joynet_current_game->type)
			{
				case JOYNET_GAME_TYPE_MOUSE:
				{
					memcpy(&joynet_current_game->input_buffer->data[joynet_current_game->input_buffer->write_pos], mp->data, joynet_current_game->input_buffer->frame_size);
					joynet_current_game->input_buffer->previous_write_pos = joynet_current_game->input_buffer->write_pos;
					joynet_current_game->input_buffer->write_pos += joynet_current_game->input_buffer->frame_size;
					if(joynet_current_game->input_buffer->write_pos >= joynet_current_game->max_buffer_frames * joynet_current_game->input_buffer->frame_size)
					{
						joynet_current_game->input_buffer->write_pos = 0;
					}
					joynet_current_game->input_buffer->frames++;
					joynet_current_game->input_buffer->filled_frames++;
					break;
				}
				case JOYNET_GAME_TYPE_CONTROLLERS:
				{
					memcpy(&joynet_current_game->input_buffer->data[joynet_current_game->input_buffer->write_pos], mp->data, joynet_current_game->input_buffer->frame_size);
					joynet_current_game->input_buffer->previous_write_pos = joynet_current_game->input_buffer->write_pos;
					joynet_current_game->input_buffer->write_pos += joynet_current_game->input_buffer->frame_size;
					if(joynet_current_game->input_buffer->write_pos >= joynet_current_game->max_buffer_frames * joynet_current_game->input_buffer->frame_size)
					{
						joynet_current_game->input_buffer->write_pos = 0;
					}
					joynet_current_game->input_buffer->frames++;
					joynet_current_game->input_buffer->filled_frames++;
					
					/* if we have exceeded the size of the buffer, the game will be desynced for this player,
					 * let the server know this has happened */
					if(joynet_current_game->input_buffer->frames > joynet_current_game->max_buffer_frames)
					{
						pp = joynet_create_packet(JOYNET_GAME_MESSAGE_INPUT_OVERFLOW, NULL);
						enet_peer_send(joynet_current_game->client->peer, JOYNET_CHANNEL_GAME, pp);
					}
					break;
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_NO_INPUT:
		{
			switch(joynet_current_game->type)
			{
				case JOYNET_GAME_TYPE_MOUSE:
				{
					/* no data received yet, build a frame from scratch */
					if(joynet_current_game->input_buffer->filled_frames == 0)
					{
						joynet_serialize(cp->serial_data, &joynet_current_game->input_buffer->data[joynet_current_game->input_buffer->write_pos]);
						if(joynet_current_game->controller_axes > 0)
						{
							joynet_putw(cp->serial_data, 0);
						}
						if(joynet_current_game->controller_axes > 1)
						{
							joynet_putw(cp->serial_data, 0);
						}
						if(joynet_current_game->controller_axes > 2)
						{
							joynet_putw(cp->serial_data, 0);
						}
						if(joynet_current_game->controller_buttons > 0)
						{
							joynet_putc(cp->serial_data, 0);
						}
					}
					
					/* use previous input frame data */
					else
					{
						memcpy(&joynet_current_game->input_buffer->data[joynet_current_game->input_buffer->write_pos], &joynet_current_game->input_buffer->data[joynet_current_game->input_buffer->previous_write_pos], joynet_current_game->input_buffer->frame_size);
					}
					joynet_current_game->input_buffer->write_pos += joynet_current_game->input_buffer->frame_size;
					if(joynet_current_game->input_buffer->write_pos >= joynet_current_game->max_buffer_frames * joynet_current_game->input_buffer->frame_size)
					{
						joynet_current_game->input_buffer->write_pos = 0;
					}
					joynet_current_game->input_buffer->frames++;
					joynet_current_game->input_buffer->filled_frames++;
					break;
				}
			}
			break;
		}
		case JOYNET_GAME_MESSAGE_SELECT_PLAYER:
		{
			short player;

			joynet_serialize(cp->serial_data, mp->data);
			joynet_getw(cp->serial_data, &player);
			joynet_current_game->current_player = player;
			break;
		}
	}
}
