#ifndef FIGURES_H
#define FIGURES_H

// The player, the monsters and the items are drawn larger in toon mode: the ink outlines eat into their silhouettes.
// Their hitboxes follow the drawing, so the sim reads the scale here. Ink::setToon sets it, a unit test may too.
namespace Figures {
[[nodiscard]] float Scale(); // 1, or 1.2 in toon mode
void SetToon(bool on);
} // namespace Figures

#endif
