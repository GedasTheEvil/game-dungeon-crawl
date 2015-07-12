
#define Enter 13
#define Backspace 8

void keyPressed(unsigned char key, int x, int y);
void specialKeyPressed(int key, int x, int y);
void ParseInt();
int  ipow(int A, int x);


int ipow(int A, int x)
{
     int i ;
     int snd = 1;
     for(i = 0; i < x; i++)
	   snd = snd * A;
     
     return snd;
}

void ParseInt()
{
     int j;
     
     if(!AtText1L)
	   Atr1  = 0;
     else
     {
	   Atr1  = 0;
	   for(j = 0; j < AtText1L;j++)
		 Atr1 += (AtText1[j]- '0')*ipow(10,AtText1L-j-1);
     }
     
     if(!AtText2L)
	   Atr2 = 0;
     else
     {
	   Atr2  = 0;
	   for(j = 0; j < AtText2L;j++)
		 Atr2 += (AtText2[j]- '0')*ipow(10,AtText2L-j-1); 
     }
     
     printf("Parsed T1:%d and T2:%d\n",Atr1,Atr2);
     
}


/* The function called whenever a normal key is pressed. */
void keyPressed(unsigned char key, int x, int y) 
{
     usleep(100);
     printf("Pressed %d\n",key);
     if(selA1)
     {
	   if(key == Enter)
	   {
		 AtText1[AtText1L] = 0;
		 selA1 = 0;
		 selA1V = 0;
		 ParseInt();
		 return;
	   }
	   
	   if(key == Backspace)
	   {
		 if(AtText1L-1 >=0)AtText1L--;
		 AtText1[AtText1L] = 0x00;
		 return;  
	   }
	   
	   if(AtText1L > 2)return;
	   if(key > 57 || key < 48)return;
	   
	   AtText1[AtText1L] = key;
	   AtText1L++;
	   
	   
     }
     
     
     if(selA1V)
     {
	   
	   if(key == Enter)
	   {
		 AtText2[AtText2L] = 0;
		 selA1V = 0;
		 selA1 = 0;
		 ParseInt();
		 return;
	   }
	   
	   if(key == Backspace)
	   {
		 if(AtText2L-1 >=0)AtText2L--;
		 AtText2[AtText2L] = 0x00;  
		 return; 
	   }
	   
	   if(AtText2L > 3)return;
	   if(key > 57 || key < 48)return;
	   	   
	   AtText2[AtText2L] = key;
	   AtText2L++;  
     }
     
     if(selMN)
     {
	   
	   if(key == Enter)
	   {
		 AtText3[AtText3L] = 0;
		 selMN = 0;
		 return;
	   }
	   
	   if(key == Backspace)
	   {
		 AtText3L--;
		 AtText3[AtText3L] = 0x00;  
		 return; 
	   }
	   
	   if(AtText3L > 5)return;
// 	   if(key > 57 || key < 48)return;
	   
	   AtText3[AtText3L] = key;
	   AtText3L++;  
     }
     
}

/* The function called whenever a key is pressed. */
void specialKeyPressed(int key, int x, int y) 
{
	/* avoid thrashing this procedure */
// 	usleep(100);
	
}

