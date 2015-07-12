#include "trap.h"
#include <stdio.h>
#include "cashe.h"
#include <cmath>

extern Cashe c;

trap::trap()
{
     Hurt_timer = new timer(100);
     mdl = new CartoonANI();
     x = 0;
     y = 0;
     scale = 3;
}

trap::~trap()
{
     delete mdl;
     delete Hurt_timer;
     printf("Deleting trap %x \n",this);
}

void trap::Show()
{
     glPushMatrix();
     glTranslatef(0,0,-30);
     glPushMatrix();
     glScalef(scale,scale,scale);
     tex.Bind();  
     if(c.Cartoon)
	   mdl -> ShowC();
     else mdl -> Show();
     glPopMatrix();
     glPopMatrix();
     Hurt();
     
}
void trap::Hurt()
{
     
     if(fabs(*DX - x - 0.5) <= 0.02 * scale && fabs(*DY - y) <= 0.006 * scale)
     {
	   c.Stats -> GetHit( 1 );
     }
}

void trap::setCords(float nX, float nY)
{
     x = nX;
     y = nY;
}

bool trap::LoadMDL(const char filename[],Textura &texture, bool compile)
{
     tex = texture;
     
     mdl -> Load(filename);
     mdl -> BindTexture(texture.ID());
     mdl -> Centrify();
     
     if(compile)
	   mdl -> Compile();
     
}

void trap::debugText()
{
     printf("I am trap %d, my x=%f, y=%f\n Dungeon x=%f, y=%f\n\n",this,x,y,*DX,*DY);
}