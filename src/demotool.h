#ifndef DEMOTOOL_H
#define DEMOTOO_H

#define STATUS_FOR_SHADER 0
#define STATUS_FOR_PROGRAM 1

#include "common.h"

void dt_start(char* title);
void dt_play(double* timeline, u64 timeline_size, char* vs_path, char* fs_path, char* music_path);
void dt_quit();

u8 status_ok(u32 id, u32 type);
u32 dt_create_shader(char* filename, u32 type);

#endif
