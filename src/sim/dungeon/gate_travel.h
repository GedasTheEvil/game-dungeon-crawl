// Gate travel (plan dungeon-subsystems): the view fades out (Out), the jump happens under the faded frame, the view
// fades in (In). A ring platform (plan ring-transporter) drops its rings first (RingsDown), fades in a white flash and
// raises them at the partner (RingsUp). The Dungeon makes the jump and stands the world still meanwhile
// (Dungeon::updateTravel).
#ifndef GATE_TRAVEL_H
#define GATE_TRAVEL_H

class GateTravel {
  public:
	enum class Phase : unsigned char { None, Out, In, RingsDown, RingsUp };
	void Start(bool exitGate); // the exit gate (red), else a teleporter (blue)
	void StartRings(); // a ring platform (white)
	// The phase that has run its length now, or None while it still runs (or none is).
	[[nodiscard]] Phase Ended() const;
	// The phase after `ended` starts now. Given, not read: the jump in between may have reset the travel (a new level).
	void Next(Phase ended);
	[[nodiscard]] bool Active() const { return phase != Phase::None; }
	[[nodiscard]] bool ToExit() const { return exit; }
	[[nodiscard]] bool ByRings() const { return rings; }
	[[nodiscard]] float Fade() const; // 0..1: how far the view is faded into the gate colour
	// Ring k's way down out of the ceiling, 0 up in its housing to 1 at rest round the player; 0 when not by rings.
	[[nodiscard]] float RingDrop(int ring) const;

  private:
	Phase phase = Phase::None;
	int startMs = 0;
	bool exit = false;
	bool rings = false;
	[[nodiscard]] int phaseMs() const; // how long the phase lasts
};

#endif
