#ifndef JOYNET3_H
#define JOYNET3_H

#include "enet-1.3.1/include/enet/enet.h"
#include "structs.h"

#define JOYNET_CHANNEL_SYSTEM 0 // system channel used for setting up connections
#define JOYNET_CHANNEL_CHAT   1 // chat channel, send and receive chat messages
#define JOYNET_CHANNEL_GAME   2 // channel used by the game system (game start, end, pause, resume, input ticks)
#define JOYNET_CHANNEL_USER   3 // application-specific channel

#define JOYNET_DISCONNECT_SERVER_CLOSED 0
#define JOYNET_DISCONNECT_CLIENT_CLOSED 1

/* system messages */
#define JOYNET_SYSTEM_MESSAGE_WELCOME   0

/* chat messages */
#define JOYNET_CHAT_MESSAGE_SET_NAME    0
#define JOYNET_CHAT_MESSAGE_SET_GROUP   1
#define JOYNET_CHAT_MESSAGE_TALK        2
#define JOYNET_CHAT_MESSAGE_GROUP_TALK  3
#define JOYNET_CHAT_MESSAGE_VOIP        4
#define JOYNET_CHAT_MESSAGE_GROUP_VOIP  5

int joynet_init(void);
void joynet_exit(void);
void joynet_ping(const char * host, int port);
void joynet_srand(unsigned int seed);
int joynet_rand(void);
double joynet_drand(void);

/* server functions */
JOYNET_SERVER * joynet_create_server(void);
void joynet_destroy_server(JOYNET_SERVER * sp);
int joynet_open_server(JOYNET_SERVER * sp, int port, int connections);
void joynet_close_server(JOYNET_SERVER * sp);
void joynet_set_server_id(JOYNET_SERVER * sp, char * id);
void joynet_poll_server(JOYNET_SERVER * sp);
void joynet_set_server_global_callback(JOYNET_SERVER * sp, int(*callback)(ENetEvent * ep));
void joynet_set_server_channel_callback(JOYNET_SERVER * sp, int channel, int(*callback)(JOYNET_MESSAGE * mp));
int joynet_get_client_from_peer(JOYNET_SERVER * sp, ENetPeer * pp);

/* client functions */
JOYNET_CLIENT * joynet_create_client(void);
void joynet_destroy_client(JOYNET_CLIENT * cp);
int joynet_connect_client_to_server(JOYNET_CLIENT * cp, char * host, int port);
void joynet_disconnect_client_from_server(JOYNET_CLIENT * cp);
void joynet_poll_client(JOYNET_CLIENT * cp);
void joynet_send_client_chat(JOYNET_CLIENT * cp, char * message, int group);
void joynet_set_client_global_callback(JOYNET_CLIENT * cp, int(*callback)(ENetEvent * ep));
void joynet_set_client_channel_callback(JOYNET_CLIENT * cp, int channel, int(*callback)(JOYNET_MESSAGE * mp));

/* chat functions */
void joynet_set_client_group(JOYNET_CLIENT * cp, short group);
void joynet_set_client_screen_name(JOYNET_CLIENT * cp, char * screen_name);
void joynet_set_client_chat_callback(JOYNET_CLIENT * cp, int(*callback)(char * user, char * message));
void joynet_set_client_voip_callback(JOYNET_CLIENT * cp, int(*callback)(int client, void * data, int size));
void joynet_send_client_voip_data(JOYNET_CLIENT * cp, void * data, int size);

#include "game.h"
#include "game_message.h"

#endif
