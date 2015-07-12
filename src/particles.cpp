#include "particles.h"
#include <cmath>
#include <GL/gl.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


#define CDefaultSystemLife 100
#define CDefaultParticleLife 100

ParSys::ParSys()
{
     life = CDefaultSystemLife; //default lifetime
     for(int i =0; i < CMaxPart; i++)
     {
	   pt[i].x = 0;
	   pt[i].y = 0;
	   pt[i].z = 0;
	   pt[i].life = CDefaultParticleLife;
     }
     
     ani_e = new timer(5);
     ani_f = new timer(5);
}

ParSys::ParSys(int life)
{
     this -> life = life;
     for(int i =0; i < CMaxPart; i++)
     {
	   pt[i].x = 0;
	   pt[i].y = 0;
	   pt[i].z = 0;
	   pt[i].life = CDefaultParticleLife;
     }
     
     ani_e = new timer(5);
     ani_f = new timer(5);
}

ParSys::~ParSys()
{
     life = 0;
     printf("Deleting Particle System %x \n",this);
     delete ani_e;
     delete ani_f;
}

void ParSys::Fall()
{
     if(!ani_f -> TimePassed())
       return;
  
     if(life <= 0)
	   return;
     
     for(int i =0; i < CMaxPart; i++)
     {
	   pt[i].x += 0.1 * sin(i);
	   pt[i].y -= 0.1;
	   pt[i].z += 0.1 * cos(i);
	   pt[i].life--;
	   if(pt[i].life <= 0)
	   {
		 pt[i].x = 0;
		 pt[i].y = 0;
		 pt[i].z = 0;
		 pt[i].life = CDefaultParticleLife;
	   }
     }
     
     life--;
}

void ParSys::Explode()
{
      if(!ani_e -> TimePassed())
	return;
  
     if(life <= 0)
	   return;
     
     for(int i =0; i < CMaxPart; i++)
     {
	   pt[i].x += 0.1 * sin(i*3.14) * (((int)rand())%5);
	   pt[i].y -= 0.1 * sin(2*i*3.14) * (((int)rand())%5);
	   pt[i].z += 0.1 * cos(-i*3.14) * (((int)rand())%5);
	   pt[i].life--;
	   if(pt[i].life <= 0)
	   {
		 pt[i].x = 0;
		 pt[i].y = 0;
		 pt[i].z = 0;
		 pt[i].life = CDefaultParticleLife;
	   }
     }
     
     life--;
}

void ParSys::Draw()
{
    
     glPointSize(8);
     
     if(life <= 0)
	   return;
     
//      glEnable(GL_POINT_SMOOTH);
     glBlendFunc(GL_SRC_COLOR,/*GL_ONE_MINUS_SRC_COLOR*/ GL_ONE_MINUS_SRC_ALPHA);
     glEnable(GL_BLEND);
     
     glColor4f( 0.7, 0.1, 0.1, 0.6);
     
     glPushMatrix();
     glTranslatef(x,y,z);
     glBegin(GL_POINTS);
     
     for(int i =0; i < CMaxPart; i++)
	   glVertex3f(pt[i].x,pt[i].y,pt[i].z);
     
     glEnd();
     glPopMatrix();
     
     glColor4f(1,1,1,1);
     glDisable(GL_BLEND);
}

void ParSys::setCords(float x, float y, float z )
{
     this -> x = x;
     this -> y = y;
     this -> z = z;
}

void ParSys::Reset()
{
     life = CDefaultSystemLife; //default lifetime
}
