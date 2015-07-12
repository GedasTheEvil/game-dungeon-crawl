#ifndef MenuH
#define MenuH

#include "timer.h"

/// @file menu.h
/// Set of functions for showing the menu

class MainMenu
{
private:
     bool h1,
	   h2,
	   h3,
	   h4,
	   h5,
	   h6,
	   h7;
     void NoHover();
     bool credits;
     timer *t_cred;
     
public:
     MainMenu();
     ~MainMenu();
     bool inGame;
     bool saveD;
     bool loadD;
     bool show;
     void Draw();
     void DrawSave();
     void DrawLoad();
     void InGameDraw();     
     void MouseFunction(int button, int state , int x, int y);
     void SaveMouseFunction(int button, int state , int x, int y);
     void LoadMouseFunction(int button, int state , int x, int y);
     void InGameMouseFunction(int button, int state , int x, int y);
     void MousePassiveMotion(int a, int b);
     void MousePassiveMotionSave(int a, int b);
};

#endif
