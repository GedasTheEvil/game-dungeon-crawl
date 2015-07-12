#include "item.h"
#include <cmath>
#include <stdio.h>
#include "cashe.h"

extern Cashe c;

void item::Draw()
{
     if(!loaded)
	   return;
     
     glPushMatrix();
     
     if(!in_inventory && fabs(x) >= 0.1)
	   glTranslatef(40*x - 20,y,-30);
     else glTranslatef(0,0,-30);
     
     glPushMatrix();// will add rotation
	    
     glScalef(scale,scale,scale);

     tex . Bind();
     glRotatef(rotA,0,1,0);
	
     if(c.Cartoon)
	   mdl -> ShowC();
     else mdl -> Show();
     glPopMatrix();
     glPopMatrix();
     mdl -> Advance_Animation();
}

bool item::getPickedUp()
{
     if(!loaded)
	   return 0;
     
     in_inventory = 1;
     
     return 1;
}

item::item()
{
     mdl = NULL;
     x = 0;
     y = 0;
     scale = 0;
     rotA = 0;
     loaded = 0;
     heal = 1;
     damage =1;
     range = 1;
     type = 0;
}
item::~item()
{
     if(mdl)
	   delete mdl;
     mdl = NULL;
     loaded = 0;     
     printf("Deleting item %x \n");
}

bool item::LoadMDL(const char filename[],Textura &texture, bool compile)
{
     tex = texture;
     mdl = new CartoonANI();
     mdl -> Load(filename);
     mdl -> Centrify();
     mdl -> BindTexture(tex.ID());
     if(compile)
	   mdl -> Compile();
     
     loaded = 1;
}
