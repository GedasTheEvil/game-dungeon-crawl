#include <stdio.h>
#include <GL/gl.h>
#include "cashe.h"

extern Cashe c;

extern float RotW;
extern float rotM,rotN;

extern bool Attacking;

unsigned char last_key;

int lastMx = 0;
int lastMy = 0;

void Draw();

void Idle()
{
     Draw();
}

void keyPressed(unsigned char key, int x, int y)
{
//      printf("You pressed %d\n",key);

     if(c.rid -> show)
     {
	   c.rid -> KeyboardF(key, x,y);
	   return;
     }
     
     if(key == 27)//esc
     {
	   if(c.Stats -> show || c.invent-> show)
	   {
		 c.Stats -> show = 0;
		 c.invent-> show = 0;
	   }
	   
	   c.menu.show = !c.menu.show;
	   return;
     }
     
     if(c.menu.show)
	   return;//jei rodomas meniu, tai reaguojam tik i [esc]

     if(c.Player -> Alive() && !c.IHaveWon)
     {

	   if(key == 97)//a
	   {
		 c.dungeon.Move(-0.015,0);
		 RotW = -110;
		 c.Player -> changeMDL(1);
	   }
	   if(key == 100)//d
	   {
		 c.dungeon.Move(0.015,0);
		 RotW = 70;
		 c.Player -> changeMDL(1);
	   }
	   
	   if(key == 115)//s
		 c.dungeon.Move(0,-0.015);
	   if(key == 119)//w
		 c.dungeon.Move(0,0.015);
	   
	   if(key == 32)//spacebar, jump
	   {

		 if(c.jump_timer -> TimePassed() && !c.falling)
		 {
		      c.jump_s.Play();
		      c.jumping = 1;
		      c.jump_up_timer -> Reset();
		 }
	   }
	   
	   if(key == 13 && c.Player -> Att_timer -> TimePassed())//enter, attack
	   {
		 c.dungeon.GetAttack(c.Stats -> Damage() ,c.invent -> Equipped() -> range);
		 c.Player -> att_s.Play();
		 Attacking = 1;
	   }
	   
     }//eo Alive
     
     if(key == 'i')
	   c.invent-> show = !c.invent-> show;
     
     if(key == 'o')
	   c.Stats -> show = !c.Stats-> show;
     
     last_key = key;
}

void specialKeyPressed(int key, int x , int y)
{
//      printf("Special key %d pressed\n",key);

     if(key == 1)
	   c.Cartoon = !c.Cartoon;
     
     if(key == 2)
	   c.Orig_model = !c.Orig_model;

     if(key == 100)
     {
	   rotM -= 2.5;
	   if(rotM < -30)
		 rotM = -30;
     }
     if(key == 102)
     {
	   rotM += 2.5;
	   if(rotM > 30)
		 rotM = 30;
     }
     if(key == 101)
     {
	   rotN += 2.5;
	   if(rotN > 10)
		 rotN = 10;
     }
     if(key == 103)
     {
	   rotN -= 2.5;
	   if(rotN < -10)
		 rotN = -10;
     }


     if(key == 5)c.ss[1].Play();

     
     if(key == 12)
     {
	   c.dungeon.GetPickUp();
	   c.dungeon.GetRiddle();
     }
}

void processMouse(int button, int state , int x, int y)
{
     if(c.menu.show)
     {
	   c.menu.MouseFunction(button,state ,x, y);
	   return;
     }
     
     if(c.invent-> show)
     {
	   c.invent-> MouseFunction(button,state,x,y);
	   return;
     }
     
     if(state && c.Player -> Alive() && !c.IHaveWon)
     {
	if(button == 0 && c.Player -> Att_timer -> TimePassed())
	{
	    c.dungeon.GetAttack(c.Stats -> Damage() ,c.invent -> Equipped() -> range);
	    c.Player -> att_s.Play();
	    Attacking = 1;
	}
	
	if(button == 1)
	{
	   c.dungeon.GetPickUp();
	   c.dungeon.GetRiddle();
	}
	
	if(button == 2)
	{
	   if(c.jump_timer -> TimePassed() && !c.falling)
	   {
		 c.jump_s.Play();
		 c.jumping = 1;
		 c.jump_up_timer -> Reset();
	   }
	}
	  
     }
}

void processMouseActiveMotion(int a, int b)
{
     
}

void processMousePassiveMotion(int a, int b)
{
     if(c.menu.show)
     {
	   c.menu.MousePassiveMotion(a,b);
	   return;
     }
     
     rotM += -0.08*(lastMx-a);
     rotN += -0.08*(lastMy-b);
     
     if(rotM > 30)
	rotM = 30;
     
     if(rotM < -30)
	rotM = -30;
     
     if(rotN > 10)
	rotN = 10;
     
     if(rotN < -10)
	  rotN = -10;
     
     lastMx = a;
     lastMy = b;
}

void processMouseEntry(int a)
{
     
}

