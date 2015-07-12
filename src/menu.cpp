#include "menu.h"
#include <time.h>
#include <stdio.h>
#include <GL/gl.h>
#ifndef WIN32
        #include <GL/glut.h>
#endif
#ifdef WIN32
       #include <GL/freeglut.h>
#endif
#include <stdlib.h>
#include "cashe.h"
#include <string.h>

extern Cashe c;

MainMenu::MainMenu()
{
     show = 1;
     inGame = 0;
     h1 = 0;
     h2 = 0;
     h3 = 0;
     h4 = 0;
     h5 = 0;
     h6 = 0;
     h7 = 0;
     saveD = 0;
     loadD = 0;
     t_cred = new timer(10000);
}

MainMenu::~MainMenu()
{
     delete t_cred;
     printf("Menu %x deleted \n", this);
}

void MainMenu::Draw()
{
     if(credits)
     {
	   c.wlc -> DrawCredits();
	   if(t_cred -> TimePassed())
		 credits = 0;
	   return;
     }	   
     
     if(saveD || loadD)
     {
	   DrawSave();
	   return;
     }
     
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     glLoadIdentity();
     
     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();                
     glOrtho(0,100,0,100,-21,21);   
     glMatrixMode(GL_MODELVIEW);  
     
     
     glColor3f(1,1,1);
     
     c.menu_bg.Bind();
     
     glBegin(GL_QUADS);
	   glNormal3f(0,0,1);
	   glTexCoord2f(0,0);
	   glVertex3i(0,0,-20);
	   glTexCoord2f(0,1);
	   glVertex3i(0,100,-20);
	   glTexCoord2f(1,1);
	   glVertex3i(100,100,-20);
	   glTexCoord2f(1,0);
	   glVertex3i(100,0,-20);
     glEnd();
     
     
     glColor3f(1,1,1);
	   
     glBlendFunc(GL_SRC_COLOR,GL_ONE_MINUS_SRC_COLOR) ;
     glEnable(GL_BLEND);
     
     if(inGame)
     {
	   InGameDraw();
	   return;
     }
     
     if(h1)
	   glColor3f(0,1,0);
     else glColor3f(1,1,1);
     c.load_font.print(25,72,"New Game");
     
     if(h2)
	   glColor3f(0,1,0);
     else glColor3f(1,1,1);     
     c.load_font.print(25,54,"Load Game");
     
     if(h3)
	   glColor3f(0,1,0);
     else glColor3f(1,1,1);
     c.load_font.print(25,36,"Credits");
     
     if(h4)
	   glColor3f(0,1,0);
     else glColor3f(1,1,1);
     c.load_font.print(25,18,"Exit");
     
     glDisable(GL_BLEND);
	   
     glFlush();      
     
     glutSwapBuffers();   
     
}

void MainMenu::InGameDraw()
{
          
     if(h1)
	   glColor3f(0,1,0);
     else glColor3f(1,1,1);
     c.load_font.print(25,72,"Return to game");
     
     if(h2)
	   glColor3f(0,1,0);
     else glColor3f(1,1,1);     
     c.load_font.print(25,54,"Save Game");
     
     if(h3)
	   glColor3f(0,1,0);
     else glColor3f(1,1,1);
     c.load_font.print(25,36,"Load Game");
     
     if(h4)
	   glColor3f(0,1,0);
     else glColor3f(1,1,1);
     c.load_font.print(25,18,"Exit to MainMenu");
     
     glDisable(GL_BLEND);
	   
     glFlush();      
     
     glutSwapBuffers();   
     
}

void MainMenu::MouseFunction(int button, int state , int x, int y)
{
     if(!state)
	   return;
     
     if(credits)
	   return;
     
     if(saveD || loadD)
     {
	   SaveMouseFunction(button,state ,  x,  y);
	   return;
     }
     
     if(inGame)
     {
	   InGameMouseFunction(button,state ,x, y);
	   return;
     }
     
     extern int resX ,resY;

     float m_x = 100 * ((float)x / (float)resX);
     float m_y = 100 - 100 * ((float)y / (float)resY);
     
     
     if(m_x >= 23 && m_x  <= 75)
     {
	   if(m_y <= 82 && m_y >= 71)//button1
	   {
		 show = 0;
// 		 c.LoadSave("Saves/new.sav");
		 c.dungeon.Load("Levels/lvl1");
		 inGame = 1;
		 c.IHaveWon = 0;
		 c.Player -> Reanimate();
	   }
	   
	   if(m_y <= 63 && m_y >= 53)//button2
	   {
		 loadD = 1;
	   }
	   
	   if(m_y <= 46 && m_y >= 35)//button3
	   {
		 credits = 1;
	   }
	   
	   if(m_y <= 27 && m_y >= 17)//button4
	   {
		 exit(666);
	   }
     }
}

