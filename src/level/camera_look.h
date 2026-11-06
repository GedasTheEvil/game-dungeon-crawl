#ifndef CAMERA_LOOK_H
#define CAMERA_LOOK_H

#include "input.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

// The camera peek (plan controls-smoothing): rotM / rotN, the turn off straight ahead. A look key held turns it at a
// steady CAMERA_LOOK_KEY_SPEED, whatever the desktop's key repeat. With recentring on, once no look input (a look key
// held, a mouse motion that turned it) came for CAMERA_RECENTER_DELAY_MS, the peek eases back to 0.
constexpr float CAMERA_LOOK_KEY_SPEED = 60.f; // degrees a second
constexpr int CAMERA_RECENTER_DELAY_MS = 1500;
constexpr float CAMERA_RECENTER_TAU_MS = 160.f; // the easing's time constant: settled (5%) after about 0.5 s
constexpr float CAMERA_RECENTER_SNAP = 0.05f; // degrees: closer to 0 is 0

class CameraLook {
  public:
	enum class Dir : std::uint8_t { Left, Right, Up, Down };
	struct Angles {
		float m = 0.f; // left / right, CAMERA_ROTATE_LIMIT_X either way
		float n = 0.f; // up / down, CAMERA_ROTATE_LIMIT_Y
	};

	void Hold(Dir dir, bool down) { held[static_cast<size_t>(dir)] = down; }
	void Release() { held.fill(false); }
	// The mouse turned the camera: the easing stops and waits again.
	void Looked() { idleMs = 0; }

	// One tick of dtMs: the held keys turn the camera; else, with recenter, after the delay it eases back.
	Angles Step(Angles a, int dtMs, bool recenter) {
		if (std::any_of(held.begin(), held.end(), [](bool h) { return h; })) {
			idleMs = 0;
			const float step = CAMERA_LOOK_KEY_SPEED * static_cast<float>(dtMs) / 1000.f;
			a.m += step * (axis(Dir::Right) - axis(Dir::Left));
			a.n += step * (axis(Dir::Up) - axis(Dir::Down));
			return Clamp(a);
		}
		idleMs = std::min(idleMs + dtMs, CAMERA_RECENTER_DELAY_MS);
		if (!recenter || idleMs < CAMERA_RECENTER_DELAY_MS)
			return a;
		const float keep = std::exp(-static_cast<float>(dtMs) / CAMERA_RECENTER_TAU_MS);
		return {ease(a.m, keep), ease(a.n, keep)};
	}

	static Angles Clamp(Angles a) {
		return {
			std::clamp(a.m, -CAMERA_ROTATE_LIMIT_X, CAMERA_ROTATE_LIMIT_X),
			std::clamp(a.n, -CAMERA_ROTATE_LIMIT_Y, CAMERA_ROTATE_LIMIT_Y)
		};
	}

  private:
	std::array<bool, 4> held{};
	int idleMs = 0;

	[[nodiscard]] float axis(Dir dir) const { return held[static_cast<size_t>(dir)] ? 1.f : 0.f; }
	static float ease(float angle, float keep) {
		const float eased = angle * keep;
		return std::abs(eased) < CAMERA_RECENTER_SNAP ? 0.f : eased;
	}
};

#endif
