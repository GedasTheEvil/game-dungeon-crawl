#include "vitals.h"
#include "head_bump.h"

int Vitals::HeadBumpLoss(int maxHp) const { return std::min(headBumpHp(maxHp), std::max(0, hp)); }

void Vitals::Clear() {
	poison.Cure();
	poisonByPlayer = false;
	burning.Douse();
	burnByPlayer = false;
	trapCarry = 0;
	ComeTo();
}
