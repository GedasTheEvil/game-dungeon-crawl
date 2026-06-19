#ifndef TimerH
#define TimerH

constexpr int CDefTime = 100;

class timer {
  private:
	int time_start;
	int ticks;

  public:
	timer();
	timer(int defT);
	~timer();
	[[nodiscard]] bool TimePassed();
	[[nodiscard]] bool TimePassed(bool noRepeat);
	void Reset();
};

#endif
