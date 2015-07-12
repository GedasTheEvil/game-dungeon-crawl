#include "inventory.h"
#include "item.h"
#include "cashe.h"
#include <GL/gl.h>
#ifndef WIN32
        #include <GL/glut.h>
#endif
#ifdef WIN32
       #include <GL/freeglut.h>
#endif
#include <stdio.h>


item *viewed; // only pointer

extern Cashe c;

inventory::inventory()
{
     inv_ani = new timer(10);
     Impact.Load("Fonts/papyrus_i.bmp",5,-0.6); //maybe papyrus is better :)
//      .Load("Fonts/script_i.bmp",7,-1.8);
     small.Load("Fonts/impact_i.bmp",3.5,-0.3);
     equipped.type = 1;
     equipped.id = 0;
     show = 0;
     view .type = 1;
     view. id=0;
     view. count = 1;
     
     mw[0].count = 1;
     mw[0].realScale = c.club -> scale;
     
     mw[1].count = 0;
     mw[1].realScale = c.sword -> scale;
     
     mw[2].count = 0;
     mw[2].realScale = c.spear -> scale;
     rw.count = 0;
     rw.realScale = c.bow -> scale;
     viewed = c.club;
     
     for(int i=0; i<5; i++)
     {
	   potions[i].count = 0;
	   potions[i].realScale = c.potion -> scale;
     }
          
}

inventory::~inventory()
{
     printf("Deleting inventory %x \n",this);
     delete inv_ani;
}

void inventory::UsePotion()
{
     if(! c.Player -> Alive())
     {
	   printf("Player dead, cannot drink anymore \n");
	   return;
     }
     
     if(view.type != 3)
	   return;
     
     
     if (potions[view.id].count <= 0) 
	   return;
     
     c.drink_s.Play();
     
     switch(view.id)
     {
	   case 0: 
		 c.Stats -> Heal(25) ;
		 potions[view.id].count--;
		 break;
	   
	   case 1: 
		 c.Stats -> Heal(50) ;
		 potions[view.id].count--;
		 break;
	   
	   case 2: 
		 c.Stats -> GetStronger(2);
		 potions[view.id].count--;
		 break;
	   
	   case 3: 
		 c.Stats ->GetArmored(2);
		 potions[view.id].count--;
		 break;
	   
	   case 4: 
		 c.Stats ->GetTougher(5);
		 potions[view.id].count--;
		 break;
     }
     
}

void inventory::GetItem(int type, int id)
{
     if(type == 1)
     {
	   if(id >= 3){printf("Error: Unknown weapon, id=%d\n",id);return;}
	   mw[id].count++;
     }
     if(type == 2)
     {
	   if(id >= 1){printf("Error: Unknown bow, id=%d\n",id);return;}
	   rw.count++;
     }
     if(type == 3)
     {
	   if(id >= 5){printf("Error: Unknown potion, id=%d\n",id);return;}
	   potions[id].count++;
     }
}

