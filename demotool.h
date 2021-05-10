#ifndef DEMOTOOL_H
#define DEMOTOOL_H

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef long unsigned int  u64;


struct shader_effect {
	u32     program;
	double  duration;
};

void dt_initialize(char* title);
void dt_quit();

u32  dt_create_program(char* fs_src);
void dt_shader_include(char** src, u64 length);


void dt_play(struct shader_effect* effects, u32 effects_count, char* music_path);


#endif
