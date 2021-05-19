#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

#include "demotool.h"

#define MAX(a, b) (a >= b ? a : b)
#define MIN(a, b) (a <= b ? a : b)

#define DT_SHADER_STATUS   0
#define DT_PROGRAM_STATUS  1



static GLFWwindow* window = NULL;
static u8* audio_pos     = 0;
static u32 audio_length  = 0;
static u32 default_vertex_shader = 0;

static int dt_gflags = 0;
static int dt_common_data[DT_NUM_COMMON_DATA];
static u32 dt_current_shader = 0;


// TODO: try make this better.
int dt_check_status(u32 id, u32 flag) {
	int res = 0;
	if(flag == DT_SHADER_STATUS || flag == DT_PROGRAM_STATUS) {
		char* msg = NULL;
		int msg_length = 0;

		if(flag == DT_SHADER_STATUS) {
			glGetShaderiv(id, GL_COMPILE_STATUS, &res);
			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &msg_length);
		}
		else if(flag == DT_PROGRAM_STATUS) {
			glGetProgramiv(id, GL_LINK_STATUS, &res);
			glGetProgramiv(id, GL_INFO_LOG_LENGTH, &msg_length);
		}

		if(msg_length > 0) {
			msg = malloc(msg_length);
			if(msg != NULL) {
				if(flag == DT_SHADER_STATUS) {
					glGetShaderInfoLog(id, msg_length, NULL, msg);
				}
				else if(flag == DT_PROGRAM_STATUS) {
					glGetProgramInfoLog(id, msg_length, NULL, msg);
				}
				fprintf(stderr, "\033[33m==== SHADER INFO LOG ==================\033[0m\n%s\n", msg);
			}
			else {
				fprintf(stderr, "ERROR: Failed to allocate %i bytes of memory!\n", msg_length);
			}
		}
	}

	return res;
}


void sdl_audio_callback(void* userdata, u8* stream, int bytes) {
	if(audio_length == 0 || bytes <= 0 || stream == NULL) { return; }
	bytes = ((u32)bytes > audio_length ? audio_length : (u32)bytes);

	if(dt_gflags & DT_PAUSED) { 
		memset(stream, 0, bytes);
	}
	else {
		memmove(stream, audio_pos, bytes);
		audio_length -= bytes;
		audio_pos += bytes;
	}
}

void glfw_key_callback(GLFWwindow* win, int key, int scancode, int action, int mods) {
	if(action == GLFW_PRESS) {
		switch(key) {

			// Pause
			case GLFW_KEY_P:
			case GLFW_KEY_SPACE:
				dt_set_flag(DT_PAUSED, !(dt_gflags & DT_PAUSED));
				break;

			// Quit
			case GLFW_KEY_ESCAPE:
			case GLFW_KEY_END:
				glfwSetWindowShouldClose(win, 1);
				break;
		
			default: break;
		}
	}
}


u32 dt_create_shader(char* fs_src) {
	u32 fs = 0;
	u32 program = 0;
	
	if(window != NULL && (dt_gflags & DT_INITIALIZED)) {
		if(fs_src != NULL) {
			
			if(default_vertex_shader == 0) {
				fprintf(stderr, "Failed to create program because of default vertex shader is not compiled!\n");
				goto finish;
			}

			// Create fragment shader for program.

			fs = glCreateShader(GL_FRAGMENT_SHADER);
			if(fs == 0) {
				fprintf(stderr, "Failed to create fragment shader!\n");
				goto finish;
			}

			glShaderSource(fs, 1, (const char**)&fs_src, NULL);
			glCompileShader(fs);

			if(!dt_check_status(fs, DT_SHADER_STATUS)) {
				fprintf(stderr, "Failed to compile fragment shader!\n");
				goto finish;
			}

			// Create the program.

			program = glCreateProgram();
			if(program == 0) {
				fprintf(stderr, "Failed to create program!\n");
				goto finish;
			}

			glAttachShader(program, default_vertex_shader);
			glAttachShader(program, fs);
			glLinkProgram(program);

			if(!dt_check_status(program, DT_PROGRAM_STATUS)) {
				fprintf(stderr, "Failed to link program!\n");
				glDeleteProgram(program);
			}
		}
	}
	else {
		fprintf(stderr, "ERROR: Cant create shader because not initialized! Exit.\n");
		exit(-1);
	}

finish:

	if(fs > 0) {
		// Already linked to program or failed to create program.
		glDeleteShader(fs);
	}

	return program;
}

