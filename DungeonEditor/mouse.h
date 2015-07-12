//x + 7, y -7
#include "save.h"

void processMouse(int button, int state, int x, int y);
void processMouseActiveMotion(int x, int y);
void processMousePassiveMotion(int x, int y);
void processMouseEntry(int state);
void GetEl(int x, int y);

void processMouse(int button, int state, int x, int y) 
{
     
     
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
     GetEl(x,y);
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
