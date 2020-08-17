#ifndef __GL_SETUP_H_
#define __GL_SETUP_H_

#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/glu.h>
#else
#include <Windows.h>
#include "include/GLFW/glfw3.h"
#include "include/GL/glut.h"
#include "include/GL/glu.h"
#include "include/GL/gl.h"
#include <string.h>
#include <cstdlib>
#include <iostream>
using namespace std;

#pragma comment(lib, "lib/glfw3.lib")
#pragma comment(lib, "lib/glut32.lib")
#pragma comment(lib, "lib/glu32.lib")
#pragma comment(lib, "lib/opengl32.lib")
#endif

float	screenScale;		// Portion of the screen when not using full screen
int	screenW, screenH;	// screenScale poriton of the screen
int	windowW, windowH;	// Framebuffer size
float	aspect;			// Aspect ratio = windowW/windowH
float	dpiScaling;		// DPI scaling for HIDPI

int	vsync;			// Vertical sync on/off

bool 	perspectiveView;	// Perspective or orthographic viewing
float	fovy;			// Field of view in the y direction
float	nearDist;		// Near plane
float	farDist;		// Far plane

void errorCallback(int error, const char* description)
{
	cerr << "####" << description << endl;
}

void	setupProjectionMatrix() {
	if (perspectiveView)	gluPerspective(fovy, aspect, nearDist, farDist);
	else	glOrtho(-1.0*aspect, 1.0*aspect, -1.0, 1.0, -nearDist, farDist);
}

void reshape(GLFWwindow* window, int w, int h)
{
	aspect = (float)w / h;

	// Set the viewport
	windowW = w;
	windowH = h;
	glViewport(0, 0, w, h);

	// Initialize the projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Set up a projection matrix
	setupProjectionMatrix();

	// The Screen size is required for mouse interaction
	glfwGetWindowSize(window, &screenW, &screenH);
	cerr << "reshape(" << w << ", " << h << ")";
	cerr << " with screen " << screenW << " x " << screenH << endl;
}

GLFWwindow* initializeOpenGL(int argc, char* argv[], GLfloat bgColor[4]) {
	perspectiveView = true;

	screenScale = 0.5;
	screenW = 0, screenH = 0;
	windowW, windowH;

	dpiScaling = 0;
	vsync = 1;

	nearDist = 1.0;
	farDist = 20.0;
	fovy = 46.4;

	glfwSetErrorCallback(errorCallback);

	// Init GLFW
	if (!glfwInit())	exit(EXIT_FAILURE);

	// Enable OpenGL 2.1 in OS X
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

	// Create the window
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	int monitorW, monitorH;
	glfwGetMonitorPhysicalSize(monitor, &monitorW, &monitorH);
	cerr << "Status: Monitor " << monitorW << "mm x " << monitorH << "mm" << endl;

	const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
	if (screenW == 0)	screenW = videoMode->width * screenScale;
	if (screenH == 0)	screenH = videoMode->height * screenScale;

	GLFWwindow* window = glfwCreateWindow(screenW, screenH, argv[0], NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		cerr << "Failed in glfwCreateWindow()" << endl;
		return NULL;
	}

	// Context
	glfwMakeContextCurrent(window);

	// Clear the background ASAP
	glClearColor(bgColor[0], bgColor[1], bgColor[2], bgColor[3]);
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
	glfwSwapBuffers(window);

	// Check the size of the window
	glfwGetWindowSize(window, &screenW, &screenH);
	cerr << "Status: Screen " << screenW << " x " << screenH << endl;

	glfwGetFramebufferSize(window, &windowW, &windowH);
	cerr << "Status: Framebuffer " << windowW << " x " << windowH << endl;

	// DPI Scaling
	if (dpiScaling == 0)	dpiScaling = float(windowW) / screenW;

	// Callbacks
	glfwSetFramebufferSizeCallback(window, reshape);

	// Get the OpenGL version and renderer
	cout << "Status: Renderer " << glGetString(GL_RENDERER) << endl;
	cout << "Status: Ventor " << glGetString(GL_VENDOR) << endl;
	cout << "Status: OpenGL " << glGetString(GL_VERSION) << endl;

	// Vertical sync ...
	glfwSwapInterval(vsync);	// 0 for immediate mode (Tearing possible)

	return window;
}

void drawAxes(float l, float w) {
	glLineWidth(w);

	glBegin(GL_LINES);
	glColor3f(20, 0, 0); glVertex3f(0, 0, 0); glVertex3f(20, 0, 0); // x-axis
	glColor3f(0, 20, 0); glVertex3f(0, 0, 0); glVertex3f(0, 20, 0); // y-axis
	glColor3f(0, 0, 20); glVertex3f(0, 0, 0); glVertex3f(0, 0, 20); // z-axis
	glEnd();
}


#endif	// __GL_SETUP_H_