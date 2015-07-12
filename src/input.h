#ifndef DInputH
#define DInputH

void Load();

void Draw();

void Idle();

void keyPressed(unsigned char a, int x, int y);

void specialKeyPressed(int a, int x , int y);

void processMouse(int a, int b , int c, int d);

void processMouseActiveMotion(int a, int b);

void processMousePassiveMotion(int a, int b);

void processMouseEntry(int a);

#endif
