//x + 7, y -7
#include "save.h"

void processMouse(int button, int state, int x, int y);
void processMouseActiveMotion(int x, int y);
void processMousePassiveMotion(int x, int y);
void processMouseEntry(int state);
void GetEl(int x, int y);
int ScreenToDesignCore(int sx, int sy, int vpX, int vpY, int vpW, int vpH, float scale, int *dx, int *dy);
int ScreenToDesign(int sx, int sy, int *dx, int *dy);

// Pure core of the screen-to-design transform: takes the viewport rect and
// scale explicitly (instead of reading globals) so it can be exercised by a
// standalone offline test. See ScreenToDesign below for the real entry point.
int ScreenToDesignCore(int sx, int sy, int vpX, int vpY, int vpW, int vpH, float scale, int *dx, int *dy)
{
     if (sx < vpX || sx >= vpX + vpW ||
         sy < vpY || sy >= vpY + vpH)
          return 0;

     *dx = (int)((sx - vpX) / scale);
     *dy = (int)((sy - vpY) / scale);
     return 1;
}

// Converts a raw GLUT window-pixel mouse coordinate (top-left origin) into
// the fixed DESIGN_WIDTH x DESIGN_HEIGHT coordinate space that all existing
// hit-testing in this file assumes. Returns 0 (and leaves *dx/*dy untouched)
// if the click falls outside the current letterboxed content area — callers
// must check the return value before using *dx/*dy.
int ScreenToDesign(int sx, int sy, int *dx, int *dy)
{
     // glViewport(ViewportX, ViewportY, ...) measures ViewportY from the
     // window's BOTTOM edge (OpenGL window-coordinate convention), but GLUT
     // mouse coordinates (sx, sy) are measured from the window's TOP edge.
     // ViewportX needs no such conversion (both GLUT and OpenGL measure x
     // from the left), but reusing ViewportY as-is for the y margin is only
     // correct when the top and bottom letterbox bars are the same size,
     // which integer rounding in ComputeContainViewport does not guarantee
     // (WinHeight - ViewportH can be odd). Recompute the true top-down top
     // margin here so the bounds check/subtraction line up with where the
     // content actually renders on screen.
     int topMarginY = WinHeight - ViewportY - ViewportH;
     return ScreenToDesignCore(sx, sy, ViewportX, topMarginY, ViewportW, ViewportH, ViewportScale, dx, dy);
}

void processMouse(int button, int state, int x, int y)
{
     int dx, dy;
     if (!ScreenToDesign(x, y, &dx, &dy))
          return;
     x = dx;
     y = dy;

     if(state == GLUT_UP)
     {
	   printf("x:%d , y:%d, btn %d\n",x,y,button);
// 	   mouseX=x;
// 	   mouseY=y;  
if(x > 430)
{
	   if(x > 450 && x < 473 && y < 80 && y > 54)
		 selectedB = Wall;
	   if(x > 491 && x < 515 && y < 80 && y > 54)
		 selectedB = Empty;
	   if(x > 530 && x < 554 && y < 80 && y > 54)
		 selectedB = Door; 
	   if(x > 570 && x < 594 && y < 80 && y > 54)
		 selectedB = Death; 
          if(x > 610 && x < 634 && y < 80 && y > 54)
               selectedB = Ankh; 
          
	   
	   if(x > 450 && x < 473 && y < 120 && y > 94)
		 selectedB = Monster;
          if(x > 491 && x < 515 && y < 120 && y > 94)
               selectedB = Spike;
          if(x > 530 && x < 554 && y < 120 && y > 94)
               selectedB = Ladder;
          if(x > 570 && x < 594 && y < 120 && y > 94)
               selectedB = Area3D;
          if(x > 610 && x < 634 && y < 120 && y > 94)
               selectedB = Treasure;
          
          
          
	   if(x > 490 && x < 590 && y < 440 && y > 420)
		 Save(); 
          if(x > 490 && x < 590 && y < 470 && y > 450)
               Load(); 
          
	   
	   if(x > 443 && x < 534 && y > 147 && y < 161) //Atribute
	   {
		 selA1 = !selA1;
		 selA1V = 0;
		 selMN =0;
	   }
	   
	   if(x > 443 && x < 534 && y > 167 && y < 181) // Value
	   {
		 selA1V = !selA1V;
		 selA1 = 0;
		 selMN =0;
	   }
	   
	   if(x > 443 && x < 534 && y > 207 && y < 221) // Map name
	   {
// 		 printf("map was name%d\n",selMN);
		 selMN = !selMN;
		 selA1 = 0;
		 selA1V = 0;
// 		 printf("map now name%d\n",selMN);
	   }
}
	   if(x < 420)GetEl(x,y);
	   
// 	   printf("Selected:%d\n",selectedB);
     }
}

void processMouseActiveMotion(int x, int y)
{
     int dx, dy;
     if (!ScreenToDesign(x, y, &dx, &dy))
          return;
     GetEl(dx, dy);
}

void processMousePassiveMotion(int x, int y) 
{

}

void processMouseEntry(int state) 
{

}

void GetEl(int x, int y)
{
     int i =0, j =0;
     int k = 10,l=470;
     
     
     for(j = 0; j< 47; j++)
     {
	   for(i = 0; i < 40; i++)
	   {
		 if( (x >= k) && (x < 10 + k) && (y >= l -10) && (y < l))
		 {
// 		      printf("yay, veikia X:%d Y:%d\n",x,y);
		      
		      map[40*j + i].type = selectedB;
		      map[40*j + i].Atr  = Atr1;
		      map[40*j + i].AtrV = Atr2;
// 		      printf("X%d,y%d,l< %d  > %d ,k<%d  %d>\n",x,y,l -10,l,k,k+10);
		 }
// 		 else if(l > 300) printf("(x[%d] > k[%d]) && (x[%d] < 10 + k[%d]) && (y[%d] >= l -10[%d]) && (y[%d] < l[%d])\n", x,k,x,k+10,y,l-10,y,l);
		 
		 k+=10; 
	   }
	   
	   l = l - 10;
	   k =  10;
	   
     }
}
