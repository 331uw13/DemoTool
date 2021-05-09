cmake_minimum_required(VERSION 3.13.4)
project(demo-engine C)

find_package(OpenGL REQUIRED)

set(WARNINGS "-Wall -Wextra -Wunused-function -Wformat -Wformat-overflow -Wpointer-arith -lglfw -lGLEW -lGL -lSDL2 -O3")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3")
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNINGS}")
file(GLOB_RECURSE SOURCE_FILES "./src/*.c")
file(GLOB_RECURSE SOURCE_HEADER_FILES "./src/*.h")

add_executable(demo ${SOURCE_FILES})

include(GNUInstallDirs)

