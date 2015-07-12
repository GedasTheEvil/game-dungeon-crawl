#include "input.h"
#include <GL/gl.h>
#include "cashe.h"
#ifndef WIN32
        #include <GL/glut.h>
#endif
#ifdef WIN32
       #include <GL/freeglut.h>
#endif

float RotW = -110;
float rotM = 0;
float rotN = 0;

int weapon_rot = 0;

Cashe c;

bool Attacking = 0;

extern float resX, resY;

void DrawLoad(float xxx, char text[]);

void Draw()    
{
    
     if(!c.Cache_loaded)
     {
	   c.Load();
	   return;
     }
     
     if(c.menu . show)
     {
	   c.menu . Draw();
	   return;
     }
     
     if(c.invent -> show)
     {
	   c.invent -> Draw();
	   return;
     }
     
     if(c.Stats -> show)
     {
	   c.Stats -> Draw();
	   return;
     }
     
     if(c.rid -> show)
     {
	   c.rid -> Draw();
	   return;
     }
     
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);         // Clear The Screen And The Depth Buffer
     glLoadIdentity();
     
     glMatrixMode(GL_PROJECTION);                              // Select The Projection Matrix
     glLoadIdentity();                                         // Reset The Projection Matrix
     gluPerspective(45.0f,resX/resY,10.0f,300.0f);
     glMatrixMode(GL_MODELVIEW);                               // Select The Modelview Matrix

     glTranslatef(0,-20,-70);//for perspective, bet skaiciai is lempos
    
     glRotatef(rotM,0,1,0);
     glRotatef(rotN,1,0,0);
     
     c.Dt[0].Bind();
     
     glPushMatrix();//for perspective
     glTranslatef(-202, 0.0, -10);//for perspective, tarkim bus tiek :)
     
     c.dungeon.Draw();
     
     glPopMatrix();//for perspective

     c.Player -> rotA = RotW;  
     
     c.Player->Draw();   
     
     if(c.IHaveWon)
	   c.wlc -> DrawWin();
     
     if(c.Player -> Alive())
     {
	   if(c.mdlChange -> TimePassed())
		 c.Player -> changeMDL(0);
	   
	   glPushMatrix();//weapon
     
	   if(c.Player -> rotA > 0)
	   {
		 glTranslatef(c.Player-> scale/20, c.Player-> scale/4*3 + 0.27, 2);
		 glRotatef(-45 -weapon_rot,0,0,1);
		 
	   }
	   else
	   {
		 glTranslatef(-c.Player-> scale/20, c.Player-> scale/4*3 + 0.27, 2);
		 glRotatef(45+weapon_rot,0,0,1);
		 
	   }
	   c.invent -> Equipped() -> Draw();
	   c.invent -> Equipped() -> rotA++;//just for the heck of it
	   
	   if(Attacking)
	   {
		 if (c.Player -> Att_timer -> TimePassed() || weapon_rot <= -40)
		 {
		      weapon_rot = 70;
		      Attacking = 0;
		 }
		 else weapon_rot-=4;		 
	   }
	   else if(c.AttTimer -> TimePassed())
		 weapon_rot = 0;
	   
	   glPopMatrix();

     }//end of alive
     else c.wlc->DrawLoose();

     glLoadIdentity();
     
     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();                
     glOrtho(0,100,0,100,-21,21);   
     glMatrixMode(GL_MODELVIEW);  
     
     glBlendFunc(GL_SRC_COLOR,GL_ONE_MINUS_SRC_COLOR) ;
     glEnable(GL_BLEND);
     
     if(!c.status_timer -> TimePassed(true))
	   c.load_font.print(25,72,c.status);
     

     glFlush();      
     
     glutSwapBuffers();
}
