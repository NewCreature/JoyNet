#ifndef JOYNET_GAME_MESSAGE_H
#define JOYNET_GAME_MESSAGE_H

#include "joynet.h"

/* player setup */
#define JOYNET_GAME_MESSAGE_CONNECT                0
#define JOYNET_GAME_MESSAGE_CONNECT_FAIL           1
#define JOYNET_GAME_MESSAGE_DISCONNECT             2
#define JOYNET_GAME_MESSAGE_ADD_PLAYER             3
#define JOYNET_GAME_MESSAGE_REMOVE_PLAYER          4
#define JOYNET_GAME_MESSAGE_CHECK_PLAYERS          5
#define JOYNET_GAME_MESSAGE_UPDATE_PLAYER_OPTIONS  6
#define JOYNET_GAME_MESSAGE_ASSIGN_MASTER          7
#define JOYNET_GAME_MESSAGE_SPECTATE               8
#define JOYNET_GAME_MESSAGE_SPECTATE_FAIL          9
#define JOYNET_GAME_MESSAGE_LEAVE                 10
#define JOYNET_GAME_MESSAGE_UPLOAD_PLAYER_CONTENT 11
#define JOYNET_GAME_MESSAGE_SELECT_PLAYER_CONTENT 12

/* game setup */
#define JOYNET_GAME_MESSAGE_UPDATE_OPTION         23
#define JOYNET_GAME_MESSAGE_UPDATE_OPTIONS        13
#define JOYNET_GAME_MESSAGE_START                 14
#define JOYNET_GAME_MESSAGE_PAUSE                 15
#define JOYNET_GAME_MESSAGE_RESUME                16
#define JOYNET_GAME_MESSAGE_END                   17

/* input */
#define JOYNET_GAME_MESSAGE_INPUT                 18
#define JOYNET_GAME_MESSAGE_NO_INPUT              19
#define JOYNET_GAME_MESSAGE_SELECT_PLAYER         20
#define JOYNET_GAME_MESSAGE_INPUT_OVERFLOW        24

#define JOYNET_GAME_MESSAGE_CHECK_ID              21
#define JOYNET_GAME_MESSAGE_CHECK_ID_FAIL         22

#define JOYNET_GAME_MESSAGE_USER                  64

int joynet_server_internal_game_callback(ENetEvent * ep);
void joynet_handle_server_game_message(JOYNET_SERVER * sp, JOYNET_MESSAGE * mp);
void joynet_handle_client_game_message(JOYNET_CLIENT * cp, JOYNET_MESSAGE * mp);

#endif
