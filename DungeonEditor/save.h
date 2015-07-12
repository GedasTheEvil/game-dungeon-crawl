#ifndef SaveH
#define SaveH

#include <stdio.h>
#include <fstream>
#include "vars.h"

char tmp[255];

void Save();
void Load();


void Save()
{
     sprintf(tmp,"Saved/%s",AtText3);
     std::ofstream f(tmp);
     printf("Saving file %s \n",AtText3);
     f << 1881 << std::endl;
     for(int ii = 0; ii < 1881; ii++)
          f << map[ii].type << " " << map[ii].Atr << " " << map[ii].AtrV << " \n";
     f.close();
}

void Load()
{
     sprintf(tmp,"Saved/%s",AtText3);
     std::ifstream f(tmp);
     int header;
     printf("Loading file %s \n",AtText3);
     f >> header; 
     if (header != 1881) {printf("Wrong header\n");return;}
     
     char a, b, c[5];     
     
     for(int jj = 0; jj < 1881; jj++)
     {
          f >> map[jj].type; 

          f >> map[jj].Atr; 

          f >> map[jj].AtrV;
     }
     f.close();
}

#endif

