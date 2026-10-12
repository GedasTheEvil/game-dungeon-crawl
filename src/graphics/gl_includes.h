#ifndef GL_INCLUDES_H
#define GL_INCLUDES_H
#ifdef WIN32
#include <GL/freeglut.h>
#else
#include <GL/glut.h>
#include <GL/freeglut_ext.h> // glutLeaveMainLoop, glutSetOption (GLUT here is freeglut)
#endif
#endif
