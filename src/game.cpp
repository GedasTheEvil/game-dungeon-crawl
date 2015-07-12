#ifndef WIN32
        #include <GL/glut.h>
#endif

#ifdef WIN32
     #include <GL/freeglut.h>
#endif

#include <GL/glu.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include "input.h"
#include "textures.h"
#include "sound.h"

int resX = 800;
int resY = 500;
int window = 1;
int fs = 0;


void InitGL(GLsizei Width, GLsizei Height)	// We call this right after our OpenGL window is created.
{
   
     glEnable(GL_TEXTURE_2D);                    // Enable texture mapping.

     glBlendFunc(GL_DST_COLOR, GL_ZERO);          // Set the blending function for translucency (note off at init time)
     glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
     glClearDepth(1.0);				// Enables Clearing Of The Depth Buffer
     glDepthFunc(GL_LEQUAL);                       // type of depth test to do.
     glEnable(GL_DEPTH_TEST);                    // enables depth testing.
     glShadeModel(GL_SMOOTH);			// Enables Smooth Color Shading

     glDisable(GL_LIGHTING);

}

void ReSizeGLScene(GLsizei Width, GLsizei Height)
{
    if (Height==0)				// Prevent A Divide By Zero If The Window Is Too Small
	Height=1;

    glViewport(0, 0, Width, Height);		// Reset The Current Viewport And Perspective Transformation

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(45.0f,(GLfloat)Width/(GLfloat)Height,0.1f,10000.0f);
    glMatrixMode(GL_MODELVIEW);
	printf("Resized to : %d x %d\n",Width,Height);
	resX = Width; resY =Height;
}

//sita eilute reikalinga, kad kompiliuojant per win nemestu erroro su "WinMain@16" undefined
#undef main

int main (int argc, char *argv[])
{
	glutInit(&argc, argv);  
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ALPHA);
	
	glutInitWindowSize(resX, resY);  
	
	glutInitWindowPosition(0, 0);  

	window = glutCreateWindow("Dungeon Crawl");  

	glutDisplayFunc(&Draw);  

	if(fs)glutFullScreen();

	glutIdleFunc(&Idle); 

	glutReshapeFunc(&ReSizeGLScene);

	glutKeyboardFunc(&keyPressed);

	glutSpecialFunc(&specialKeyPressed);

	glutMouseFunc(processMouse);
	glutMotionFunc(processMouseActiveMotion);
	glutPassiveMotionFunc(processMousePassiveMotion);
	glutEntryFunc(processMouseEntry);
	
	InitGL(resX, resY);

	glutMainLoop();  
	
	return 1;
} 