void MainMenu::InGameMouseFunction(int button, int state , int x, int y)
{
     if(credits)
	   return;
     
     extern int resX ,resY;

     float m_x = 100 * ((float)x / (float)resX);
     float m_y = 100 - 100 * ((float)y / (float)resY);
     
     if(m_x >= 23 && m_x  <= 75)
     {
	   if(m_y <= 82 && m_y >= 71)//button1
	   {
		 show = 0;
	   }
	   
	   if(m_y <= 63 && m_y >= 53)//button2
	   {
		 saveD = 1;
	   }
	   
	   if(m_y <= 46 && m_y >= 35)//button3
	   {
		 loadD = 1;
	   }
	   
	   if(m_y <= 27 && m_y >= 17)//button4
	   {
		 inGame = 0;
	   }
     }
}

void MainMenu::MousePassiveMotion(int x, int y)
{
     if(credits)
	   return;
     
     if(saveD || loadD)
     {
	   MousePassiveMotionSave(x,y);
	   return;
     }
     
     extern int resX ,resY;

     float m_x = 100 * ((float)x / (float)resX);
     float m_y = 100 - 100 * ((float)y / (float)resY);
     
     if(m_x >= 23 && m_x  <= 75)
     {
	   if(m_y <= 82 && m_y >= 71)//button1
	   {
		 NoHover();
		 h1 = 1;
	   }
	   
	   if(m_y <= 63 && m_y >= 53)//button2
	   {
		 NoHover();
		 h2 = 1;
	   }
	   
	   if(m_y <= 46 && m_y >= 35)//button3
	   {
		 NoHover();
		 h3 = 1;
	   }
	   
	   if(m_y <= 27 && m_y >= 17)//button4
	   {
		 NoHover();
		 h4 = 1;
	   }
     }
     else NoHover();
}

void MainMenu::NoHover()
{
     h1 = 0;
     h2 = 0;
     h3 = 0;
     h4 = 0;
     h5 = 0;
     h6 = 0;
     h7 = 0;
}


void MainMenu::DrawSave()
{
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     glLoadIdentity();
     
     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();                
     glOrtho(0,100,0,100,-21,21);   
     glMatrixMode(GL_MODELVIEW);  
     
    
     glColor3f(1,1,1);
     
     c.menu_save_bg.Bind();
     
     glBegin(GL_QUADS);
	   glNormal3f(0,0,1);
	   glTexCoord2f(0,0);
	   glVertex3i(0,0,-20);
	   glTexCoord2f(0,1);
	   glVertex3i(0,100,-20);
	   glTexCoord2f(1,1);
	   glVertex3i(100,100,-20);
	   glTexCoord2f(1,0);
	   glVertex3i(100,0,-20);
     glEnd();
     
     
     glColor3f(1,1,1);
	   
     glBlendFunc(GL_SRC_COLOR,GL_ONE_MINUS_SRC_COLOR) ;
     glEnable(GL_BLEND);
     
     
     if(h1)
	   glColor3f(0,1,0);
     else glColor3f(1,1,1);
     c.load_font.print(6,72,c.saveNames[0].name);
     
     if(h2)
	   glColor3f(0,1,0);
     else glColor3f(1,1,1);     
     c.load_font.print(6,52,c.saveNames[1].name);
     
     if(h3)
	   glColor3f(0,1,0);
     else glColor3f(1,1,1);
     c.load_font.print(6,33,c.saveNames[2].name);
     
     
     if(h4)
	   glColor3f(0,1,0);
     else glColor3f(1,1,1);
     c.load_font.print(56,72,c.saveNames[3].name);
     
     if(h5)
	   glColor3f(0,1,0);
     else glColor3f(1,1,1);     
     c.load_font.print(56,52,c.saveNames[4].name);
     
     if(h6)
	   glColor3f(0,1,0);
     else glColor3f(1,1,1);
     c.load_font.print(56,33,c.saveNames[5].name);
     
     if(h7)
	   glColor3f(0,1,0);
     else glColor3f(1,1,1);
     c.load_font.print(6,10,"Exit");
     
     if(loadD)
     {
	   DrawLoad();
	   return;
     }
     
     c.load_font.print(15,85,"Select slot to save ");
     
     glDisable(GL_BLEND);
	   
     glFlush();      
     
     glutSwapBuffers();   
     
}

void MainMenu::DrawLoad()
{
     c.load_font.print(15,85,"Select slot to Load ");
     
     glDisable(GL_BLEND);
	   
     glFlush();      
     
     glutSwapBuffers();   
     
}

