#include "glSetup.h"
#include <Windows.h>
#include "include/GLFW/glfw3.h"
#include "include/GL/gl.h"
#include "include/GL/glut.h"

#include "include/glm/glm.hpp"		// OpenGL Mathematics
#include "include/glm/gtc/type_ptr.hpp"	// glm::value_ptr()
#include "include/glm/gtc/matrix_transform.hpp"
#include "include/glm/gtx/string_cast.hpp"
using namespace glm;

#pragma comment(lib, "lib/glfw3.lib")
#pragma comment(lib, "lib/opengl32.lib")
#pragma comment(lib, "lib/glut32.lib")

#include <iostream>
using namespace std;

// Camera configuration
vec3	eye(3, 5.5, 6);
vec3	center(0, 0, 0);
vec3	up(0, 1, 0);

// Light configuration
vec4	lightInitialP(3.0, 3.0, 0.0, 1);	// Light position

											// Global coordinate frame
float	AXIS_LENGTH = 3;
float	AXIS_LINE_WIDTH = 1;

// Colors
GLfloat bgColor[4] = { 1, 1, 1, 1 };

// Control Variable
bool pause = true;
bool vertexN = false; // toggle with the key 'n'
bool shineC = false; // toggle with the key 't'

float thetaLight[3];
bool lightOn[3];		// Point = 0, distant = 1, spot = 2 lights

bool exponent = false;
bool exponentInitial = 0.0;			// [0, 128]
bool exponentValue = exponentInitial;
bool exponentNorm = exponentValue / 128.0;	// [0, 1]

bool cutoff = false;
float cutoffMax = 60;				// [0, 90] degree
float cutoffInitial = 30.0;			// [0, cutoffMax] degree
float cutoffValue = cutoffInitial;
float cutoffNorm = cutoffValue / cutoffMax;	// [0, 1]

vec3	p[36][18];		// Points
vec3	qcenter[36][18];	// Quads' center points
vec3	normal[36][18];		// Quads' normal vectors
GLfloat mat_shininess;

float timeStep = 1.0 / 120; // 120fps. 60fps using vsync = 1
float period = 4.0;
int frame = 0; // Current frame

void reinitialize()
{
	frame = 0;
	mat_shininess = 25;

	lightOn[0] = false;	// Turn on only the point light
	lightOn[1] = false;
	lightOn[2] = false;

	for (int i = 0; i < 3; i++)
		thetaLight[i] = 0;

	exponentValue = exponentInitial;
	exponentNorm = exponentValue / 128.0;

	cutoffValue = cutoffInitial;
	cutoffNorm = cutoffValue / cutoffMax;
}

void animate()
{
	frame += 1;

	// Rotation angle of the light
	for (int i = 0; i < 3; i++)
		if (lightOn[i])	thetaLight[i] += 4 / period;	// degree

	if (lightOn[2] && exponent)
	{
		exponentNorm += radians(4.0 / period) / pi<float>();
		exponentValue = 128.0 * (acos(cos(exponentNorm * pi<float>())) / pi<float>());
	}
	if (lightOn[2] && cutoff)
	{
		cutoffNorm += radians(4.0 / period) / pi<float>();
		cutoffValue = cutoffMax * (acos(cos(cutoffNorm * pi<float>())) / pi<float>());
	}
}

// Light
void setupLight(const vec4& p, int i)
{
	GLfloat ambient[4] = { 0.1, 0.1, 0.1, 1 };
	GLfloat diffuse[4] = { 1.0, 1.0, 1.0, 1 };
	GLfloat specular[4] = { 1.0, 1.0, 1.0, 1 };

	glLightfv(GL_LIGHT0 + i, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0 + i, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0 + i, GL_POSITION, value_ptr(p));

	// Attenuation for the point light
	if (i == 0)
	{
		glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, 1.0);
		glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, 0.1);
		glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 0.05);
	}
	else { // Default value
		glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, 1.0);
		glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, 0.0);
		glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 0.0);
	}

	if (i == 2) // Spot light
	{
		vec3	spotDirection = -vec3(p);
		glLightfv(GL_LIGHT0 + i, GL_SPOT_DIRECTION, value_ptr(spotDirection));
		glLightf(GL_LIGHT0 + i, GL_SPOT_CUTOFF, cutoffValue);
		glLightf(GL_LIGHT0 + i, GL_SPOT_EXPONENT, exponentValue);
	}
	else { // Point and distant light.
		   // 180 to turn off cutoff when it was used as a spot light.
		glLightf(GL_LIGHT0 + i, GL_SPOT_CUTOFF, 180); // uniform light distribution
	}
}

