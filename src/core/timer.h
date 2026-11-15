#ifndef TIMER_H
#define TIMER_H
constexpr int DEFAULT_TIMER_MS = 100;

// Time source for all game timers. Real time (ms since startup) by default; scenario tests switch
// to a virtual clock that only moves when advanced, so runs are deterministic.
namespace GameClock {
void enableVirtual();
void resetVirtual(int ms); // virtual mode on, the time set to ms (the unit tests: every case starts the same)
void advance(int ms);
int now();
} // namespace GameClock

class Timer {
  private:
	int time_start;
	int ticks;

  public:
	Timer();
	Timer(int defT);
	~Timer();
	[[nodiscard]] bool TimePassed();
	[[nodiscard]] bool TimePassed(bool noRepeat);
	void Reset();
	void SetInterval(int ms) { ticks = ms; }
};

#endif