void inventory::Draw()
{
     
     bool rot_items;
     if(inv_ani -> TimePassed())
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
     
     if(view.type == 3)
	   glColor3f(1 - 0.3*(view.id % 3),0+0.6*(view.id / 3),0+0.3*(view.id % 3));

    
     glPushMatrix();//drawModel of view
     glTranslatef(71,40,0);
     viewed -> scale = 34;
     viewed -> Draw();
     if(rot_items)viewed -> rotA++;
     glPopMatrix();
     c.progBar.Bind();    
     
     glColor3f(0,0,0);
     glBegin(GL_LINE_LOOP); //weapon slot1
	   glVertex3i(4,92,-39);
	   glVertex3i(14,92,-39);
	   glVertex3i(14,75,-39)
	   ;glVertex3i(4,75,-39);
     glEnd();
     
     glColor3f(1,1,1);
     glPushMatrix();//drawModel1
     glTranslatef(9,75,0);
     c.club -> scale = 17;
     c.club -> Draw();
     if(rot_items)c.club -> rotA++;
     c.club -> scale = mw[0].realScale;
     glPopMatrix();
     c.progBar.Bind();   
     
     glColor3f(0,0,0);
     glBegin(GL_LINE_LOOP); //weapon slot2
	   glVertex3i(17,92,-39);
	   glVertex3i(27,92,-39);
	   glVertex3i(27,75,-39)
	   ;glVertex3i(17,75,-39);
     glEnd();
     
     glColor3f(1,1,1);
     glPushMatrix();//drawModel2
     glTranslatef(22,75,0);
     c.sword -> scale = 17;
     c.sword -> Draw();
     if(rot_items)c.sword -> rotA++;
     c.sword -> scale = mw[1].realScale;
     glPopMatrix();
     c.progBar.Bind();    
     
     glColor3f(0,0,0);
     glBegin(GL_LINE_LOOP); //weapon slot3
	   glVertex3i(30,92,-39);
	   glVertex3i(40,92,-39);
	   glVertex3i(40,75,-39)
	   ;glVertex3i(30,75,-39);
     glEnd();
     
     glColor3f(1,1,1);
     glPushMatrix();//drawModel2
     glTranslatef(35,75,0);
     c.spear -> scale = 17;
     c.spear -> Draw();
     if(rot_items)c. spear -> rotA++;
     c.spear -> scale = mw[2].realScale;
     glPopMatrix();
     c.progBar.Bind();   
     
     glColor3f(0,0,0);
     glBegin(GL_LINE);//underline for weapons text1
	   glVertex3i(4,94,-39);
	   glVertex3i(45,94,-39);
     glEnd();
     
     glBegin(GL_LINE);//underline for weapons text2
	   glVertex3i(4,69,-39);
	   glVertex3i(45,69,-39);
     glEnd();
     
     glBegin(GL_LINE_LOOP); //weapon2 slot1
	   glVertex3i(4,65,-39);
	   glVertex3i(14,65,-39);
	   glVertex3i(14,48,-39)
	   ;glVertex3i(4,48,-39);
     glEnd();
     
     glColor3f(1,1,1);
     glPushMatrix();//drawModel2.2
     glTranslatef(9,48,0);
     c.bow -> scale = 17;
     c.bow -> Draw();
     if(rot_items)c.bow -> rotA++;
     c.bow -> scale = rw.realScale;
     glPopMatrix();
     c.progBar.Bind();    
     
     glColor3f(0,0,0);
     glBegin(GL_LINE);//underline forpotions text
	   glVertex3i(4,42,-39);
	   glVertex3i(45,42,-39);
     glEnd();
     
     //potions slots
     for(int j =0; j <2; j++)
	   for(int i = 0; i<3; i++)
	   {
		 glColor3f(0,0,0);
		 glBegin(GL_LINE_LOOP); // sloti.j
		      glVertex3i(4+13*i,40 -20*j,-39);
		      glVertex3i(14+13*i,40-20*j,-39);
		      glVertex3i(14+13*i,25-20*j,-39)
		      ;glVertex3i(4+13*i,25-20*j,-39);
		 glEnd();
		 
		 glColor3f(1 - 0.3*i,0+0.6*j,0+0.3*i);
		 
		 glPushMatrix();//drawModel1
		 glTranslatef(9+13*i,25-20*j,0);
		 c.potion -> scale = 10;
		 c.potion -> Draw();
		 c.potion -> scale = potions[0].realScale;
		 glPopMatrix();
		 glColor3f(1,1,1);
		 c.progBar.Bind();  
		 
	   }
	   
     glColor3f(1,1,1);
	   
     if(rot_items)c.potion -> rotA++;
	   
     ///selected item slot   
     glColor3f(0,0,0);
     glBegin(GL_LINE_LOOP); //frame
	   glVertex3i(52,92,-39);
	   glVertex3i(92,92,-39);
	   glVertex3i(92,20,-39)
	   ;glVertex3i(52,20,-39);
     glEnd();
     
     glBegin(GL_LINE_LOOP); //model slot
	   glVertex3i(54,80,-39);
	   glVertex3i(88,80,-39);
	   glVertex3i(88,40,-39)
	   ;glVertex3i(54,40,-39);
     glEnd();
     
     glBegin(GL_LINE_LOOP); //equip / use button
	   glVertex3i(54,14,-30);
	   glVertex3i(88,14,-30);
	   glVertex3i(88,8,-30)
	   ;glVertex3i(54,8,-30);
     glEnd();
     
     glColor3f(1,1,1);
	   
     //all text comes from this point on
     glBlendFunc(GL_ONE_MINUS_SRC_COLOR,GL_SRC_COLOR) ;
     glEnable(GL_BLEND);
     Impact.print(54,8,"EQUIP / USE"); //equip / use button
     
     Impact.print(2,93, "Close combat weapons:");
     
     Impact.print(2,68, "Ranged weapons:");
     
     Impact.print(2,41, "Potions:");
     
     
     if( view.type == 1)
     {
	   Impact.print(53,86, "Type: Close Weapon");     
	   
	   if(view.id == 0)
	   {
		 Impact.print(53,82, "Name: Club"); 
		 Impact.print(53,34, "Damage: %d hp",c.club->damage);
		 Impact.print(53,31, "Range: %d ",c.club->range);
		 Impact.print(53,26,"Good old club.");
		 Impact.print(53,23,"Now with spikes.");
	   }
	   if(view.id == 1)
	   {
		 Impact.print(53,82, "Name: Sword");
		 Impact.print(53,34, "Damage: %d hp",c.sword->damage);
		 Impact.print(53,31, "Range: %d ",c.sword->range);
		 Impact.print(53,26,"Shiny sword.");
		 Impact.print(53,23,"Hahah!");
	   }
	   if(view.id == 2)
	   {
		 Impact.print(53,82, "Name: Spear");
		 Impact.print(53,34, "Damage: %d hp",c.spear->damage);
		 Impact.print(53,31, "Range: %d ",c.spear->range);
		 Impact.print(53,26,"Long spear.");
		 Impact.print(53,23,"None shall pass!");
	   }
	   
	   
     }
     
     if( view.type == 2)
     {
	   Impact.print(53,86, "Type: Ranged Weapon");
	   
	   Impact.print(53,82, "Name: Bow"); 
	   Impact.print(53,34, "Damage: %d hp",c.bow->damage);
	   Impact.print(53,31, "Range: %d ",c.bow->range);
	   Impact.print(53,26,"The simple bow.");
	   Impact.print(53,23,"For slow monsters");
     }
     
     if( view.type == 3)
     {
	   Impact.print(53,86, "Type: Magic Potion");
	   if(view.id == 0)
	   {
		 Impact.print(53,82, "Name: Small health"); 
		 Impact.print(53,26,"Heals 25\% HP");
		
	   }
	   if(view.id == 1)
	   {
		 Impact.print(53,82, "Name: Large health"); 
		 Impact.print(53,26,"Heals 50\% HP");
	   }
	   if(view.id == 2)
	   {
		 Impact.print(53,82, "Name: Aphethamine"); 
		 Impact.print(53,26,"Adds 2 strength");
	   }
	   if(view.id == 3)
	   {
		 Impact.print(53,82, "Name: Stone skin"); 
		 Impact.print(53,26,"Adds 2 armor");
	   }
	   if(view.id == 4)
	   {
		 Impact.print(53,82, "Name: Elixir of life"); 
		 Impact.print(53,26,"Adds 5\% HP");
	   }
     }
     
     //item quantities
     small.print(4,75,"%d",mw[0].count);
     small.print(17,75,"%d",mw[1].count);
     small.print(30,75,"%d",mw[2].count);
     small.print(4,48,"%d",rw.count);
     
     glColor3f(1,1,1);
     
     //potions
     for(int j =0; j <2; j++)
	   for(int i = 0; i<3-j; i++)
		 small.print(4+13*i,25-20*j,"%d",potions[3*j + i].count);
     
     glDisable(GL_BLEND);
	   
     glFlush();      
     
     glutSwapBuffers();
     
}