// Material
void setupMaterial()
{
	// Make it possible to change a subset of material parameters
	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	// Material
	GLfloat mat_ambient[4] = { 0.1, 0.1, 0.1, 1 };
	GLfloat mat_specular[4] = { 0.5, 0.5, 0.5, 1 };

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
}

// Sphere, cylinder
GLUquadricObj* sphere = NULL;
GLUquadricObj* cylinder = NULL;
GLUquadricObj* cone = NULL;

void init()
{
	// Animation system
	reinitialize();

	// Prepare points p[36][18]
	float angle1 = 0.0f, angle2 = 0.0f;
	vec3 start(0.5f, 0.0f, 0.0f);
	vec3 axisy(0.0f, 1.0f, 0.0f);
	vec3 axisz(0.0f, 0.0f, 1.0f);
	mat4 TR = translate(mat4(1.0), vec3(1.0f, 1.0f, 0.0f));

	// Set p[j][i]
	for (int i = 0; i < 18; i++)
	{
		mat4 R1 = rotate(mat4(1.0), angle1, axisz);
		for (int j = 0; j < 36; j++)
		{
			mat4 R2 = rotate(mat4(1.0), angle2, axisy);
			vec4 V = R2 * TR * R1 * vec4(start, 1);
			p[j][i] = vec3(V.x, V.y, V.z);
			angle2 += (pi<float>() / 18.0f);
		}
		angle1 -= (pi<float>() / 9.0f);
	}

	// Set qcenter[j][i] and normal[j][i]
	for (int i = 0; i < 18; i++)
		for (int j = 0; j < 36; j++)
		{
			qcenter[j][i] = (p[j][i] + p[j][(i + 1) % 18] + p[(j + 1) % 36][(i + 1) % 18] + p[(j + 1) % 36][i]) / 4.0f;
			normal[j][i] = normalize(cross(p[j][i] - p[(j + 1) % 36][(i + 1) % 18], p[j][(i + 1) % 18] - p[(j + 1) % 36][i]));
		}

	// Prepare quadric shapes
	sphere = gluNewQuadric();
	gluQuadricDrawStyle(sphere, GLU_FILL);
	gluQuadricNormals(sphere, GLU_SMOOTH);
	gluQuadricOrientation(sphere, GLU_OUTSIDE);
	gluQuadricTexture(sphere, GL_FALSE);

	cylinder = gluNewQuadric();
	gluQuadricDrawStyle(cylinder, GLU_FILL);
	gluQuadricNormals(cylinder, GLU_SMOOTH);
	gluQuadricOrientation(cylinder, GLU_OUTSIDE);
	gluQuadricTexture(cylinder, GL_FALSE);

	cone = gluNewQuadric();
	gluQuadricDrawStyle(cone, GLU_FILL);
	gluQuadricNormals(cone, GLU_SMOOTH);
	gluQuadricOrientation(cone, GLU_OUTSIDE);
	gluQuadricTexture(cone, GL_FALSE);

	// Keyboard
	cout << "Draw a torus model" << endl;

	cout << "Draw the vertex normal vectors: toggle with the key 'n'" << endl;
	cout << "Point light: toggle with the key 'p'" << endl;
	cout << "Directional light: toggle with the key 'd'" << endl;
	cout << "Spot light: toggle with the key 's'" << endl;

	cout << "Time-varying shininess coefficient in specular reflection: toggle with the key 't'" << endl;
}

void quit()
{
	// Delete quadric shapes
	gluDeleteQuadric(sphere);
	gluDeleteQuadric(cone);
}

