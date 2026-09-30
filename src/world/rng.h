#ifndef RNG_H
#define RNG_H

// A small random generator (splitmix64) that gives the same sequence on every platform, so a seed means the same
// levels (level_gen), loot and splashes everywhere. Each user owns its own stream.

#include <cstdint>

class Rng {
  public:
	explicit Rng(uint64_t seed = 1) : state(seed * 0x9e3779b97f4a7c15ULL + 0x632be59bd9b4e019ULL) {}
	uint32_t next() {
		uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
		z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
		z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
		return static_cast<uint32_t>((z ^ (z >> 31)) >> 32);
	}
	int range(int lo, int hi) { // inclusive
		return hi <= lo ? lo : lo + static_cast<int>(next() % static_cast<uint32_t>(hi - lo + 1));
	}
	int below(int n) { return n <= 0 ? 0 : static_cast<int>(next() % static_cast<uint32_t>(n)); } // 0 .. n - 1
	bool chance(float p) { return static_cast<float>(next() % 10000U) < p * 10000.f; }
	bool percent(int p) { return below(100) < p; }

  private:
	uint64_t state;
};

#endif
