#pragma once
//CSCI 5611 OpenGL Animation Tutorial 
//A 1D bouncing ball

//Running on Mac OSX
//  Download the SDL2 Framework from here: https://www.libsdl.org/download-2.0.php
//  Open the .dmg and move the file SDL2.Framework into the directory /Library/Frameworks/
//  Make sure you place this cpp file in the same directory with the "glad" folder and the "glm" folder
//  g++ Bounce.cpp glad/glad.c -framework OpenGL -framework SDL2; ./a.out

//Running on Windows
//  Download the SDL2 *Development Libararies* from here: https://www.libsdl.org/download-2.0.php
//  Place SDL2.dll, the 3 .lib files, and the include directory in locations known to MSVC
//  Add both Bounce.cpp and glad/glad.c to the project file
//  Compile and run

//Running on Ubuntu
//  sudo apt-get install libsdl2-2.0-0 libsdl2-dev
//  Make sure you place this cpp file in the same directory with the "glad" folder and the "glm" folder
//  g++ Bounce.cpp glad/glad.c -lGL -lSDL; ./a.out

#include "glad/glad.h"  //Include order can matter here
#ifndef _WIN32
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
#endif
#include <cstdio>

#define PARTICLE_NUM 200
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include <omp.h>
#include <time.h>
#include <vector>
#include <fstream>
using namespace std;

//data structures
typedef struct Triangle {
	int index[3][2]; //3 nodes, 2D indices
	glm::vec3 force_aero;
	glm::vec3 velocity;
	glm::vec3 normal_star;
	bool burned = false;
};

//functions
void Win2PPM(int width, int height);
static char* readShaderSource(const char* shaderFile);
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);
bool IsUnderCone(glm::vec3 point, float radius, float height);
bool IsInHemisphere(glm::vec3 point, glm::vec3 center, float radius);
float randf();
void computePhysics(int i, float dt);