void dt_delete_shader(u32 id) {
	if(id > 1) {
		glDeleteProgram(id);
		if(id == dt_current_shader) {
			dt_current_shader = 0;
		}
	}
}

void dt_use_shader(u32 id) {
	if(id > 1) {
		glUseProgram(id);
		dt_current_shader = id;
	}
}

void dt_initialize(char* title, u32 flags) {
	dt_gflags = flags;
	dt_current_shader = 0;

	if(!glfwInit()) {
		fprintf(stderr, "ERROR: Failed to initialize glfw!\n");
		exit(-1);
		return;
	}
	
	if(dt_gflags & DT_FULLSCREEN) {
		glfwWindowHint(GLFW_MAXIMIZED, 1);
	}
	if(dt_gflags & DT_DISABLE_CURSOR) {
		glfwWindowHint(GLFW_FOCUS_ON_SHOW, 1);
	}
	glfwWindowHint(GLFW_DECORATED, 0);
	glfwWindowHint(GLFW_RESIZABLE, 0);
	glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	int monitor_x = 0;
	int monitor_y = 0;
	int monitor_w = 0;
	int monitor_h = 0;
	glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &monitor_x, &monitor_y, &monitor_w, &monitor_h);
	
	int win_req_w = (dt_gflags & DT_FULLSCREEN) ? monitor_w : dt_common_data[DT_WINDOW_W];
	int win_req_h = (dt_gflags & DT_FULLSCREEN) ? monitor_h : dt_common_data[DT_WINDOW_H];
	
	if(win_req_w <= 0) {
		win_req_w = DT_DEFAULT_WINDOW_WIDTH;
	}

	if(win_req_h <= 0) {
		win_req_h = DT_DEFAULT_WINDOW_HEIGHT;
	}


	window = glfwCreateWindow(win_req_w, win_req_h, title, NULL, NULL);

	if(window == NULL) {
		fprintf(stderr, "ERROR: Failed to create window (width: %i, height: %i)\n", win_req_w, win_req_h);
		glfwTerminate();
		exit(-1);
		return;
	}

	int* win_w = &dt_common_data[DT_WINDOW_W];
	int* win_h = &dt_common_data[DT_WINDOW_H];

	glfwMakeContextCurrent(window);
	glfwGetWindowSize(window, win_w, win_h);
	
	const int win_req_x = (dt_gflags & DT_FULLSCREEN) ? monitor_x : (monitor_w/2-*win_w/2);
	const int win_req_y = (dt_gflags & DT_FULLSCREEN) ? monitor_y : (monitor_h/2-*win_h/2);
	
	glfwSetWindowPos(window, win_req_x, win_req_y);
	glfwSetKeyCallback(window, glfw_key_callback);
	glfwSwapInterval((dt_gflags & DT_USE_VSYNC));

	glViewport(0.0, 0.0, *win_w, *win_h);
	glClearColor(0.0, 0.0, 0.0, 1.0);

	if(dt_gflags & DT_DISABLE_CURSOR) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	
	if(glewInit() != GLEW_OK) {
		fprintf(stderr, "ERROR: Failed to initialize glew!\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		exit(-1);
		return;
	}

	if(SDL_Init(SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "ERROR: Failed to initialize SDL for audio! %s\n"
				"DT_NO_AUDIO is now set!\n", SDL_GetError());
		dt_gflags |= DT_NO_AUDIO;
		// Dont call exit() here... maybe user still wants to watch the demo?
		// TODO: make sure user sees this error message and can decide if wanted to continue.
	}

	// Compile default vertex shader.

	default_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	if(default_vertex_shader > 0) {
		const char* vertex_source = 
			"#version 330\n"
			"in vec2 pos;"
			"void main() {"
				"gl_Position=vec4(pos.x,pos.y,0.0,1.0);"
			"}";

		glShaderSource(default_vertex_shader, 1, &vertex_source, NULL);
		glCompileShader(default_vertex_shader);
		
		if(dt_check_status(default_vertex_shader, DT_SHADER_STATUS)) {
			dt_gflags |= DT_INITIALIZED;
		}
		else {
			fprintf(stderr, "ERROR: Failed to compile default vertex shader!\n");
			dt_cleanup();
			exit(-1);
		}
	}
	else {
		fprintf(stderr, "ERROR: Failed to create default vertex shader!\n");
	}
}

