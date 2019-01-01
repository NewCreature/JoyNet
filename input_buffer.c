#include <stdlib.h>
#include <memory.h>
#include "input_buffer.h"

JOYNET_INPUT_BUFFER * joynet_create_input_buffer(int frame_size, int max_frames)
{
    JOYNET_INPUT_BUFFER * ip;

    ip = malloc(sizeof(JOYNET_INPUT_BUFFER));
    if(ip)
    {
        ip->frame_size = frame_size;
        ip->max_frames = max_frames;
        ip->data = malloc(ip->frame_size * ip->max_frames);
        if(!ip->data)
        {
            free(ip);
            ip = NULL;
        }
        ip->frames = 0;
        ip->read_pos = 0;
        ip->write_pos = 0;
        ip->previous_write_pos = 0;
        ip->filled_frames = 0;
        ip->read_frames = 0;
    }
    return ip;
}

void joynet_destroy_input_buffer(JOYNET_INPUT_BUFFER * ip)
{
    free(ip->data);
    free(ip);
}

void joynet_write_input_buffer_frame(JOYNET_INPUT_BUFFER * ip, const char * data)
{
    ip->previous_write_pos = ip->write_pos;
    memcpy(&(ip->data[ip->write_pos]), data, ip->frame_size);
    ip->write_pos += ip->frame_size;
    if(ip->write_pos >= ip->frame_size * ip->max_frames)
    {
        ip->write_pos = 0;
    }
    ip->filled_frames++;
    ip->frames++;
}

void joynet_read_input_buffer_frame(JOYNET_INPUT_BUFFER * ip, char * out)
{
    memcpy(out, &ip->data[ip->read_pos], ip->frame_size);
    ip->read_pos += ip->frame_size;
    if(ip->read_pos >= ip->max_frames * ip->frame_size)
    {
        ip->read_pos = 0;
    }
    ip->read_frames++;
    ip->frames--;
}

void joynet_duplicate_input_buffer_frame(JOYNET_INPUT_BUFFER * ip)
{
    char data[1024] = {0};

    if(ip->filled_frames)
    {
        memcpy(data, &ip->data[ip->previous_write_pos], ip->frame_size);
    }
    joynet_write_input_buffer_frame(ip, data);
}

void joynet_reset_input_buffer(JOYNET_INPUT_BUFFER * ip)
{
    ip->frames = 0;
    ip->read_pos = 0;
    ip->write_pos = 0;
    ip->previous_write_pos = 0;
    ip->filled_frames = 0;
    ip->read_frames = 0;
}

int joynet_get_input_buffer_frames(JOYNET_INPUT_BUFFER * ip)
{
    return ip->frames;
}
