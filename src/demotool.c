#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

#include "demotool.h"


static GLFWwindow* window = NULL;
static int window_width = 0;
static int window_height = 0;
static u8* audio_pos = 0;
static u32 audio_length = 0;


void sdl_audio_callback(void* userdata, u8* stream, int bytes) {
	if(audio_length == 0) { return; }
	bytes = ((u32)bytes > audio_length ? audio_length : (u32)bytes);
	SDL_memcpy(stream, audio_pos, bytes);
	audio_pos += bytes;
	audio_length -= bytes;
}


void dt_start(char* title) {
	
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
	glViewport(0, 0, window_width, window_height);

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
}

void dt_play(double* timeline, u64 num_points, char* vs_path, char* fs_path, char* music_path) {

	if(timeline == NULL || num_points == 0) {
		fprintf(stderr, "Timeline cant be empty!\n");
		return;
	}

	// Setup shader program.

	const u32 vs = dt_create_shader(vs_path, GL_VERTEX_SHADER);
	const u32 fs = dt_create_shader(fs_path, GL_FRAGMENT_SHADER);
	const u32 program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	
	glDeleteShader(vs);
	glDeleteShader(fs);

	if(status_ok(program, STATUS_FOR_PROGRAM)) {

		// Setup audio.

		SDL_AudioSpec audio_spec;
		SDL_AudioDeviceID audio_dev;
		u8* audio_start = NULL;
		u32 audio_start_length = 0;

		if(SDL_LoadWAV(music_path, &audio_spec, &audio_start, &audio_start_length) == NULL) {
			fprintf(stderr, "Failed to open \"%s\"! %s\n", music_path, SDL_GetError());
		}
		else {
			audio_pos = audio_start;
			audio_length = audio_start_length;
			audio_spec.callback = sdl_audio_callback;
			audio_dev = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);
			if(audio_dev > 0) {
				SDL_PauseAudioDevice(audio_dev, 0);
			}
			else {
				fprintf(stderr, "Failed to open audio device! %s\n", SDL_GetError());
			}
		}


		// Find uniform locations.

		const u32 ulocation_timeline_point = glGetUniformLocation(program, "gTimelinePoint");
		const u32 ulocation_time = glGetUniformLocation(program, "gTime");
		const u32 ulocation_res = glGetUniformLocation(program, "gRes");
	

		// Start rendering stuff...

		u64 timeline_point = 0;
		double start = glfwGetTime();
		double timeline_point_start = start;

		while(!glfwWindowShouldClose(window) && audio_length > 0) {

			const double elapsed = glfwGetTime() - start;
			if(glfwGetKey(window, GLFW_KEY_ENTER)) {
				start = glfwGetTime();
				timeline_point = 0;
				audio_pos = audio_start;
				audio_length = audio_start_length;
			}
			else if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
				glfwSetWindowShouldClose(window, 1);
			}
			
			glClearColor(0.0, 0.0, 0.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT);
			glUseProgram(program);

			glUniform1i(ulocation_timeline_point,  timeline_point);
			glUniform1f(ulocation_time,            elapsed);
			glUniform2f(ulocation_res,             window_width, window_height);

			glBegin(GL_QUADS);
			glVertex2f(-1.0, -1.0);
			glVertex2f(-1.0,  1.0);
			glVertex2f( 1.0,  1.0);
			glVertex2f( 1.0, -1.0);
			glEnd();
			
			if(elapsed - timeline_point_start > timeline[timeline_point]) {
				timeline_point_start = glfwGetTime();
				if(timeline_point+1 < num_points) {
					timeline_point++;
				}
			}

			glfwSwapBuffers(window);
			glfwPollEvents();
		}

		if(audio_dev > 0) {
			SDL_CloseAudioDevice(audio_dev);
		}
	
		if(audio_start != NULL) {
			SDL_FreeWAV(audio_start);
		}
	}
	else {
		fprintf(stderr, "Failed to link program!\n");
	}

	glDeleteProgram(program);
	dt_quit();
}

void dt_quit() {
	glfwDestroyWindow(window);
	glfwTerminate();
	SDL_Quit();

	puts("\033[32m Bye! <3\033[0m");
}

u8 status_ok(u32 id, u32 type) {
	u8 res = 1;
	char* msg = NULL;
	int msg_length = 0;
	int p = 0;

	if(type == STATUS_FOR_SHADER) {
		glGetShaderiv(id, GL_COMPILE_STATUS, &p);
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &msg_length);
	}
	else if(type == STATUS_FOR_PROGRAM) {
		glGetProgramiv(id, GL_LINK_STATUS, &p);
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &msg_length);
	}

	if(msg_length > 0) {
		res = 0;
		msg = malloc(msg_length);
		if(msg != NULL) {
			if(type == STATUS_FOR_SHADER) {
				glGetShaderInfoLog(id, msg_length, NULL, msg);
			}
			else if(type == STATUS_FOR_PROGRAM) {
				glGetProgramInfoLog(id, msg_length, NULL, msg);
			}
			fprintf(stderr, 
					"\033[91m"
					"---- SHADER ERROR ----------------------\n"
					"%s\033[0m\n", msg);
		}
		else {
			fprintf(stderr, "Failed to allocate %i bytes of memory!\n", msg_length);
		}
	}

	return res;
}

u32 dt_create_shader(char* filename, u32 type) {
	u32 shader = glCreateShader(type);
	if(shader > 0) {
		int fd = open(filename, O_RDONLY);
		if(fd >= 0) {
			struct stat sb;
			if(fstat(fd, &sb) >= 0) {
				char* buf = malloc(sb.st_size+1);
				if(buf != NULL) {
					if(read(fd, buf, sb.st_size) >= 0) {
						buf[sb.st_size] = '\0';
						glShaderSource(shader, 1, (const char**)&buf, NULL);
						glCompileShader(shader);
						if(!status_ok(shader, STATUS_FOR_SHADER)) {
							fprintf(stderr, "Failed to compile shader from \"%s\" file!\n", filename);
						}
					}
					else {
						fprintf(stderr, "Failed to read file \"%s\"! errno: %i\n", filename, errno);
						perror("read");
					}

					free(buf);
				}
				else {
					fprintf(stderr, "Failed to allocate %li bytes of memory! errno: %i\n", sb.st_size, errno);
				}
			}
			else {
				fprintf(stderr, "Failed to get file status \"%s\"! fd: %i errno: %i\n", filename, fd, errno);
				perror("fstat");
			}

			close(fd);
		}
		else {
			fprintf(stderr, "Failed to open file \"%s\"! errno: %i\n", filename, errno);
			perror("open");
		}
	}
	else  {
		fprintf(stderr, "Failed to create shader!\n");
	}
	return shader;
}

