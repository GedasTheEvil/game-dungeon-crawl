#include "view_window.h"
#include <algorithm>
#include <cmath>

namespace {
// Row k of a column-major 4x4 matrix.
struct Row {
	float x, y, z, w;
};
Row row(const float m[16], int k) { return {m[k], m[4 + k], m[8 + k], m[12 + k]}; }
} // namespace

// Each screen corner's ray meets the planes z 0 and z -tileSize: on the plane, the point whose clip x / w and y / w
// are the corner's (+-1) solves two linear equations in x and y. A ray that misses a plane (parallel to it, or it
// meets it behind the camera: a turned camera at a wide window) sees the level up to the cap on its side.
CellRect drawnWindow(const ViewWindow& view, const float clip[16], float tileSize) {
	const CellRect base = view.cells();
	const Row rx = row(clip, 0);
	const Row ry = row(clip, 1);
	const Row rw = row(clip, 3);
	constexpr float FAR = 1e6f;
	float minX = FAR;
	float maxX = -FAR;
	float minY = FAR;
	float maxY = -FAR;
	for (const float z : {0.f, -tileSize})
		for (const float sx : {-1.f, 1.f})
			for (const float sy : {-1.f, 1.f}) {
				const Row a{rx.x - sx * rw.x, rx.y - sx * rw.y, rx.z - sx * rw.z, rx.w - sx * rw.w};
				const Row b{ry.x - sy * rw.x, ry.y - sy * rw.y, ry.z - sy * rw.z, ry.w - sy * rw.w};
				const float det = a.x * b.y - a.y * b.x;
				float x = sx * FAR;
				float y = sy * FAR;
				if (std::fabs(det) > 1e-9f) {
					const float ca = -(a.z * z + a.w);
					const float cb = -(b.z * z + b.w);
					const float px = (ca * b.y - a.y * cb) / det;
					const float py = (a.x * cb - ca * b.x) / det;
					if (rw.x * px + rw.y * py + rw.z * z + rw.w > 0.f) {
						x = px;
						y = py;
					}
				}
				minX = std::min(minX, x);
				maxX = std::max(maxX, x);
				minY = std::min(minY, y);
				maxY = std::max(maxY, y);
			}
	// In cells from the window's origin, clamped while still float: a missed ray's FAR does not fit an int.
	constexpr float MARGIN = 1.f;
	auto cell = [&](float v, int lo, int hi) {
		return static_cast<int>(std::floor(std::clamp(v / tileSize, static_cast<float>(lo), static_cast<float>(hi))));
	};
	const int left0 = base.col0 - view.originCol; // the gameplay window, from the origin
	const int right0 = left0 + base.cols;
	const int bottom0 = 0;
	const int top0 = base.rows;
	const int left = cell(minX - MARGIN * tileSize, left0 - DRAWN_EXTRA_COLS, left0);
	const int right = cell(maxX + MARGIN * tileSize, right0 - 1, right0 + DRAWN_EXTRA_COLS - 1) + 1;
	const int bottom = cell(minY - MARGIN * tileSize, bottom0 - DRAWN_EXTRA_ROWS, bottom0);
	const int top = cell(maxY + MARGIN * tileSize, top0 - 1, top0 + DRAWN_EXTRA_ROWS - 1) + 1;
	return {view.originCol + left, view.originRow + bottom, right - left, top - bottom};
}