void inventory::MouseFunction(int button, int state , int x, int y)
{
     if(!state)
	   return;
     
     extern int resX ,resY;
     float inv_x = 100 * ((float)x / (float)resX);
     float inv_y = 100 - 100 * ((float)y / (float)resY);
     
     
     
     if(inv_x > 4 && inv_x < 84) //item slot area
     {
	   if(inv_y > 74 && inv_y < 92)//weapon slot 1
	   {
		 if(inv_x <= 15)//club
		 {
		      viewed = c.club;
		      view.type = 1;
		      view.id = 0;
		      view.count = mw[0].count;
		 }
		 
		 else if(inv_x >= 30)//spear
		 {
		      viewed = c.spear;
		      view.type = 1;
		      view.id = 2;
		      view.count = mw[2].count;
		 }
		 
		 else //sword
		 {
		      viewed = c.sword;
		      view.type = 1;
		      view.id = 1;
		      view.count = mw[1].count;
		 }
	   }
	   
	   if(inv_y > 48 && inv_y < 65 && inv_x <= 15)//ranged weapon (slot 2)
	   {
		 viewed = c.bow;
		 view.type = 2;
		 view.id = 1;
		 view.count = rw.count;
	   }
	   
	   if(inv_y > 24 && inv_y < 40)//potion slot 1
	   {
		 if(inv_x <= 15)
		 {
		      viewed = c.potion;
		      view.type = 3;
		      view.id = 0;
		      view.count = potions[0].count;
		 }
		 
		 else if(inv_x >= 30)
		 {
		      viewed = c.potion;
		      view.type = 3;
		      view.id = 2;
		      view.count = potions[2].count;
		 }
		 
		 else 
		 {
		      viewed = c.potion;
		      view.type = 3;
		      view.id = 1;
		      view.count = potions[1].count;
		 }
	   }
	   
	   if(inv_y > 5 && inv_y < 20)//potion slot 2
	   {
		 if(inv_x <= 15)//club
		 {
		      viewed = c.potion;
		      view.type = 3;
		      view.id = 3;
		      view.count = potions[3].count;
		 }
		 
		 else if(inv_x >= 30);//nebeturim tiek
		 
		 else 
		 {
		      viewed = c.potion;
		      view.type = 3;
		      view.id = 4;
		      view.count = potions[4].count;
		 }
	   }
	   
     }
     
      //equip / use button
     if(inv_y >= 8 && inv_y <= 14 &&  inv_x >= 54 && inv_x <= 88 && view.count > 0)
     {
	   if(!c.Player -> Alive())
	   {
		 printf("Player dead. Deal with it. No more actions\n");
		 return;
	   }
	   if(view.type != 3)
		 equipped = view;
	   else UsePotion();
     }
}

item *inventory::Equipped()
{
     if(equipped.type == 1)
     {
	   if(equipped.id == 0)
		 return c.club;
	   
	   if(equipped.id == 1)
		 return c.sword;
	   
	   if(equipped.id == 2)
		 return c.spear;
     }
     else if (equipped.type == 2)
	   return c.bow;
     else return c.club;
}


void inventory::Dump(std::ofstream &f)
{
     f << mw[0].count << " "  << mw[1].count << " "  << mw[2].count << " ";
     f <<  rw.count << " ";
     f <<  potions[0].count << " " << potions[1].count << " "  << potions[2].count << " "  << potions[3].count << " "  << potions[4].count << " \n";
}

void inventory::LoadDump(std::ifstream &f)
{
     for(int i = 0; i< 3 ; i++)
	   f >> mw[i].count;
     f >> rw.count;
     for(int i = 0; i< 5 ; i++)
	   f >> potions[i].count;
     
}

