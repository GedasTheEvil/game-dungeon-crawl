#ifndef LOADING_TEXT_H
#define LOADING_TEXT_H
#include <cstring>

// The loading screen's two text lines (plan loading-two-lines). The fonts are monospaced, so the width maths is
// plain: Assets::LoadLoadingScreen, DrawLoad and the unit test share these numbers.
namespace loading_text {
constexpr float STEP_SIZE = 7.f; // line 1: what loads
constexpr float STEP_SPACING = -1.f;
constexpr float DETAIL_SIZE = 5.f; // line 2: the detail, smaller
constexpr float DETAIL_SPACING = -0.5f;
constexpr float LEFT = 10.f; // both lines start where the bar does
constexpr float MAX_WIDTH = 120.f; // x 10 to 130 of the 140-wide screen

constexpr float STEP_ADVANCE = STEP_SIZE / 2 + STEP_SPACING;
constexpr float DETAIL_ADVANCE = DETAIL_SIZE / 2 + DETAIL_SPACING;

[[nodiscard]] inline float stepWidth(const char* text) { return static_cast<float>(std::strlen(text)) * STEP_ADVANCE; }
[[nodiscard]] inline float detailWidth(const char* text) {
	return static_cast<float>(std::strlen(text)) * DETAIL_ADVANCE;
}
} // namespace loading_text

#endif
