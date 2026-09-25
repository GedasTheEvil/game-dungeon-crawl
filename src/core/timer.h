#ifndef TimerH
#define TimerH

constexpr int CDefTime = 100;

// Time source for all game timers. Real SDL ticks by default; scenario tests switch
// to a virtual clock that only moves when advanced, so runs are deterministic.
namespace GameClock {
void enableVirtual();
void advance(int ms);
int now();
} // namespace GameClock

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
