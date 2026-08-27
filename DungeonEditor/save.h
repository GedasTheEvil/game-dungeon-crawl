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
     StatusColor = StatusBusy;
     sprintf(StatusMsg,"Saving %s...",AtText3);

     std::ofstream f(tmp);
     if(!f.is_open())
     {
          printf("Failed to save file %s \n",AtText3);
          StatusColor = StatusError;
          sprintf(StatusMsg,"Save failed: %s",AtText3);
          return;
     }

     printf("Saving file %s \n",AtText3);
     f << 1881 << std::endl;
     for(int ii = 0; ii < 1881; ii++)
          f << map[ii].type << " " << map[ii].Atr << " " << map[ii].AtrV << " \n";
     f.close();

     StatusColor = StatusOk;
     sprintf(StatusMsg,"Saved: %s",AtText3);
}

void Load()
{
     sprintf(tmp,"Saved/%s",AtText3);
     StatusColor = StatusBusy;
     sprintf(StatusMsg,"Loading %s...",AtText3);

     std::ifstream f(tmp);
     if(!f.is_open())
     {
          printf("File not found: %s \n",AtText3);
          StatusColor = StatusError;
          sprintf(StatusMsg,"Not found: %s",AtText3);
          return;
     }

     int header;
     printf("Loading file %s \n",AtText3);
     f >> header;
     if (header != 1881)
     {
          printf("Wrong header\n");
          StatusColor = StatusError;
          sprintf(StatusMsg,"Invalid file: %s",AtText3);
          return;
     }

     for(int jj = 0; jj < 1881; jj++)
     {
          f >> map[jj].type;

          f >> map[jj].Atr;

          f >> map[jj].AtrV;
     }
     f.close();

     StatusColor = StatusOk;
     sprintf(StatusMsg,"Loaded: %s",AtText3);
}

#endif