void dt_play(char* music_path, void(*frame_update_callback)(double)) {
	
	if(frame_update_callback == NULL) {
		fprintf(stderr, "WARNING: frame_update_callback is equal to NULL!\n");
	}

	SDL_AudioSpec audio_spec;
	SDL_AudioDeviceID audio_dev = 0;
	u8* audio_init_pos = NULL;
	u32 audio_init_length = 0;

	if(!(dt_gflags & DT_NO_AUDIO)) {
		if(SDL_LoadWAV(music_path, &audio_spec, &audio_init_pos, &audio_init_length)) {
			audio_pos = audio_init_pos;
			audio_length = audio_init_length;
			audio_spec.callback = sdl_audio_callback;

			audio_dev = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);
			
			if(audio_dev > 0) {
				SDL_PauseAudioDevice(audio_dev, 0);
			}
			else {
				fprintf(stderr, "ERROR: Failed to open audio device! %s\n", SDL_GetError());
				dt_gflags |= DT_NO_AUDIO;
			}
		}
		else {
			fprintf(stderr, "ERROR: %s\n", SDL_GetError());
		}
	}

	double start = glfwGetTime();
	double elapsed = start;
	double dt = start;
	double pause_time = 0.0;
	const double min_dt = 60.0/1000.0;


	while(!glfwWindowShouldClose(window) && audio_length > 0) {
		if(dt < min_dt) {
			// Previous frame finished early.
			SDL_Delay((min_dt-dt)*1000.0);
		}

		double time = glfwGetTime();
	
		if(!(dt_gflags & DT_PAUSED)) {
			if(pause_time > 0.0) {
				glfwSetTime(pause_time);
				time = pause_time;
				pause_time = 0.0;
			}
			elapsed = time - start;
		}
		else {
			if(pause_time <= 0.0) {
				pause_time = glfwGetTime();
			}
		}
		
		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT);

		if(frame_update_callback != NULL) {
			frame_update_callback(elapsed);
		}

		if(dt_current_shader > 0) {
			glBegin(GL_QUADS);
			glVertex2f(-1.0, -1.0);
			glVertex2f(-1.0,  1.0);
			glVertex2f( 1.0,  1.0);
			glVertex2f( 1.0, -1.0);
			glVertex2f(-1.0, -1.0);
			glEnd();
		}

		glfwSwapBuffers(window);
		dt = glfwGetTime() - time;
	}

	glUseProgram(0);
	if(!(dt_gflags & DT_NO_AUDIO)) {
		if(audio_dev > 0) {
			SDL_CloseAudioDevice(audio_dev);
		}
		if(audio_init_pos != NULL) {
			SDL_FreeWAV(audio_init_pos);
		}
	}

	dt_cleanup();
}

void dt_stop() {
	glfwSetWindowShouldClose(window, 1);
}

void dt_cleanup() {
	glfwDestroyWindow(window);
	glfwTerminate();
	SDL_Quit();

	puts("\033[32m Bye! <3\033[0m");
}

u8 dt_get_flag(u32 flag) {
	return dt_gflags & flag;
}

void dt_set_flag(u32 flag, u8 b) {
	b ? (dt_gflags |= flag) : (dt_gflags &= ~flag);
}

int dt_get(u32 i) {
	int res = -1;
	if(i < DT_NUM_COMMON_DATA) {
		res = dt_common_data[i];
	}
	return res;
}

void dt_set(u32 i, int value) {
	if(i < DT_NUM_COMMON_DATA) {
		dt_common_data[i] = value;
	}
}


void dt_shader_uniform_1f(char* name, double x) {
	glUniform1f(glGetUniformLocation(dt_current_shader, name), x);
}

void dt_shader_uniform_2f(char* name, double x, double y) {
	glUniform2f(glGetUniformLocation(dt_current_shader, name), x, y);
}

void dt_shader_uniform_3f(char* name, double x, double y, double z) {
	glUniform3f(glGetUniformLocation(dt_current_shader, name), x, y, z);
}

void dt_shader_uniform_4f(char* name, double x, double y, double z, double w) {
	glUniform4f(glGetUniformLocation(dt_current_shader, name), x, y, z, w);
}


void dt_shader_uniform_1i(char* name, int x) {
	glUniform1i(glGetUniformLocation(dt_current_shader, name), x);
}

void dt_shader_uniform_2i(char* name, int x, int y) {
	glUniform2i(glGetUniformLocation(dt_current_shader, name), x, y);
}

void dt_shader_uniform_3i(char* name, int x, int y, int z) {
	glUniform3i(glGetUniformLocation(dt_current_shader, name), x, y, z);
}

void dt_shader_uniform_4i(char* name, int x, int y, int z, int w) {
	glUniform4i(glGetUniformLocation(dt_current_shader, name), x, y, z, w);
}



