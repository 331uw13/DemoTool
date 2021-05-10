#include <errno.h>
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
static int window_width = 0;
static int window_height = 0;
static u8* audio_pos = 0;
static u32 audio_length = 0;
static u32 default_vertex_shader = 0;
static char** shader_include_src = NULL;
static u64 shader_include_src_length = 0;

static int  dt_check_status(u32 id, u32 flag);
static void dt_delete_effects(struct shader_effect* effects, u32 count);


u32 dt_create_program(char* fs_src) {
	
	u32 fs = 0;
	u32 program = 0;
	
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

		if(shader_include_src == NULL || shader_include_src_length == 0) {
			glShaderSource(fs, 1, (const char**)&fs_src, NULL);
		}
		else {
			const u64 origin_src_length = strlen(fs_src);
			char* full_src = malloc(shader_include_src_length + origin_src_length + 1);
			
			if(full_src != NULL) {
				memmove(full_src, *shader_include_src, shader_include_src_length);
				memmove(full_src + shader_include_src_length, fs_src, origin_src_length);

				full_src[shader_include_src_length + origin_src_length] = '\0';
				glShaderSource(fs, 1, (const char**)&full_src, NULL);
			}
			else {
				fprintf(stderr, "Failed to allocate %li bytes of memory! errno: %i\n", 
						shader_include_src_length + origin_src_length, errno);
				goto finish;
			}
		}

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
			goto finish;
		}
	}

finish:

	if(fs > 0) {
		// Already linked to program or failed to create program.
		glDeleteShader(fs);
	}

	return program;
}

void dt_shader_include(char** src, u64 length) {
	shader_include_src = src;
	shader_include_src_length = (src == NULL) ? 0 : length;
}

void sdl_audio_callback(void* userdata, u8* stream, int bytes) {
	if(audio_length == 0) { return; }
	bytes = ((u32)bytes > audio_length ? audio_length : (u32)bytes);
	SDL_memcpy(stream, audio_pos, bytes);
	audio_pos += bytes;
	audio_length -= bytes;
}

void glfw_key_callback(GLFWwindow* win, int key, int scancode, int action, int mods) {
	if(key == GLFW_KEY_ESCAPE || key == GLFW_KEY_END && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(win, 1);
	}

}

void dt_initialize(char* title) {

	if(!glfwInit()) {
		fprintf(stderr, "Failed to initialize glfw!\n");
		return;
	}

	glfwWindowHint(GLFW_MAXIMIZED, 1);
	glfwWindowHint(GLFW_FOCUS_ON_SHOW, 1);
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

	glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), 
			&monitor_x, &monitor_y, &monitor_w, &monitor_h);

	window = glfwCreateWindow(monitor_w, monitor_h, title, NULL, NULL);
	if(window == NULL) {
		fprintf(stderr, "Failed to create window\n");
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	glfwGetWindowSize(window, &window_width, &window_height);
	glfwSetKeyCallback(window, glfw_key_callback);

	glViewport(0.0, 0.0, window_width, window_height);
	glClearColor(0.0, 0.0, 0.0, 1.0);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowPos(window, monitor_x, monitor_y);
	
	if(glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize glew!\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		return;
	}

	if(SDL_Init(SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Failed to initialize SDL for audio! %s\n", SDL_GetError());
	}

	// Compile default vertex shader.

	default_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	if(default_vertex_shader > 0) {
		const char* vertex_source = 
			"#version 330\n"
			"in vec2 pos;"
			"void main() {"
				"gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);"
			"}";

		glShaderSource(default_vertex_shader, 1, &vertex_source, NULL);
		glCompileShader(default_vertex_shader);
		
		if(!dt_check_status(default_vertex_shader, DT_SHADER_STATUS)) {
			fprintf(stderr, "Failed to compile default vertex shader!\n");
		}
	}
	else {
		fprintf(stderr, "Failed to create default vertex shader!\n");
	}
}

void dt_play(struct shader_effect* effects, u32 effects_count, char* music_path) {
	
	if(effects == NULL || effects_count == 0) {
		fprintf(stderr, "There are no effects to play...\n");
		dt_delete_effects(effects, effects_count);
		return;
	}
	
	glDeleteShader(default_vertex_shader); // Not needed anymore.


	// Setup audio.
	
	SDL_AudioSpec audio_spec;
	SDL_AudioDeviceID audio_dev;
	u8* audio_init_pos = NULL;
	u32 audio_init_length = 0;

	if(SDL_LoadWAV(music_path, &audio_spec, &audio_init_pos, &audio_init_length)) {
		audio_pos = audio_init_pos;
		audio_length = audio_init_length;
		audio_spec.callback = sdl_audio_callback;
		audio_dev = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);
		if(audio_dev > 0) {
			SDL_PauseAudioDevice(audio_dev, 0);
		}
		else {
			fprintf(stderr, "Failed to open audio device! %s\n", SDL_GetError());
		}
	}
	else {
		fprintf(stderr, "Failed to load music file \"%s\"! %s\n", music_path, SDL_GetError());
	}

	struct shader_effect* eff = &effects[0];
	u32 effect_index = 0;
	glUseProgram(eff->program);

	double start = glfwGetTime();
	double elapsed = start;
	double eff_start = start;
	double eff_elapsed = start;
	double dt = start;
	const double min_dt = 60.0 / 1000.0;

	while(!glfwWindowShouldClose(window) && audio_length > 0) {
		if(dt < min_dt) {
			// Previous frame finished early.
			SDL_Delay((min_dt - dt) * 1000.0);
		}

		const double time = glfwGetTime();
		elapsed = time - start;
		eff_elapsed = time - eff_start;
		//printf("Elapsed: %f | Effect elapsed: %f\n", elapsed, eff_elapsed);
		
		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT);

		if(eff_elapsed >= eff->duration) {
			if(effect_index < effects_count) {
				eff_start = time;
				effect_index++;
				eff = &effects[effect_index];
				
				glUseProgram(eff->program);
			}
		}

		glUniform2f(glGetUniformLocation(eff->program, "screen"), window_width, window_height);
		glUniform1f(glGetUniformLocation(eff->program, "time"), elapsed);
		glUniform1f(glGetUniformLocation(eff->program, "eff_time"), eff_elapsed);
	
		glBegin(GL_QUADS);
		glVertex2f(-1.0, -1.0);
		glVertex2f(-1.0,  1.0);
		glVertex2f( 1.0,  1.0);
		glVertex2f( 1.0, -1.0);
		glVertex2f(-1.0, -1.0);
		glEnd();

		glfwSwapBuffers(window);
		dt = glfwGetTime() - time;
	}

	glUseProgram(0);


	if(audio_dev > 0) {
		SDL_CloseAudioDevice(audio_dev);
	}
	
	if(audio_init_pos != NULL) {
		SDL_FreeWAV(audio_init_pos);
	}

	dt_delete_effects(effects, effects_count);
}

void dt_quit() {
	glfwDestroyWindow(window);
	glfwTerminate();
	SDL_Quit();

	puts("\033[32m Bye! <3\033[0m");
}

void dt_delete_effects(struct shader_effect* effects, u32 count) {
	for(u32 i = 0; i < count; i++) {
		glDeleteProgram(effects[i].program);
	}
}

int dt_check_status(u32 id, u32 flag) {
	int res = 0;
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
			fprintf(stderr, "Failed to allocate %i bytes of memory!\n", msg_length);
		}
	}

	return res;
}

