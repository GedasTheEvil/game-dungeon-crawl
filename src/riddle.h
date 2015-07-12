#ifndef RiddleH
#define RiddleH

struct riddle
{
     const char *question_l1;
     const char *question_l2;
     const char *question_l3;
     const char *question_l4;
     const char *answer;
};


class Riddle
{
     private:
	   int RiddleCount;
	   riddle *rid;
	   int selected;
	   int ans_l;
     public:
	   bool show;
	   char YourAnswer[25];
	   int GateX, Gatey; 
	   Riddle();
	   ~Riddle();
	   void GetRiddle();
	   void Draw();
	   void KeyboardF(unsigned char key, int x, int y);
	   bool CheckAnswer();
};

#endif