void MainMenu::SaveMouseFunction(int button, int state , int x, int y)
{
     if(loadD)
     {
	   LoadMouseFunction(button,state ,  x,  y);
	   return;
     }
     
     extern int resX ,resY;

     float mx = 100 * ((float)x / (float)resX);
     float my = 100 - 100 * ((float)y / (float)resY);
     
     time_t t = time(0);
     struct tm* lt = localtime(&t);
     char time_str[25];
     sprintf(time_str, "%02d_%02d-%02d_%02d:%02d",c.curMap,  lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min );
     
     if(mx >= 6 && mx <= 43)
     {
	   if(my >= 71 && my <= 84)
	   {
		 c.Save("Saves/save0.sav");
		 
		 strcpy(c.saveNames[0].name , time_str);
		 std::ofstream f("Saves/gamelist.dat");
		 for(int a = 0; a< 6; a++)
		      f <<  c.saveNames[a].name << "\n";
		 f.close();
		 
		 saveD = 0;
	   }
	   
	   if(my >= 51 && my <= 64)
	   {
		 c.Save("Saves/save1.sav");
		 
		 strcpy(c.saveNames[1].name , time_str);
		 std::ofstream f("Saves/gamelist.dat");
		 for(int a = 0; a< 6; a++)
		      f <<  c.saveNames[a].name << "\n";
		 f.close();
		 
		 saveD = 0;
	   }
	   
	   if(my >= 31 && my <= 44)
	   {
		 c.Save("Saves/save2.sav");
		 strcpy(c.saveNames[2].name , time_str);
		 std::ofstream f("Saves/gamelist.dat");
		 for(int a = 0; a< 6; a++)
		      f <<  c.saveNames[a].name << "\n";
		 f.close();
		 saveD = 0;
	   }
	   
	   if(my >= 10 && my <= 23)//exit button
	   {
		 saveD = 0;
	   }
     }
     else if(mx >= 54 && mx <= 93)
     {
	   if(my >= 71 && my <= 84)
	   {
		 c.Save("Saves/save3.sav");
		 strcpy(c.saveNames[3].name , time_str);
		 std::ofstream f("Saves/gamelist.dat");
		 for(int a = 0; a< 6; a++)
		      f <<  c.saveNames[a].name << "\n";
		 f.close();
		 saveD = 0;
	   }
	   
	   if(my >= 51 && my <= 64)
	   {
		 c.Save("Saves/save4.sav");
		 strcpy(c.saveNames[4].name , time_str);
		 std::ofstream f("Saves/gamelist.dat");
		 for(int a = 0; a< 6; a++)
		      f <<  c.saveNames[a].name << "\n";
		 f.close();
		 saveD = 0;
	   }
	   
	   if(my >= 31 && my <= 44)
	   {
		 c.Save("Saves/save5.sav");
		 strcpy(c.saveNames[5].name , time_str);
		 std::ofstream f("Saves/gamelist.dat");
		 for(int a = 0; a< 6; a++)
		      f <<  c.saveNames[a].name << "\n";
		 f.close();
		 saveD = 0;
	   }
     }
     
     
     
}

void MainMenu::LoadMouseFunction(int button, int state , int x, int y)
{
     
     //c.LoadSave("Saves/dump.tmp");
     extern int resX ,resY;

     float mx = 100 * ((float)x / (float)resX);
     float my = 100 - 100 * ((float)y / (float)resY);
     
     if(mx >= 6 && mx <= 43)
     {
	   if(my >= 71 && my <= 84)
	   {
		 c.LoadSave("Saves/save0.sav");
		 loadD = 0;
		 show = 0;
		 inGame = 1;
	   }
	   
	   if(my >= 51 && my <= 64)
	   {
		 c.LoadSave("Saves/save1.sav");
		 loadD = 0;
		 show = 0;
		 inGame = 1;
	   }
	   
	   if(my >= 31 && my <= 44)
	   {
		 c.LoadSave("Saves/save2.sav");
		 loadD = 0;
		 show = 0;
		 inGame = 1;
	   }
	   
	   if(my >= 10 && my <= 23)//exit button
	   {
		 loadD = 0;
	   }
     }
     else if(mx >= 54 && mx <= 93)
     {
	   if(my >= 71 && my <= 84)
	   {
		 c.LoadSave("Saves/save3.sav");
		 loadD = 0;
		 show = 0;
		 inGame = 1;
	   }
	   
	   if(my >= 51 && my <= 64)
	   {
		 c.LoadSave("Saves/save4.sav");
		 loadD = 0;
		 show = 0;
		 inGame = 1;
	   }
	   
	   if(my >= 31 && my <= 44)
	   {
		 c.LoadSave("Saves/save5.sav");
		 loadD = 0;
		 show = 0;
		 inGame = 1;
	   }
     }
}

void MainMenu::MousePassiveMotionSave(int x, int y)
{
     extern int resX ,resY;

     float mx = 100 * ((float)x / (float)resX);
     float my = 100 - 100 * ((float)y / (float)resY);
     
     if(mx >= 6 && mx <= 43)
     {
	   if(my >= 71 && my <= 84)
	   {
		 NoHover();
		 h1 = 1;
	   }
	   
	   if(my >= 51 && my <= 64)
	   {
		 NoHover();
		 h2 = 1;
	   }
	   
	   if(my >= 31 && my <= 44)
	   {
		 NoHover();
		 h3 = 1;
	   }
	   
	   if(my >= 10 && my <= 23)//exit button
	   {
		 NoHover();
		 h7 = 1;
	   }
     }
     else if(mx >= 54 && mx <= 93)
     {
	   if(my >= 71 && my <= 84)
	   {
		 NoHover();
		 h4 = 1;
	   }
	   
	   if(my >= 51 && my <= 64)
	   {
		 NoHover();
		 h5 = 1;
	   }
	   
	   if(my >= 31 && my <= 44)
	   {
		 NoHover();
		 h6 = 1;
	   }
     }
         
}

