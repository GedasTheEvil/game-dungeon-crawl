#include "monster.h"
#include <GL/gl.h>
#include <stdio.h>
#include <cmath>

#include "cashe.h"

#ifdef WIN32

     #include <cstdlib>

     inline int random()
     {
	   return rand();
     }

#endif

extern Cashe c;

monster::monster()
{
     x = 0;
     y = 0;
     speed = 1;
     HP = 200;
     MaxHP = 200;
     damage = 1;
     XP = 1000;
     stat = -1;
     scale = 1;
     
     Att_timer = new timer(1000);
     walk_timer = new timer(40);
     
     blood = new ParSys(100);
}
//================================================================================
monster::monster(float x, float y)
{
     X = x;
     Y = y;

     this -> x = 0;
     this -> y = 0;
     speed = 1;
     HP = 20;
     MaxHP = 20;
     damage = 1;
     XP = 1000;
     stat = -1;
     scale = 1;
     Att_timer = new timer(1000);
     walk_timer = new timer(40);
     
     blood = new ParSys(100);
}
//================================================================================
monster::monster(float nX, float nY, int nSpeed, int nHP, int nDamage, int nXP)
{
     x = nX;
     y = nY;
     speed = nSpeed;
     MaxHP = nHP;
     HP = nHP;
     damage = nDamage;
     XP = nXP;
     stat = -1;
     Att_timer = new timer(1000);
     walk_timer = new timer(40);
     
     blood = new ParSys(100);
}
//================================================================================
monster::~monster()
{
     if (stat == 1) 
     {
	   delete walk;
	   delete attack;
	   delete die;
	   delete Att_timer;
	   delete blood;
	   delete walk_timer;
     }
     stat = -1;
     printf("Deleting monster %x \n",this);
}
//================================================================================
bool monster::Draw()// needs to choose animation
{ 
     glPushMatrix();
     
     if(this != c.Player)
	   glTranslatef(40*x - 20,y,-30);
     else glTranslatef(0,0,-30);
     
     glPushMatrix();// will add rotation
	    
	   glScalef(scale,scale,scale);
     
     if(Alive())
     {
	   if(mdl == die)
	   {
		 if(!AtDir())
		      mdl = attack;
		 else mdl = walk;
	   }

	   TNull . Bind();
	   
	   glColor4f(1,1,1,0.9);
	   
	   glBegin(GL_LINE_LOOP);
	   glVertex3f(-0.501,1.101,0);
	   glVertex3f(0.501,1.101,0);
	   glVertex3f(0.501,1.201,0);
	   glVertex3f(-0.501,1.201,0);
	   glEnd();
	   
	   glEnable(GL_BLEND);
	   
	   float xxx = (float)HP/(float)MaxHP;
	   glColor3f(3*(1-xxx),3*xxx,0);
	   glBegin(GL_QUADS);
	   glTexCoord2f(0,0);
	   glVertex3f(-0.5,1.1,0);
	   glTexCoord2f(1,0);
	   glVertex3f(xxx-0.5,1.1,0);
	   glTexCoord2f(1,1);
	   glVertex3f(xxx-0.5,1.2,0);
	   glTexCoord2f(0,1);
	   glVertex3f(-0.5,1.2,0);
	   glEnd();
	   
	   glDisable(GL_BLEND);
	   
	   glColor3f(1,1,1);
	   
	   glPushMatrix();
	   glScalef(0.5/scale,0.5/scale,0.5/scale);

	   c.nullTex.Bind();
	   blood->Explode();
	   blood->Fall();
	   blood->Draw();
	   glPopMatrix();
     }
     else mdl = die;
     
     tex . Bind();
	   
     if(this != c.Player)
	   glRotatef(rotA + 90* AtDir(),0,1,0);
     else glRotatef(rotA ,0,1,0);
	  
     if(c.Cartoon)
	   mdl -> ShowC();
     else mdl -> Show();
     
   
       glPopMatrix();
       glPopMatrix();
       mdl -> Advance_Animation();
     return 1;
}
//================================================================================
bool monster::LoadMDL(const char filename[],Textura &texture,Textura &nullT, bool compile)
{
     TNull = nullT;
     
     if(stat != -1)
     {
	   printf("Error [%d]: Object Already loaded\n",stat);
	   return 0;	   
     }

     char tmp1[255],tmp2[255],tmp3[255],tmp4[255],tmp5[255];

     sprintf(tmp1,"Models/%s.mdl",filename);
     sprintf(tmp2,"Models/%s_att.mdl",filename);
     sprintf(tmp3,"Models/%s_die.mdl",filename);
     
     sprintf(tmp4,"Sounds/%s_att.wav",filename);
     sprintf(tmp5,"Sounds/%s_die.wav",filename);
     
     printf("Loading model: %s\n", tmp1);
     walk = new CartoonANI;
     walk -> Load(tmp1);
     walk -> BindTexture(texture.ID());
     walk -> Centrify();
     walk -> setSpeed(35);

     printf("Loading model: %s\n", tmp2);
     attack =  new CartoonANI;
     attack -> Load(tmp2);
     attack -> BindTexture(texture.ID());
     attack -> Centrify();
     attack -> setSpeed(35);

     printf("Loading model: %s\n", tmp3);
     die =  new CartoonANI;
     die -> Load(tmp3);
     die -> BindTexture(texture.ID());	
     die -> Centrify();
     die -> setSpeed(35);
     
     die -> loop = 0;

     die_s.LoadWAV(tmp5);
     att_s.LoadWAV(tmp4);
     
     if(compile)
     {
	   walk   -> Compile();
	   attack -> Compile();
	   die    -> Compile();	   
     }
     stat = 1;
     
     mdl = walk;
     
     return 1;
}
//================================================================================
void monster::setCords(float nX, float nY)
{
       x = nX;
       y = nY; 
}
//================================================================================
bool monster::Alive()
{
       if(HP > 0) 
	     return 1;
       
       return 0;
}
//================================================================================
bool monster::getHit(int dmg)
{
       if(Alive())
	{
	     HP -= dmg;
	     blood->setCords(random()%(int)scale,random()%(int)scale,0);
	     blood -> Reset();
	}
	
	if(!Alive() && mdl != die)
	{
	     mdl = die;
	     sprintf(c.status, "Gained %d XP", XP );
	     c.status_timer -> Reset();
	     c.Stats -> GetXP(XP);
	     die_s.Play();
	}
	
       
       return Alive();
}
//================================================================================
void monster::Reanimate()
{
     HP = MaxHP;
     mdl = walk;
}
//================================================================================
