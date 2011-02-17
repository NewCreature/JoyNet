#include <string.h>
#include "joynet.h"

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

void joynet_getw(JOYNET_SERIAL_DATA * dp, short * c)
{
	memcpy(c, dp->pointer, sizeof(short));
	dp->pointer += sizeof(short);
	dp->size += sizeof(short);
}

void joynet_putw(JOYNET_SERIAL_DATA * dp, short c)
{
	memcpy(dp->pointer, &c, sizeof(short));
	dp->pointer += sizeof(short);
	dp->size += sizeof(short);
}

void joynet_getl(JOYNET_SERIAL_DATA * dp, long * c)
{
	memcpy(c, dp->pointer, sizeof(long));
	dp->pointer += sizeof(long);
	dp->size += sizeof(long);
}

void joynet_putl(JOYNET_SERIAL_DATA * dp, long c)
{
	memcpy(dp->pointer, &c, sizeof(long));
	dp->pointer += sizeof(long);
	dp->size += sizeof(long);
}

int joynet_get_serial_size(JOYNET_SERIAL_DATA * dp)
{
	return dp->size;
}
