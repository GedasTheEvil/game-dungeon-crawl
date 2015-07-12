#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include "vars.h"
#include <string.h>

#include "textures.h"
#include "keyboard.h"
#include "mouse.h"
#include "text.h"
#include "scene.h"


// int window = 0;

void InitGL(GLsizei Width, GLsizei Height)	// We call this right after our OpenGL window is created.
{

	glEnable(GL_TEXTURE_2D);                    // Enable texture mapping.
	LoadTGA(&texttex, "./Textures/Font.tga");
	LoadTGA(&nullt,   "./Textures/null.tga");
       LoadTGA(&Mat[Empty], "./Textures/null.tga");
	LoadTGA(&Mat[Wall], "./Textures/wall.tga");
	LoadTGA(&Mat[Door], "./Textures/gate.tga");
	LoadTGA(&Mat[Death], "./Textures/death.tga");
	LoadTGA(&Mat[Monster], "./Textures/monster.tga");
       LoadTGA(&Mat[Spike], "./Textures/spikes.tga");
       LoadTGA(&Mat[Ladder], "./Textures/ladder.tga");
       LoadTGA(&Mat[Area3D], "./Textures/3D.tga");
       LoadTGA(&Mat[Treasure], "./Textures/treasure.tga");
       LoadTGA(&Mat[Ankh], "./Textures/ankh.tga");
	LoadTGA(&Button[0], "./Textures/Btn_save.tga");
       LoadTGA(&Button[1], "./Textures/Btn_load.tga");
// 	memset(map[0].type, 0, 40*47);
       int i, j;
       for(i = 0; i < 40; i++)
	     for(j = 0; j< 47; j++)
	     {
		   map[j*40 + i].type = 0;
		   map[j*40 + i].Atr  = 0;
		   map[j*40 + i].AtrV = 0;   
	     }
	
	glBlendFunc(GL_DST_COLOR, GL_ZERO);          // Set the blending function for translucency (note off at init time)
	glClearColor(0.3f, 0.3f, 0.4f, 0.0f);	// This Will Clear The Background Color To sky blue
	glClearDepth(1.0);				// Enables Clearing Of The Depth Buffer
	glDepthFunc(GL_LEQUAL);                       // type of depth test to do.
	glEnable(GL_DEPTH_TEST);                    // enables depth testing.
	glShadeModel(GL_SMOOTH);			// Enables Smooth Color Shading
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();				// Reset The Projection Matrix
	
	gluPerspective(45.0f,(GLfloat)Width/(GLfloat)Height,0.1f,100.0f);	// Calculate The Aspect Ratio Of The Window
	
	glMatrixMode(GL_MODELVIEW);

	BuildFont();
}

void ReSizeGLScene(GLsizei Width, GLsizei Height){}
int main (int argc, char *argv[])
{


	/* Initialize GLUT state - glut will take any command line arguments that pertain to it or 
	X Windows - look at its documentation at http://reality.sgi.com/mjk/spec3/spec3.html */  
	glutInit(&argc, argv);  
	
	/* Select type of Display mode:   
	Double buffer 
	RGBA color
	Depth buffer 
	Alpha blending */  
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ALPHA);
	
	/* get a 640 x 480 window */
	glutInitWindowSize(640, 480);  
	
	/* the window starts at the upper left corner of the screen */
	glutInitWindowPosition(0, 0);  
	
	/* Open a window */  
	window = glutCreateWindow("Map editor :: by Gedas");  
	
	/* Register the function to do all our OpenGL drawing. */
	glutDisplayFunc(&Draw);  
	

	/* Even if there are no events, redraw our gl scene. */
	glutIdleFunc(&Idle); 
	
	/* Register the function called when our window is resized. */
	glutReshapeFunc(&ReSizeGLScene);
	
	/* Register the function called when the keyboard is pressed. */
	glutKeyboardFunc(&keyPressed);
	
	/* Register the function called when special keys (arrows, page down, etc) are pressed. */
	glutSpecialFunc(&specialKeyPressed);
	//adding here the mouse processing callbacks
	glutMouseFunc(processMouse);
	glutMotionFunc(processMouseActiveMotion);
	glutPassiveMotionFunc(processMousePassiveMotion);
	glutEntryFunc(processMouseEntry);
	
	/* Initialize our window. */
	InitGL(640, 480);
	
	/* Start Event Processing Engine */  
	glutMainLoop();  
	
	return 1;
} 
