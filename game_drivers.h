#ifndef JOYNET_GAME_DRIVERS_H
#define JOYNET_GAME_DRIVERS_H

void joynet_game_driver_mouse_init_input(JOYNET_GAME * gp);
int joynet_game_driver_mouse_encode_input(JOYNET_GAME * gp, char * buffer);
void joynet_game_driver_mouse_update_game_server(JOYNET_SERVER * sp, JOYNET_GAME * gp);
void joynet_game_driver_mouse_game_logic(JOYNET_GAME * gp);

void joynet_game_driver_mice_init_input(JOYNET_GAME * gp);
int joynet_game_driver_mice_encode_input(JOYNET_GAME * gp, char * buffer);
void joynet_game_driver_mice_update_game_server(JOYNET_SERVER * sp, JOYNET_GAME * gp);
void joynet_game_driver_mice_game_logic(JOYNET_GAME * gp);

void joynet_game_driver_controller_init_input(JOYNET_GAME * gp);
int joynet_game_driver_controller_encode_input(JOYNET_GAME * gp, char * buffer);
void joynet_game_driver_controller_update_game_server(JOYNET_SERVER * sp, JOYNET_GAME * gp);
void joynet_game_driver_controller_game_logic(JOYNET_GAME * gp);

#endif
