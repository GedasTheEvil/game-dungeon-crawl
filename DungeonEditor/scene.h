void Idle();
void Draw();
void DrawMap();


void Idle()
{
     Draw();
}

void Draw()
{
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer
     glLoadIdentity();
     
     glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
     glEnable(GL_BLEND);
     glMatrixMode(GL_PROJECTION);					// Select The Projection Matrix
     glPushMatrix();							// Store The Projection Matrix
     glLoadIdentity();						// Reset The Projection Matrix
     glOrtho(0,640,0,480,-1,1);					// Set Up An Ortho Screen
     glMatrixMode(GL_MODELVIEW);					// Select The Modelview Matrix
     
     glColor3f(0,0.2,0);
     glBindTexture(GL_TEXTURE_2D,nullt.texID);
//      glEnable(GL_LIGHTING);
     glBegin(GL_QUADS);// bg		
     {
	   glTexCoord2i(0, 1); 	  glVertex2i(10,10);		// Texture / Vertex Coord (Bottom Left)
	   glTexCoord2i(1, 1); 	  glVertex2i(400,10);	// Texutre / Vertex Coord (Bottom Right)
	   glTexCoord2i(1, 0);	  glVertex2i(400,490);	// Texture / Vertex Coord (Top Right)
	   glTexCoord2i(0, 0);	  glVertex2i(10,490);	// Texture / Vertex Coord (Top Left)
     }
     glEnd();
     
     glPushMatrix();
	   glColor3f(1,1,1);
	   glTranslatef(450,400,0.01); 
	   
	   glBindTexture(GL_TEXTURE_2D,Mat[Wall].texID);
	   glBegin(GL_QUADS);		
	   {
		 glTexCoord2i(0, 1);   glVertex2i(0,0  );
		 glTexCoord2i(1, 1);   glVertex2i(25,0 );
		 glTexCoord2i(1, 0);   glVertex2i(25,25);
		 glTexCoord2i(0, 0);   glVertex2i(0,25 );
	   }
	   glEnd();
	   
	   glTranslatef(40,0,0); 
	   glBindTexture(GL_TEXTURE_2D,Mat[Empty].texID);
	   glBegin(GL_QUADS);		
	   {
		 glTexCoord2i(0, 1);   glVertex2i(0,0  );
		 glTexCoord2i(1, 1);   glVertex2i(25,0 );
		 glTexCoord2i(1, 0);   glVertex2i(25,25);
		 glTexCoord2i(0, 0);   glVertex2i(0,25 );
	   }
	   glEnd();
	   
	   
	   glTranslatef(40,0,0); 
	   glBindTexture(GL_TEXTURE_2D,Mat[Door].texID);
	   glBegin(GL_QUADS);		
	   {
		 glTexCoord2i(0, 1);   glVertex2i(0,0  );
		 glTexCoord2i(1, 1);   glVertex2i(25,0 );
		 glTexCoord2i(1, 0);   glVertex2i(25,25);
		 glTexCoord2i(0, 0);   glVertex2i(0,25 );
	   }
	   glEnd();
	   
	   glTranslatef(40,0,0); 
	   glBindTexture(GL_TEXTURE_2D,Mat[Death].texID);
	   glBegin(GL_QUADS);		
	   {
		 glTexCoord2i(0, 1);   glVertex2i(0,0  );
		 glTexCoord2i(1, 1);   glVertex2i(25,0 );
		 glTexCoord2i(1, 0);   glVertex2i(25,25);
		 glTexCoord2i(0, 0);   glVertex2i(0,25 );
	   }
	   glEnd();
	   
          glTranslatef(40,0,0); 
          glBindTexture(GL_TEXTURE_2D,Mat[Ankh].texID);
          glBegin(GL_QUADS);              
          {
               glTexCoord2i(0, 1);   glVertex2i(0,0  );
               glTexCoord2i(1, 1);   glVertex2i(25,0 );
               glTexCoord2i(1, 0);   glVertex2i(25,25);
               glTexCoord2i(0, 0);   glVertex2i(0,25 );
          }
          glEnd();
          
	   glPushMatrix();
	   
	   glTranslatef(-160,-40,0); 
	   glBindTexture(GL_TEXTURE_2D,Mat[Monster].texID);
	   glBegin(GL_QUADS);		
	   {
		 glTexCoord2i(0, 1);   glVertex2i(0,0  );
		 glTexCoord2i(1, 1);   glVertex2i(25,0 );
		 glTexCoord2i(1, 0);   glVertex2i(25,25);
		 glTexCoord2i(0, 0);   glVertex2i(0,25 );
	   }
	   glEnd();
	   
          glTranslatef(40,0,0); 
          glBindTexture(GL_TEXTURE_2D,Mat[Spike].texID);
          glBegin(GL_QUADS);              
          {
               glTexCoord2i(0, 1);   glVertex2i(0,0  );
               glTexCoord2i(1, 1);   glVertex2i(25,0 );
               glTexCoord2i(1, 0);   glVertex2i(25,25);
               glTexCoord2i(0, 0);   glVertex2i(0,25 );
          }
          glEnd();
          
                 glTranslatef(40,0,0); 
          glBindTexture(GL_TEXTURE_2D,Mat[Ladder].texID);
          glBegin(GL_QUADS);              
          {
               glTexCoord2i(0, 1);   glVertex2i(0,0  );
               glTexCoord2i(1, 1);   glVertex2i(25,0 );
               glTexCoord2i(1, 0);   glVertex2i(25,25);
               glTexCoord2i(0, 0);   glVertex2i(0,25 );
          }
          glEnd();
          
                 glTranslatef(40,0,0); 
          glBindTexture(GL_TEXTURE_2D,Mat[Area3D].texID);
          glBegin(GL_QUADS);              
          {
               glTexCoord2i(0, 1);   glVertex2i(0,0  );
               glTexCoord2i(1, 1);   glVertex2i(25,0 );
               glTexCoord2i(1, 0);   glVertex2i(25,25);
               glTexCoord2i(0, 0);   glVertex2i(0,25 );
          }
          glEnd();
          
                 glTranslatef(40,0,0); 
          glBindTexture(GL_TEXTURE_2D,Mat[Treasure].texID);
          glBegin(GL_QUADS);              
          {
               glTexCoord2i(0, 1);   glVertex2i(0,0  );
               glTexCoord2i(1, 1);   glVertex2i(25,0 );
               glTexCoord2i(1, 0);   glVertex2i(25,25);
               glTexCoord2i(0, 0);   glVertex2i(0,25 );
          }
          glEnd();
          
         
	   glPopMatrix();
	   
	   
	   
	   glTranslatef(-120,40,0); 
	   
	   if(selectedB != Empty)
		 glBindTexture(GL_TEXTURE_2D,Mat[selectedB].texID);
	   else glBindTexture(GL_TEXTURE_2D,nullt.texID);
	   glBegin(GL_QUADS);		
	   {
		 glTexCoord2i(0, 1);   glVertex2i(0,0  );
		 glTexCoord2i(1, 1);   glVertex2i(25,0 );
		 glTexCoord2i(1, 0);   glVertex2i(25,25);
		 glTexCoord2i(0, 0);   glVertex2i(0,25 );
	   }
	   glEnd();
	   

	   
	   
	   glTranslatef(0,-400,0);
	   
	   glBindTexture(GL_TEXTURE_2D,Button[0].texID); //save
	   
	   glBegin(GL_QUADS);		
	   {
		 glTexCoord2i(0, 1);   glVertex2i(0,0  );
		 glTexCoord2i(1, 1);   glVertex2i(100,0 );
		 glTexCoord2i(1, 0);   glVertex2i(100,20);
		 glTexCoord2i(0, 0);   glVertex2i(0,20 );
	   }
	   glEnd();
          
          glTranslatef(0,-30,0);
          
          glBindTexture(GL_TEXTURE_2D,Button[1].texID); //load
          
          glBegin(GL_QUADS);              
          {
               glTexCoord2i(0, 1);   glVertex2i(0,0  );
               glTexCoord2i(1, 1);   glVertex2i(100,0 );
               glTexCoord2i(1, 0);   glVertex2i(100,20);
               glTexCoord2i(0, 0);   glVertex2i(0,20 );
          }
          glEnd();
	   
     glPopMatrix();
     
     DrawMap();

     glColor3f(1,1,1);
     glPrint(270,450,"Editor");
     
     glPrint(0,0,"(0,0)");     
     glPrint(570,460,"(640,480)");     
     glPrint(0,460,"(0,480)");     
     glPrint(570,0,"(640,0)");
     if(selA1)glColor3f(0,1,0);
     else glColor3f(1,1,1);
     glPrint(440,320,"Atribute: %s", AtText1);
     if(selA1V)glColor3f(0,1,0);
     else glColor3f(1,1,1);
     glPrint(440,300,"Value: %s", AtText2);
     
     if(selMN)glColor3f(0,1,0);
     else glColor3f(1,1,1);
     glPrint(440,260,"DungeonName: %s", AtText3);
     
     glColor3f(1,1,1);
     glMatrixMode(GL_PROJECTION);				// Select The Projection Matrix
     glPopMatrix();						// Restore The Old Projection Matrix
     glMatrixMode(GL_MODELVIEW);				// Select The Modelview Matrix
     
     glFlush();	
     glDisable(GL_BLEND);
     glBlendFunc(GL_DST_COLOR, GL_ZERO);
     
     glutSwapBuffers();
}

void DrawMap()
{
     glTranslatef(10,10,0.01);
     
     int i =0, j =0;
     
     for(j = 0; j< 47; j++)
     {
	   for(i = 0; i < 40; i++)
	   {
		 if(map[40*j + i].type != Empty)
		      glBindTexture(GL_TEXTURE_2D,Mat[map[40*j + i].type].texID);
		 else glBindTexture(GL_TEXTURE_2D,nullt.texID);
		 glBegin(GL_QUADS);		
		 {
		      glTexCoord2i(0, 1); 	  glVertex2i(0,0);
		      glTexCoord2i(1, 1); 	  glVertex2i(9,0);	
		      glTexCoord2i(1, 0);	  glVertex2i(9,9);	
		      glTexCoord2i(0, 0);	  glVertex2i(0,9);	
		 }
		 glEnd(); 
		 glTranslatef(10,0,0);
	   }
	   glTranslatef(-400,10,0);
	   
     }

}
