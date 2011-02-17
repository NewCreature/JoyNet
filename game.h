#ifndef JOYNET_GAME_H
#define JOYNET_GAME_H

#include "joynet.h"
#include "message.h"

/* what type of game are we? */
#define JOYNET_GAME_TYPE_CONTROLLERS  0
#define JOYNET_GAME_TYPE_MICE         1 // each player has own mouse
#define JOYNET_GAME_TYPE_MOUSE        2 // share a mouse

#define JOYNET_GAME_MAX_PLAYERS            256
#define JOYNET_GAME_MAX_CONTROLLERS          8
#define JOYNET_GAME_MAX_MICE                 8
#define JOYNET_GAME_MAX_OPTIONS            128
#define JOYNET_GAME_MAX_PLAYER_OPTIONS     128
#define JOYNET_GAME_MAX_CONTROLLER_BUTTONS  16
#define JOYNET_GAME_MAX_CONTROLLER_AXES      8
#define JOYNET_GAME_MAX_CONTENT_LISTS        8
#define JOYNET_GAME_MAX_CONTENT_LIST_SIZE 1024

#define JOYNET_GAME_STATE_OFF         0
#define JOYNET_GAME_STATE_PLAYING     1
#define JOYNET_GAME_STATE_PAUSED      2

typedef struct
{
	
	int port; // which controller "port" is this plugged into
	int backport; // which local controller is connected to this controller
	
	/* joysticks and such */
	char button[JOYNET_GAME_MAX_CONTROLLER_BUTTONS];
	float axis[JOYNET_GAME_MAX_CONTROLLER_AXES]; // convert these to char to upload
	
	/* mouse */
	short mouse_x;
	short mouse_y;
	short mouse_z;
	char mouse_b;
	
	/* button bits for controllers */
	char bits[2];
	
} JOYNET_CONTROLLER;

typedef struct
{
	
	JOYNET_CONTROLLER * controller;
	
	char name[256];
	unsigned long selected_content[JOYNET_GAME_MAX_CONTENT_LISTS]; // 0 means no selection
	int selected_content_index[JOYNET_GAME_MAX_CONTENT_LISTS];     // index to content in local content list
	
	/* options */
	int * option[JOYNET_GAME_MAX_PLAYER_OPTIONS];
	int server_option[JOYNET_GAME_MAX_PLAYER_OPTIONS];
	int options;
	
	int local; // this player is local
	int client;
	
} JOYNET_PLAYER;

typedef struct
{
	
	char * data;
	
	int read_pos;
	int write_pos;
	int previous_write_pos;
	int frames;
	int frame_size;
	
} JOYNET_INPUT_BUFFER;

typedef struct
{
	
	unsigned long hash[JOYNET_GAME_MAX_CONTENT_LIST_SIZE];
	int count;
	
} JOYNET_CONTENT_LIST;

typedef struct
{
	
	int port;
	int index;
	
} JOYNET_CONTROLLER_SORT_DATA;

typedef struct
{
	int type;
	int state;
	char name[256];
	
	/* players */
	JOYNET_PLAYER * player[JOYNET_GAME_MAX_PLAYERS];
	int players;
	int player_count;
	
	/* local controllers */
	JOYNET_CONTROLLER * controller[JOYNET_GAME_MAX_CONTROLLERS];
	
	/* options */
	int * option[JOYNET_GAME_MAX_OPTIONS];
	int server_option[JOYNET_GAME_MAX_OPTIONS];
	int options;
	
	/* content */
	JOYNET_CONTENT_LIST * local_content_list[JOYNET_GAME_MAX_CONTENT_LISTS];
	JOYNET_CONTENT_LIST * content_list[JOYNET_GAME_MAX_CONTENT_LISTS]; // this is the content that everyone has
	
	/* networking */
	JOYNET_SERVER * server;
	JOYNET_CLIENT * client;
	int(*callback)(JOYNET_MESSAGE * mp); // used for local play
	
	/* server stuff */
	JOYNET_CONTENT_LIST * server_content_list[JOYNET_GAME_MAX_PLAYERS][JOYNET_GAME_MAX_CONTENT_LISTS];
	JOYNET_CONTENT_LIST * server_master_content_list[JOYNET_GAME_MAX_CONTENT_LISTS]; // the content that everyone has
	
	/* used to encode/decode input messages */
	int controllers;
	int controller_buttons;
	int controller_axes;
	int max_buffer_frames;
	JOYNET_INPUT_BUFFER * input_buffer;
	int current_player;
	int current_client;
	int received_input;
	JOYNET_CONTROLLER_SORT_DATA controller_sort_data[JOYNET_GAME_MAX_CONTROLLERS];
	
	JOYNET_SERIAL_DATA * serial_data;
	
} JOYNET_GAME;

extern JOYNET_GAME * joynet_current_game;
extern JOYNET_GAME * joynet_current_server_game;

int joynet_qsort_controllers(const void * e1, const void * e2);
JOYNET_GAME * joynet_create_game(char * name, int type, int max_players, int max_controllers, int(*callback)(JOYNET_MESSAGE * mp));
void joynet_setup_game_controllers(JOYNET_GAME * gp, int buttons, int axes, int buffer_size);
void joynet_destroy_game(JOYNET_GAME * gp);
void joynet_select_game(JOYNET_GAME * gp);
int joynet_open_game_server(JOYNET_GAME * gp, JOYNET_SERVER * sp, int port, int clients);
int joynet_connect_to_game_server(JOYNET_GAME * gp, JOYNET_CLIENT * cp, char * address, int port);
void joynet_disconnect_from_game_server(JOYNET_GAME * gp, JOYNET_CLIENT * cp);

/* player management */
void joynet_connect_to_game(JOYNET_GAME * gp, short controller, short player);
void joynet_disconnect_from_game(JOYNET_GAME * gp, short controller, short player);
void joynet_watch_game(JOYNET_GAME * gp);
void joynet_leave_game(JOYNET_GAME * gp);
void joynet_add_game_option(JOYNET_GAME * gp, int * op);
void joynet_add_player_option(JOYNET_GAME * gp, int player, int * op);
void joynet_update_game_options(JOYNET_GAME * gp);
void joynet_update_player_options(JOYNET_GAME * gp, int player);

/* content management */
void joynet_add_game_content(JOYNET_GAME * gp, int list, unsigned long hash);
void joynet_select_game_content(JOYNET_GAME * gp, int player, int list, unsigned long hash);

/* game functions */
void joynet_start_game(JOYNET_GAME * gp);
void joynet_pause_game(JOYNET_GAME * gp);
void joynet_resume_game(JOYNET_GAME * gp);
void joynet_end_game(JOYNET_GAME * gp);
void joynet_select_player(JOYNET_GAME * gp, int player); // for shared-mouse turn-based games
int joynet_game_input_frames(JOYNET_GAME * gp);
void joynet_update_game(JOYNET_GAME * gp, int process);
void joynet_update_game_server(JOYNET_SERVER * sp, JOYNET_GAME * gp);
void joynet_game_logic(JOYNET_GAME * gp);

#endif