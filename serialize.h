#ifndef JOYNET_SERIALIZE_H
#define JOYNET_SERIALIZE_H

#include <stdint.h>
#include "joynet.h"

JOYNET_SERIAL_DATA * joynet_create_serial_data(void);
void joynet_destroy_serial_data(JOYNET_SERIAL_DATA * dp);
void joynet_serialize(JOYNET_SERIAL_DATA * dp, char * ptr);
void joynet_read(JOYNET_SERIAL_DATA * dp, char * dest, int num);
void joynet_write(JOYNET_SERIAL_DATA * dp, char * dest, int num);
void joynet_getc(JOYNET_SERIAL_DATA * dp, char * c);
void joynet_putc(JOYNET_SERIAL_DATA * dp, char c);
void joynet_getw(JOYNET_SERIAL_DATA * dp, int16_t * c);
void joynet_putw(JOYNET_SERIAL_DATA * dp, int16_t c);
void joynet_getuw(JOYNET_SERIAL_DATA * dp, uint16_t * c);
void joynet_putuw(JOYNET_SERIAL_DATA * dp, uint16_t c);
void joynet_getl(JOYNET_SERIAL_DATA * dp, int32_t * c);
void joynet_putl(JOYNET_SERIAL_DATA * dp, int32_t c);
void joynet_getul(JOYNET_SERIAL_DATA * dp, uint32_t * c);
void joynet_putul(JOYNET_SERIAL_DATA * dp, uint32_t c);
int joynet_get_serial_size(JOYNET_SERIAL_DATA * dp);

#endif
