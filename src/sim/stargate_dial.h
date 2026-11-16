// The Stargate dialled (plan stargate-chamber, plan dungeon-subsystems): its chevrons lock one by one, the seventh
// will not, the inner track turns back and forth. The Dungeon plays the sounds and pays the reward
// (dungeon_stargate.cpp); DungeonView::drawStargate draws it.
#ifndef STARGATE_DIAL_H
#define STARGATE_DIAL_H
#include <vector>

class StargateDial {
  public:
	// What a dial step does, in order: each chevron but the seventh locks, the seventh fails, the dial fails (the
	// message, the reward the first time). After the last step the dial ends (Dialling false).
	enum class Step : unsigned char { ChevronLock, ChevronFail, Failed };
	void Start(int cell); // the Stargate in cell (map index); the first chevron's turn starts now
	// The steps due since the last call, in order; empty when not dialling.
	[[nodiscard]] std::vector<Step> Update();
	[[nodiscard]] bool Dialling() const { return cell >= 0; }
	[[nodiscard]] int Cell() const { return cell; } // -1 when still
	// The track's turn in degrees (counter-clockwise as seen) of the Stargate in cell at (map index).
	[[nodiscard]] float TrackDeg(int at) const;
	// How bright chevron k (0 to STARGATE_CHEVRONS - 1, the seventh at the top) of the Stargate in cell at glows, 0..1.
	[[nodiscard]] float ChevronGlow(int at, int chevron) const;

  private:
	int cell = -1; // the Stargate's cell while the chevrons lock, -1 when still
	int startMs = 0; // GameClock
	int step = 0; // the next step: a chevron's turn, the failing seventh, its message, the end
	float restDeg = 0.f; // the track's turn kept from the dials before
	[[nodiscard]] static float trackDeg(int elapsedMs); // the track's turn so far in a dial
};

#endif
