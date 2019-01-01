#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "enet/include/enet/enet.h"
#include "joynet.h"
#include "serialize.h"

static long joynet_random_seed = 0;
int joynet_swap_endianess = 0;

static void joynet_detect_endianess(void)
{
	unsigned long Value32;
	unsigned char * VPtr = (unsigned char *)&Value32;

	VPtr[0] = VPtr[1] = VPtr[2] = 0; VPtr[3] = 1;

	if(Value32 == 1)
	{
		joynet_swap_endianess = 1; // big endian
	}
	else
	{
		joynet_swap_endianess = 0; // little endian
	}
}

int joynet_init(void)
{
	if(enet_initialize() != 0)
	{
		return 0;
	}
	joynet_detect_endianess();
	return 1;
}

void joynet_exit(void)
{
	enet_deinitialize();
}

void joynet_ping(const char * url, int port)
{
	ENetAddress address;
	ENetHost * host;
	ENetPeer * peer;

	if(enet_address_set_host(&address, url) < 0)
	{
		return;
	}
	address.port = port;
	host = enet_host_create(NULL, 1, 0, 0, 0);
	if(!host)
	{
		return;
	}
	peer = enet_host_connect(host, &address, 4, 0);
	if(!peer)
	{
		return;
	}
	enet_host_destroy(host);
}

void joynet_srand(unsigned int seed)
{
	joynet_random_seed = (long)seed;
}

#define JOYNET_RAND_MAX 0xFFFF
int joynet_rand(void)
{
	return (((joynet_random_seed = joynet_random_seed * 214013L + 2531011L) >> 16) & JOYNET_RAND_MAX);
}

#define JOYNET_RS_SCALE (1.0 / (1.0 + JOYNET_RAND_MAX))
double joynet_drand(void)
{
	double d;
	do
	{
		d = (((joynet_rand() * JOYNET_RS_SCALE) + joynet_rand()) * JOYNET_RS_SCALE + joynet_rand()) * JOYNET_RS_SCALE;
	} while (d >= 1); /* round off */
	return d;
}
