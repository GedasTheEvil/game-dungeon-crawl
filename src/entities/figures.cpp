#include "figures.h"

namespace {
bool gToon = false;
} // namespace

float Figures::Scale() { return gToon ? 1.2f : 1.f; }

void Figures::SetToon(bool on) { gToon = on; }
