#ifndef PAD_STICK_H
#define PAD_STICK_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <utility>

// A gamepad's sticks (plan sdl3-gamepad), GL- and SDL-free. Axes run -1..1, y down (SDL's). The left stick is four
// held directions, the walk and climb keys or a menu's arrows (PadStick); the right stick turns the look
// (padLookTurn). Inside the dead zone a stick does nothing.
constexpr float PAD_LOOK_SPEED = 90.f; // degrees a second, the right stick fully over at 100 %
// A direction let go a little inside where it was pressed, so a stick held at the edge does not flicker.
constexpr float PAD_STICK_HYSTERESIS = 0.05f;

class PadStick {
  public:
	enum class Dir : std::uint8_t { Left, Right, Up, Down };
	static constexpr int DIRS = 4;

	// The held directions after the stick moved to (x, y); deadZone 0..1.
	void Move(float x, float y, float deadZone) {
		update(Dir::Left, -x, deadZone);
		update(Dir::Right, x, deadZone);
		update(Dir::Up, -y, deadZone);
		update(Dir::Down, y, deadZone);
	}
	[[nodiscard]] bool Held(Dir dir) const { return held[std::to_underlying(dir)]; }
	void Release() { held.fill(false); }

  private:
	std::array<bool, DIRS> held{};

	void update(Dir dir, float along, float deadZone) {
		bool& on = held[std::to_underlying(dir)];
		on = on ? along > deadZone - PAD_STICK_HYSTERESIS : along > deadZone;
	}
};

// How far past the dead zone an axis is, 0..1 with its sign, squared for a fine turn near the centre.
[[nodiscard]] inline float padStickReach(float axis, float deadZone) {
	const float past = (std::abs(axis) - deadZone) / std::max(1.f - deadZone, 0.01f);
	if (past <= 0.f)
		return 0.f;
	const float reach = std::min(past, 1.f);
	return std::copysign(reach * reach, axis);
}

struct PadLookTurn {
	float m = 0.f; // degrees: left / right, as CameraLook::Angles
	float n = 0.f; // up / down
};

// The right stick at (x, y) over dtMs: the look's turn. speedPercent of PAD_LOOK_SPEED; the stick up looks up (y is
// down), invertY the other way.
[[nodiscard]] inline PadLookTurn
padLookTurn(float x, float y, float deadZone, int speedPercent, bool invertY, int dtMs) {
	const float degrees = PAD_LOOK_SPEED * static_cast<float>(speedPercent) / 100.f * static_cast<float>(dtMs) / 1000.f;
	const float up = -padStickReach(y, deadZone);
	return {padStickReach(x, deadZone) * degrees, (invertY ? -up : up) * degrees};
}

#endif
