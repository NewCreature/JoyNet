#ifndef JOYNET_CHAT_H
#define JOYNET_CHAT_H

#include "joynet.h"
#include "message.h"

void joynet_handle_server_chat_message(JOYNET_SERVER * sp, JOYNET_MESSAGE * mp);
void joynet_handle_client_chat_message(JOYNET_CLIENT * cp, JOYNET_MESSAGE * mp);
void joynet_set_chat_callback(int(*callback)(char * user, char * message));

#endif
