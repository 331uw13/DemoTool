#ifndef DEMOTOOL_H
#define DEMOTOOL_H

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef long unsigned int  u64;


// Global flags can be set or unset with 'dt_set_flag()'
// and check if they are set with 'dt_get_flag()'
// NOTE: some flags may not affect anything if changed after calling 'dt_play()' or 'dt_initialize()'
#define DT_INITIALIZED      0x1
#define DT_NO_AUDIO         0x2
#define DT_PAUSED           0x4
#define DT_FULLSCREEN       0x8
#define DT_DISABLE_CURSOR   0x10
#define DT_USE_VSYNC        0x20

// Some default values for things..
#define DT_DEFAULT_WINDOW_WIDTH      1000
#define DT_DEFAULT_WINDOW_HEIGHT     800

#define DT_NUM_COMMON_DATA 2  // Used to check if user calls 'dt_set()' or 'dt_get()'
// to check that parameter 'i' is in range so it wont crash if its too big by mistake.

// Can be used with 'dt_set()' and 'dt_get()' for 'u32 i'
#define DT_WINDOW_W  0  // Window width.
#define DT_WINDOW_H  1  // Window height.

/*
struct dt_timep {
	int     id;       // Position in array.
	u8      playing;  // Is it active right now?
	u8      played;   // Is it now already played?
};
*/
void dt_initialize(char* title, u32 flags);
void dt_play(char* music_path, void(*frame_update_callback)(double));
void dt_stop();
void dt_cleanup();
double dt_get_time();
//double dt_random(double min, double max);

u32  dt_create_shader(char* fs_src);
void dt_delete_shader(u32 id);
void dt_use_shader(u32 id);
/*
void dt_alloc_timepoints(int count);
void dt_add_timepoint(double time, int id);
void dt_delete_timepoint(int id);
void dt_get_timepoint(struct dt_timep* out);
*/
u8   dt_get_flag(u32 flag);
void dt_set_flag(u32 flag, u8 b);

int  dt_get(u32 i);
void dt_set(u32 i, int value);


// These affect the current shader that was set with 'dt_use_shader()'
void dt_shader_uniform_1f(char* name, double x);
void dt_shader_uniform_2f(char* name, double x, double y);
void dt_shader_uniform_3f(char* name, double x, double y, double z);
void dt_shader_uniform_4f(char* name, double x, double y, double z, double w);
void dt_shader_uniform_1i(char* name, int x);
void dt_shader_uniform_2i(char* name, int x, int y);
void dt_shader_uniform_3i(char* name, int x, int y, int z);
void dt_shader_uniform_4i(char* name, int x, int y, int z, int w);

#endif
