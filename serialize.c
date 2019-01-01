#include <string.h>
#include "joynet.h"
#include <stdint.h>

/* Integer types larger than 1 byte are assumed to be transmitted as little
   endian. Big endian platforms should convert to little endian before
   transmitting. */

//! Byte swap unsigned short
static uint16_t swap_uint16( uint16_t val )
{
    return (val << 8) | (val >> 8 );
}

//! Byte swap short
static int16_t swap_int16( int16_t val )
{
    return (val << 8) | ((val >> 8) & 0xFF);
}

//! Byte swap unsigned int
static uint32_t swap_uint32( uint32_t val )
{
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF );
    return (val << 16) | (val >> 16);
}

//! Byte swap int
static int32_t swap_int32( int32_t val )
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF );
    return (val << 16) | ((val >> 16) & 0xFFFF);
}

static int16_t swap_w(int16_t val)
{
	if(joynet_swap_endianess)
	{
		return swap_int16(val);
	}
	return val;
}

static uint16_t swap_uw(uint16_t val)
{
	if(joynet_swap_endianess)
	{
		return swap_uint16(val);
	}
	return val;
}

static int32_t swap_l(int32_t val)
{
	if(joynet_swap_endianess)
	{
		return swap_int32(val);
	}
	return val;
}

static uint32_t swap_ul(uint32_t val)
{
	if(joynet_swap_endianess)
	{
		return swap_uint32(val);
	}
	return val;
}

JOYNET_SERIAL_DATA * joynet_create_serial_data(void)
{
	JOYNET_SERIAL_DATA * dp = NULL;

	dp = malloc(sizeof(JOYNET_SERIAL_DATA));
	if(dp)
	{
		dp->pointer = NULL;
		dp->size = 0;
	}
	return dp;
}

void joynet_destroy_serial_data(JOYNET_SERIAL_DATA * dp)
{
	free(dp);
}

void joynet_serialize(JOYNET_SERIAL_DATA * dp, char * ptr)
{
	dp->data = ptr;
	dp->pointer = ptr;
	dp->size = 0;
}

void joynet_read(JOYNET_SERIAL_DATA * dp, char * dest, int num)
{
	memcpy(dest, dp->pointer, num);
	dp->pointer += num;
	dp->size += num;
}

void joynet_write(JOYNET_SERIAL_DATA * dp, char * dest, int num)
{
	memcpy(dp->pointer, dest, num);
	dp->pointer += num;
	dp->size += num;
}

void joynet_getc(JOYNET_SERIAL_DATA * dp, char * c)
{
	*c = dp->pointer[0];
	dp->pointer++;
	dp->size++;
}

void joynet_putc(JOYNET_SERIAL_DATA * dp, char c)
{
	dp->pointer[0] = c;
	dp->pointer++;
	dp->size++;
}

void joynet_getw(JOYNET_SERIAL_DATA * dp, int16_t * c)
{
	int16_t d;

	memcpy(&d, dp->pointer, sizeof(int16_t));
	*c = swap_w(d);
	dp->pointer += sizeof(int16_t);
	dp->size += sizeof(int16_t);
}

void joynet_putw(JOYNET_SERIAL_DATA * dp, int16_t c)
{
	int16_t d;

	d = swap_w(c);
	memcpy(dp->pointer, &d, sizeof(int16_t));
	dp->pointer += sizeof(int16_t);
	dp->size += sizeof(int16_t);
}

void joynet_getuw(JOYNET_SERIAL_DATA * dp, uint16_t * c)
{
	uint16_t d;

	memcpy(&d, dp->pointer, sizeof(uint16_t));
	*c = swap_uw(d);
	dp->pointer += sizeof(uint16_t);
	dp->size += sizeof(uint16_t);
}

void joynet_putuw(JOYNET_SERIAL_DATA * dp, uint16_t c)
{
	uint16_t d;

	d = swap_uw(c);
	memcpy(dp->pointer, &d, sizeof(uint16_t));
	dp->pointer += sizeof(uint16_t);
	dp->size += sizeof(uint16_t);
}

void joynet_getl(JOYNET_SERIAL_DATA * dp, int32_t * c)
{
	int32_t d;

	memcpy(&d, dp->pointer, sizeof(int32_t));
	*c = swap_l(d);
	dp->pointer += sizeof(int32_t);
	dp->size += sizeof(int32_t);
}

void joynet_putl(JOYNET_SERIAL_DATA * dp, int32_t c)
{
	int32_t d;

	d = swap_l(c);
	memcpy(dp->pointer, &d, sizeof(int32_t));
	dp->pointer += sizeof(int32_t);
	dp->size += sizeof(int32_t);
}

void joynet_getul(JOYNET_SERIAL_DATA * dp, uint32_t * c)
{
	uint32_t d;

	memcpy(&d, dp->pointer, sizeof(uint32_t));
	*c = swap_ul(d);
	dp->pointer += sizeof(uint32_t);
	dp->size += sizeof(uint32_t);
}

void joynet_putul(JOYNET_SERIAL_DATA * dp, uint32_t c)
{
	uint32_t d;

	d = swap_ul(c);
	memcpy(dp->pointer, &d, sizeof(uint32_t));
	dp->pointer += sizeof(uint32_t);
	dp->size += sizeof(uint32_t);
}

int joynet_get_serial_size(JOYNET_SERIAL_DATA * dp)
{
	return dp->size;
}
