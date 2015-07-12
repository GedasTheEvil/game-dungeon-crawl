#include "monster.h"
#include <cmath>
#include <stdio.h>
// // #include "stats.h"
#include "cashe.h"

extern Cashe c;


int monster::AtDir()
{
     if(!speed)
     {
	   if( *DX - X -0.5 > 0.2+ 0.02*scale && fabs(Y -*DY) < 0.8)
		 return 1;
	   else if ( *DX - X -0.5 < -0.2 -0.02*scale && fabs(Y -*DY) < 0.8)
		 return -1;
	   else return 0;
     }
     
     if((x+X+0.5) - *DX > 0.05 + 0.02*scale && fabs(Y -*DY) < 0.8)
	   return -1;
     else if ((x+X+0.5) - *DX < -0.05 - 0.02*scale && fabs(Y -*DY) < 0.8)
	   return 1;
     return 0;     
}

int monster::Seek()
{
     if(Alive())
     {
// 	   if(walk_timer -> TimePassed() )
		 x += 0.0042*(AtDir()*speed);
	   
	   if(!AtDir())
		 return 0;
	   else mdl = walk;
	   
	   return 1;
     }
}

void monster::Attack()
{
     if(!Alive())
	   return;
     
     mdl = attack;
     
     if(/*Att_timer -> TimePassed() &&*/ fabs(Y -*DY) < 0.8)
     {
	   c.Stats -> GetHit(damage);
	   att_s.Play();
     }
}

void monster::GetCords(float &xx, float &yy)
{
     xx = x;
     yy = y;
}

bool monster::Nearby(float xx, float yy, int rangei)
{
    
     float range = 0.1 * (float)rangei;
     
     if(fabs(X+x+0.5 - xx) <=  range && fabs(Y -yy) < 0.7)
	   return 1;
	   
     return 0;     
}

void monster::changeMDL(int id)
{
     if(id == 0)
     {
	   mdl = walk;
	   return;
     }
     
     if(id == 1)
     {
	   mdl = attack;
	   return;
     }
     
     if(id == 2)
     {
	   mdl = die;
	   return;
     }
}

int monster::Model_state()
{
  if(mdl == walk)
    return 1;
  if(mdl == attack)
    return 2;
  return 0;
}

void monster::setModel(int state)
{
  if(state == 1)
    mdl = walk;
  else if(state = 2)
    mdl = attack;
  else mdl = die;
}
