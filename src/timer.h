#ifndef TimerH
#define TimerH

#define CDefTime 100

class timer
{
     private :
	   int time_start;
	   int ticks;
     
     public:
	   timer();
	   timer(int defT);
	   ~timer();
	   bool TimePassed();
	   bool TimePassed(bool noRepeat);
	   void Reset();
};

#endif
