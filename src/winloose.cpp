#include "winloose.h"
#include <GL/gl.h>
#ifndef WIN32
        #include <GL/glut.h>
#endif
#ifdef WIN32
       #include <GL/freeglut.h>
#endif

winL::winL()
{
     win.LoadBMP("Textures/win.bmp");
     loose.LoadBMP("Textures/dead.bmp");
     credits.LoadBMP("Textures/credits.bmp");
}

void winL::DrawQuad(float sx, float sy)
{
     glBegin(GL_QUADS);
     glNormal3f(0,0,-1);
     glTexCoord2f(0,0);
     glVertex3f(-sx/2.0, -sy/2.0, 15);
     glTexCoord2f(1,0);
     glVertex3f(sx/2.0, -sy/2.0, 15);
     glTexCoord2f(1,1);
     glVertex3f(sx/2.0, sy/2.0, 15);
     glTexCoord2f(0,1);
     glVertex3f(-sx/2.0, sy/2.0, 15);     
     glEnd();
}

void winL::DrawWin()
{
     glBlendFunc(GL_SRC_COLOR,GL_ONE_MINUS_SRC_COLOR) ;
     glEnable(GL_BLEND);
     
     glPushMatrix();
     
     extern float rotN, rotM;
     
     glRotatef(-rotN,1,0,0);
     glRotatef(-rotM,0,1,0);     
     
     glTranslatef(0,20,0);
     
     win.Bind();
     DrawQuad(60,40);
     glPopMatrix();
     
     glDisable(GL_BLEND);
}

void winL::DrawLoose()
{
     glBlendFunc(GL_SRC_COLOR,GL_ONE_MINUS_SRC_COLOR) ;
     glEnable(GL_BLEND);
     
     glPushMatrix();
     
     extern float rotN, rotM;
     
     glRotatef(-rotN,1,0,0);
     glRotatef(-rotM,0,1,0);     
     
     glTranslatef(0,20,0);
     
     loose.Bind();
     DrawQuad(60,40);
     glPopMatrix();
     
     glDisable(GL_BLEND);
}

void winL::DrawCredits()
{
     
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     glLoadIdentity();
     
     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();                
     glOrtho(0,100,0,100,-21,21);   
     glMatrixMode(GL_MODELVIEW);  
     
     
     glColor3f(1,1,1);
     
     credits.Bind();
     
     glPushMatrix();
     glTranslatef(45,45,0);
     DrawQuad(90,90);
     glPopMatrix();
     
     glFlush();      
     
     glutSwapBuffers();
}