void drawTorus() // Draw a Torus model
{
	glEnable(GL_POLYGON_OFFSET_FILL);

	glPolygonOffset(1.f, 1.f);
	glColor3f(0.4, 0.4, 0.4);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBegin(GL_QUADS);
	for (int i = 0; i < 18; i++)
	{

		for (int j = 0; j < 36; j++)
		{
			glNormal3fv(value_ptr(p[j][i]));
			glVertex3fv(value_ptr(p[j][i]));
			glNormal3fv(value_ptr(p[j][(i + 1) % 18]));
			glVertex3fv(value_ptr(p[j][(i + 1) % 18]));
			glNormal3fv(value_ptr(p[(j + 1) % 36][(i + 1) % 18]));
			glVertex3fv(value_ptr(p[(j + 1) % 36][(i + 1) % 18]));
			glNormal3fv(value_ptr(p[(j + 1) % 36][i]));
			glVertex3fv(value_ptr(p[(j + 1) % 36][i]));
		}
	}
	glEnd();

	glColor3f(0, 0, 0);
	glPolygonOffset(0.0f, 0.0f);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	for (int i = 0; i < 18; i++)
	{
		glBegin(GL_QUADS);
		for (int j = 0; j < 36; j++)
		{
			glNormal3fv(value_ptr(p[j][i]));
			glVertex3fv(value_ptr(p[j][i]));
			glNormal3fv(value_ptr(p[j][(i + 1) % 18]));
			glVertex3fv(value_ptr(p[j][(i + 1) % 18]));
			glNormal3fv(value_ptr(p[(j + 1) % 36][(i + 1) % 18]));
			glVertex3fv(value_ptr(p[(j + 1) % 36][(i + 1) % 18]));
			glNormal3fv(value_ptr(p[(j + 1) % 36][i]));
			glVertex3fv(value_ptr(p[(j + 1) % 36][i]));
		}
		glEnd();
	}

	if (vertexN)
	{
		glColor3f(0, 0, 0);
		glPolygonOffset(1.f, 1.f);
		for (int i = 0; i < 18; i++)
			for (int j = 0; j < 36; j++)
			{
				glBegin(GL_LINES);
				glNormal3fv(value_ptr(qcenter[j][i]));
				glVertex3fv(value_ptr(qcenter[j][i]));
				glNormal3fv(value_ptr(qcenter[j][i] + 0.2f * normal[j][i]));
				glVertex3fv(value_ptr(qcenter[j][i] + 0.2f * normal[j][i]));
				glEnd();
			}
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// Draw a sphere using a GLU quadirc
void drawSphere(float radius, int slices, int stacks)
{
	gluSphere(sphere, radius, slices, stacks);
}

// Draw a cylinder using a GLU quadric
void drawCylinder(float radius, float height, int slices, int stacks)
{
	gluCylinder(cylinder, radius, radius, height, slices, stacks);
}

// Draw a cone using a GLU quadric
void drawCone(float radius, float height, int slices, int stacks)
{
	gluCylinder(cone, 0, radius, height, slices, stacks);
}

void computeRotation(const vec3& a, const vec3& b, float& theta, vec3& axis)
{
	axis = cross(a, b);
	float sinTheta = length(axis);
	float cosTheta = dot(a, b);
	theta = atan2(sinTheta, cosTheta) * 180 / pi<float>();
}

void drawArrow(const vec4& p, bool tailOnly)
{
	// Make it possible to change a subset of material parameters
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	// Common material
	GLfloat mat_specular[4] = { 1, 1, 1, 1 };
	GLfloat mat_shininess = 25;
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);

	// Transformation
	glPushMatrix();

	glTranslatef(p.x, p.y, p.z);

	if (!tailOnly)
	{
		float	theta;
		vec3	axis;
		computeRotation(vec3(0, 0, 1), vec3(0, 0, 0) - vec3(p), theta, axis);
		glRotatef(theta, axis.x, axis.y, axis.z);
	}

	// Tail sphere
	float	arrowTailRadius = 0.05;
	glColor3f(1, 0, 0); // ambient and diffuse
	drawSphere(arrowTailRadius, 16, 16);

	if (!tailOnly)
	{
		// Shaft cylinder
		float	arrowShaftRadius = 0.02;
		float	arrowShaftLength = 0.2;
		glColor3f(0, 1, 0);
		drawCylinder(arrowShaftRadius, arrowShaftLength, 16, 5);

		// Head one
		float	arrowheadHeight = 0.09;
		float	arrowheadRadius = 0.06;
		glTranslatef(0, 0, arrowShaftLength + arrowheadHeight);
		glRotatef(180, 1, 0, 0);
		glColor3f(0, 0, 1); // ambient and diffuse
		drawCone(arrowheadRadius, arrowheadHeight, 16, 5);
	}

	glPopMatrix();

	// For convential material setting
	glDisable(GL_COLOR_MATERIAL);
}

void drawSpotLight(const vec4& p, float cutoff)
{
	glPushMatrix();

	glTranslatef(p.x, p.y, p.z);

	float theta;
	vec3 axis;
	computeRotation(vec3(0, 0, 1), vec3(0, 0, 0) - vec3(p), theta, axis);
	glRotatef(theta, axis.x, axis.y, axis.z);

	// tan(cutoff)= r/h
	float h = 0.15;
	float r = h * tan(radians(cutoff));
	drawCone(r, h, 16, 5);

	// Color
	setupMaterial();

	// Apex
	float apexRadius = 0.06 *(0.5 + exponentValue / 128.0);
	drawSphere(apexRadius, 16, 16);

	glPopMatrix();
}

void render(GLFWwindow* window)
{
	// Background color
	glClearColor(bgColor[0], bgColor[1], bgColor[2], bgColor[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Modelview matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt(eye[0], eye[1], eye[2], center[0], center[1], center[2], up[0], up[1], up[2]);

	// Axes
	glDisable(GL_LIGHTING);
	drawAxes(AXIS_LENGTH, AXIS_LINE_WIDTH*dpiScaling);

	// Smooth shading
	glShadeModel(GL_SMOOTH);

	// Rotation of the light or 3x3 models
	vec3	axis(0, 1, 0);

	// Lighting
	//
	if (lightOn[0] || lightOn[1] || lightOn[2])	glEnable(GL_LIGHTING);

	// Set up the lights
	vec4 lightP[3];
	for (int i = 0; i < 3; i++)
	{
		// Just turn off the i-th light, if not lit
		if (!lightOn[i]) { glDisable(GL_LIGHT0 + i); continue; }

		// Turn on the i-th light
		glEnable(GL_LIGHT0 + i);

		// Dealing with the distant light
		lightP[i] = lightInitialP;
		if (i == 1) lightP[i].w = 0;

		// Lights rotate around the center of the world coordinate system
		mat4 R = rotate(mat4(1.0), radians(thetaLight[i]), axis);
		lightP[i] = R * lightP[i];

		// Set up the i-th light
		setupLight(lightP[i], i);
	}

	// Draw the geometries of the lights
	for (int i = 0; i < 3; i++)
	{
		if (!lightOn[i])	continue;

		if (i == 2)	drawSpotLight(lightP[i], cutoffValue);
		else drawArrow(lightP[i], i == 0);
	}

	// Material
	setupMaterial();

	// Draw
	drawTorus();
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		switch (key)
		{
			// Quit
		case GLFW_KEY_Q:
		case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GL_TRUE); break;

			// Play on/off
		case GLFW_KEY_SPACE: pause = !pause;	break;

			// vertex normal vectors
		case GLFW_KEY_N: vertexN = !vertexN;		break;

			// 3 lights
		case GLFW_KEY_P: lightOn[0] = !lightOn[0];	break;
		case GLFW_KEY_D: lightOn[1] = !lightOn[1];	break;
		case GLFW_KEY_S: lightOn[2] = !lightOn[2];	break;

			// shininess coefficient
		case GLFW_KEY_T: shineC = !shineC;				break;
		}
	}
}

int main(int argc, char* argv[])
{
	// Initialize the OpenGL system
	GLFWwindow* window = initializeOpenGL(argc, argv, bgColor);
	if (window == NULL)	return -1;

	// Callbacks
	glfwSetKeyCallback(window, keyboard);

	// Depth test
	glEnable(GL_DEPTH_TEST);

	// Normal vectors are normalized after transformation
	glEnable(GL_NORMALIZE);

	// Viewport and perspective setting
	reshape(window, windowW, windowH);

	// Initialization
	init();

	// Main loop
	float previous = glfwGetTime();
	float elapsed = 0;
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();		// Events

		float now = glfwGetTime();
		float delta = now - previous;
		previous = now;

		//Time passed after the previous frame
		elapsed += delta;

		// Deal with the current frame
		if (elapsed > timeStep)
		{
			// Animate 1 frame
			if (!pause) animate();

			elapsed = 0;
		}

		if (!shineC) {
			mat_shininess = 25;
		}
		else {
			int timecount = (int(glfwGetTime()) % 10);
			mat_shininess += (timecount*0.1);
			if (mat_shininess > 120)
				mat_shininess = 120;
		}

		render(window);			// Draw one frame
		glfwSwapBuffers(window);	// Swap buffers
	}

	// Finalization
	quit();

	// Terminate the glfw system
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}