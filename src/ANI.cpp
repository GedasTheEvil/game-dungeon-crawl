#include <iostream>
#include <fstream>
#include "ANI.h"
#include <GL/glu.h>
#include <cmath>
#include <stdio.h>

using namespace std;
//============================================================
ANI::ANI()
{
      VCount = 0;
      Ver = NULL;
      Normals  =  NULL;
      TexCords =  NULL;
      frame = 0;
      speed = 1;
      scale = 0.000000000000;
      frameC = 0;
      compiled = 0;
      texture = 0;
      bounds = 0;
      loop = 1;
      frameChange = new timer(100);
}
////============================================================
ANI::~ANI()
{
    if(VCount)
	{
		VCount = 0;
		for(int i = 0; i < frameC ; i++)
		{
		     if(Ver[i].v)delete Ver[i].v;
		     printf("Deleting vertex array %x \n", &Ver[i].v);
		}
		delete Normals;
		delete TexCords;
              frameC = 0;
              delete Ver;
	}
	delete frameChange;
	printf("ANI %x destroyed\n", this);
}
//============================================================
int ANI::Load(const char FileName[])
{
     ifstream r(FileName);
     r >> VCount >> frameC;
     
     if(!VCount)
	   return 0;
     
     float tmp;
     Ver = new VF[frameC+1];
     Ver[0].v = new float[VCount *3];
     Normals  = new float[VCount *3];
     TexCords = new float[VCount *2];
     
     for(int i = 0; i < VCount* 3; i++)
           r >> Ver[0].v[i];

     for(int i = 0; i < VCount * 3; i++)
           r >> Normals[i];
     
     for(int i = 0; i < VCount * 2; i++)
           r >> TexCords[i];
     
     for(int j = 1; j < frameC+1; j++)
     {
	   Ver[j].v = new float[VCount *3];
	   for(int i = 0; i < VCount* 3; i++)
		 r >> Ver[j].v[i];
     }
     
     return 1;
}
//============================================================
void ANI::Show()
{

     if(bounds) //debug:: Bounding cube
     {
          glBegin(GL_LINE_STRIP);
          glVertex3f(-0.5,1,-0.5);
          glVertex3f(0.5,1,-0.5);
          glVertex3f(0.5,0,-0.5);
          glVertex3f(-0.5,0,-0.5);
          
	   glVertex3f(-0.5,0,0.5);
          glVertex3f(-0.5,1,0.5);
          glVertex3f(0.5,1,0.5);
          glVertex3f(0.5,0,0.5);
          glVertex3f(-0.5,0,0.5);
          
          glEnd();
	   
	   glBegin(GL_LINE_STRIP);
          glVertex3f(-0.5,1,0.5);
          glVertex3f(-0.5,1,-0.5);
          glVertex3f(0.5,1,-0.5);
          glVertex3f(0.5,1,0.5);
          
          glEnd();
	   
	   glBegin(GL_LINE);
          glVertex3f(-0.5,0,-0.5);
          glVertex3f(-0.5,1,-0.5);

          
          glEnd();
	   
     }
         
     glBindTexture(GL_TEXTURE_2D, texture);
         
     if(!compiled)
     {
          glEnableClientState(GL_VERTEX_ARRAY);
          glEnableClientState(GL_TEXTURE_COORD_ARRAY);
          glEnableClientState(GL_NORMAL_ARRAY);
          glVertexPointer(  3, GL_FLOAT, 0, &Ver[(int) frame].v[0]);
          glNormalPointer(     GL_FLOAT, 0, &Normals[0]);
          glTexCoordPointer(2, GL_FLOAT, 0, &TexCords[0]);
          glDrawArrays(GL_TRIANGLES, 0, VCount);
          glDisableClientState(GL_VERTEX_ARRAY);
          glDisableClientState(GL_TEXTURE_COORD_ARRAY);
          glDisableClientState(GL_NORMAL_ARRAY);          
     }
     else glCallList(List[(int) frame]);
}
//============================================================
float ANI::getScale()
{
     if(fabs(scale) > 0.00000000001) return scale;
     
     float minX = 1000.0, maxX = -1000.0;
     float minY = 1000.0, maxY = -1000.0;
     float minZ = 1000.0, maxZ = -1000.0;  
     
     
     //find maximum dimensions of the model
     for(int i =0; i< VCount *3; i+=3)
     {
           if(Ver[(int) frame].v[i] > maxX)maxX = Ver[(int) frame].v[i];
           if(Ver[(int) frame].v[i] < minX)minX = Ver[(int) frame].v[i];
	   
           if(Ver[(int) frame].v[i+1] > maxY)maxY = Ver[(int) frame].v[i+1];
           if(Ver[(int) frame].v[i+1] < minY)minY = Ver[(int) frame].v[i+1];
	   
           if(Ver[(int) frame].v[i+2] > maxZ)maxZ = Ver[(int) frame].v[i+2];
           if(Ver[(int) frame].v[i+2] < minZ)minZ = Ver[(int) frame].v[i+2];
     }
     
     //find the largest scale
     
     float ScX = maxX - minX;
     float ScY = maxY - minY;
     float ScZ = maxZ - minZ;
     
     if(ScX > ScY && ScX > ScZ)scale = ScX;
     
     else if(ScY > ScX && ScY > ScZ)scale = ScY;
     else scale = ScZ;
     
     return scale;
}
//============================================================
void ANI::Advance_Animation()
{
     if(frameC==1)
          return;
     
     if(!frameChange -> TimePassed())
	   return;
     
     if(!loop && frame < frameC)
	   frame += 0.04 * speed;
     
     if(!loop && frame >= frameC-1)
	   frame = frameC-1;
     
     if(loop)
	   frame += 0.04 * speed;
     
     
     if(loop && frame >= (frameC - 0.2))
	   frame = 0.0;
}
//============================================================
void ANI::setSpeed(int nSpeed)
{
     if (nSpeed < 1)speed = 1;
     else speed = nSpeed;
}
//============================================================
void ANI::Compile()
{
     if(compiled)return; //avoid too many compilations
     
     List = new int [frameC];

     for(int i = 0; i< frameC; i++)
     {
          List[i] = glGenLists(1);
          
          glBindTexture(GL_TEXTURE_2D, texture);
          
          glNewList(List[i],GL_COMPILE); 
          
          glEnableClientState(GL_VERTEX_ARRAY);
          glEnableClientState(GL_TEXTURE_COORD_ARRAY);
          glEnableClientState(GL_NORMAL_ARRAY);
          glVertexPointer(  3, GL_FLOAT, 0/*a*/, &Ver[i].v[0]);//0
          glNormalPointer(     GL_FLOAT, 0/*a*/, &Normals[0]);//0
          glTexCoordPointer(2, GL_FLOAT, 0/*b*/, &TexCords[0]);//0
          glDrawArrays(GL_TRIANGLES, 0, VCount/*/divisor*/);
          glDisableClientState(GL_VERTEX_ARRAY);
          glDisableClientState(GL_TEXTURE_COORD_ARRAY);
          glDisableClientState(GL_NORMAL_ARRAY);
          
          glEndList();          
     }
     
     compiled = 1;
}
//============================================================
void ANI::BindTexture(int t)
{
     texture = t;
}
//============================================================
void ANI::Centrify()
{
      float scale_ =  1/getScale();
      Scale(scale_);
      
      
     float minX = 1000.0, maxX = -1000.0;
     float minY = 1000.0, maxY = -1000.0;
     float minZ = 1000.0, maxZ = -1000.0;  
     
     
     //find maximum dimensions of the model
     for(int i =0; i< VCount *3; i+=3)
     {
           if(Ver[0].v[i] > maxX)maxX = Ver[0].v[i];
           if(Ver[0].v[i] < minX)minX = Ver[0].v[i];
          
           if(Ver[0].v[i+1] > maxY)maxY = Ver[0].v[i+1];
           if(Ver[0].v[i+1] < minY)minY = Ver[0].v[i+1];
          
           if(Ver[0].v[i+2] > maxZ)maxZ = Ver[0].v[i+2];
           if(Ver[0].v[i+2] < minZ)minZ = Ver[0].v[i+2];
     }
     
     Translate(-(minX+maxX)/2, -minY,-(maxZ+minZ)/2 ); //apacia bus 0, x centruojam, z 0
     
     scale = 1;
      
}
//============================================================
void ANI::Translate(float x, float y, float z)
{
     for(int j = 0; j< frameC; j++)
     {
          for(int i =0; i<VCount*3 ; i+=3)
          {
               Ver[j].v[i] += x;
               Ver[j].v[i+1] += y;
               Ver[j].v[i+2] += z;
          }
     }
}
//============================================================
void ANI::Scale(float sc)
{
     for(int j = 0; j< frameC; j++)
          for(int i =0; i<VCount*3; i++)
               Ver[j].v[i] *= sc;
}
//============================================================
void ANI::Reset()
{
      frame = 0.0;
}
//============================================================
