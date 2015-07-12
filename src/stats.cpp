#include "stats.h"
#include <cmath>
#include <GL/gl.h>
#ifndef WIN32
        #include <GL/glut.h>
#endif
#ifdef WIN32
       #include <GL/freeglut.h>
#endif
#include <stdio.h>
#include "cashe.h"

extern Cashe c;

void stats::GetStronger(int ns)
{
     Might += ns;
}

void stats::Draw()
{
     
     bool rot_items;
     if(stats_ani -> TimePassed())
       rot_items = 1;
     else rot_items = 0;
     
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);         // Clear The Screen And The Depth Buffer
     glLoadIdentity();
     
     glMatrixMode(GL_PROJECTION);                              // Select The Projection Matrix
     glLoadIdentity();                                         // Reset The Projection Matrix
     glOrtho(0,100,0,100,-200,200);                                // Set Up An Ortho Screen
     glMatrixMode(GL_MODELVIEW);                               // Select The Modelview Matrix
     
     //Background image
     c.bg.Bind();
     
     glBegin(GL_QUADS);
	   glNormal3f(0,0,1);
	   glTexCoord2f(0,0);
	   glVertex3i(0,0,-40);
	   glTexCoord2f(0,1);
	   glVertex3i(0,100,-40);
	   glTexCoord2f(1,1);
	   glVertex3i(100,100,-40);
	   glTexCoord2f(1,0);
	   glVertex3i(100,0,-40);
     glEnd();
     
     c.progBar.Bind();
     glColor3f(1,1,1);
     
     realPscale = c.Player -> scale;
     realIscale = c.invent -> Equipped() -> scale;
     
     glBegin(GL_LINE_LOOP); //player slot
	   glVertex3i(4,4,-38);
	   glVertex3i(4,96,-38);
	   glVertex3i(48,96,-38);
	   glVertex3i(48,4,-38);
     glEnd();
     
     glPushMatrix();
     glTranslatef(22,54,20);
     c.Player -> scale = 32;
     c.Player -> Draw();
     if(rot_items)c.Player -> rotA--;
     glPopMatrix();
     c.Player -> scale = realPscale;
     
     glBegin(GL_LINE_LOOP); //item
	   glVertex3i(5,5,-38);
	   glVertex3i(5,50,-38);
	   glVertex3i(47,50,-38);
	   glVertex3i(47,5,-38);
     glEnd();
     
     glPushMatrix();
     c.invent -> Equipped() -> scale = 40;
     glTranslatef(26,5,25);
     c.invent -> Equipped() -> Draw();
     if(rot_items)c.invent -> Equipped() -> rotA++;
     glPopMatrix();
     c.invent -> Equipped() -> scale = realIscale;
     
     glColor3f(1,1,1);
	   
     //all text comes from this point on
     glBlendFunc(GL_ONE_MINUS_SRC_COLOR,GL_SRC_COLOR) ;
     glEnable(GL_BLEND);
     
     Impact.print(54,90,"Level: %d ",level);
     Impact.print(54,80,"XP: %d ",(int)XP);
     Impact.print(54,70,"Next level:%d ",(int)(1000 * (pow( level,1.4) )));
     Impact.print(54,60,"HP %d/%d",HP,MaxHP);
     Impact.print(54,50,"Might: %d",Might);
     Impact.print(54,40,"Armor: %d",Armor);
     Impact.print(54,30,"Damage: %d",Damage());
     Impact.print(54,20,"Range: %d",c.invent -> Equipped() -> range);
     
     glDisable(GL_BLEND);
	   
     glFlush();      
     
     glutSwapBuffers();
}

void stats::GetXP(int xp)
{
//      printf("getXP called with %d \n",xp);
     XP += xp;
     while(AdvanceLevel());
}

void stats::Heal(int hp_part)
{
     if(HP == MaxHP)
	   return;
     
     float heal = (float)(hp_part * MaxHP) / 100.0;
     if(HP + heal > MaxHP)
	   HP = MaxHP;
     else HP += heal;
     
     c.Player -> HP = HP;
}

stats::stats()
{
     MaxHP = 50;
     HP = MaxHP;
     Might = 0;
     Armor = 0;
     level = 1;
     Impact.Load("Fonts/papyrus_i.bmp",5,-0.6);
     show = 0;
     c.Player -> MaxHP = MaxHP;
     c.Player -> HP = MaxHP;
     stats_ani = new timer(10);
}

stats::~stats()
{
     printf("Deleting stats %x \n",this);
}

void stats::GetArmored(int na)
{
     Armor += na;
}

void stats::GetHit(int dmg)
{    
     int damage_ = 1;
     
     if(dmg - Armor > 0)
	   damage_ = dmg - Armor;
     
     c.Player -> getHit(damage_);
     HP = c.Player -> HP;
}

void stats::MouseFunction(int button, int state , int x, int y)
{
     
}

bool stats::AdvanceLevel()
{
     if(XP >= 1000 * (pow(level ,1.4)))
	   level++;
     else return 0;
     
     if(level % 5 == 0)
	   Armor++;
     
     if(level % 8 == 0)
	   Might++;
     
     MaxHP += 20;
     HP = MaxHP;
     
     c.Player -> MaxHP = MaxHP;
     c.Player -> HP = MaxHP;
     
     sprintf(c.status, "Now you are level %d\n",level);
     c.status_timer -> Reset();
     
     return 1;
}

int stats::Damage()
{
     return Might + c.invent -> Equipped() -> damage; // + weapon dmg
}

void stats::GetTougher(int hp_part)
{
     float more = 1+ (float)(hp_part) / 100.0;
     MaxHP *= more;
     HP = MaxHP;
     c.Player -> MaxHP = MaxHP;
     c.Player -> HP = MaxHP;
     
}


void stats::Dump(std::ofstream &f)
{
     f << level << " " << XP << " " << Armor << " " << MaxHP << " " << HP << " " << Might << "\n";
}

void stats::LoadDump(std::ifstream &f)
{
     f >> level >> XP >> Armor >> MaxHP >> HP >> Might;
     c.Player -> MaxHP = MaxHP;
     c.Player -> HP = HP;
}
