#include "page_turn.h"
#include <algorithm>
#include <cmath>

void clampCorner(PageCurl& c) {
	float d = std::hypot(c.cornerX, c.cornerY);
	if (d > c.w) {
		c.cornerX *= c.w / d;
		c.cornerY *= c.w / d;
	}
	float diagonal = std::hypot(c.w, c.h);
	float dx = c.cornerX;
	float dy = c.cornerY - c.h;
	d = std::hypot(dx, dy);
	if (d > diagonal) {
		c.cornerX = dx * diagonal / d;
		c.cornerY = c.h + dy * diagonal / d;
	}
}

float curlProgress(const PageCurl& c) {
	return std::clamp(std::hypot(c.w - c.cornerX, c.cornerY) / (2 * c.w), 0.f, 1.f);
}
