#include "page_curl.h"
#include "../graphics/gl_includes.h"
#include <GL/gl.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace {

constexpr int COLUMNS = 48;
constexpr int ROWS = 20;
constexpr float MAX_RADIUS = 5.f;	  // of the curl, in the middle of the turn
constexpr float EYE_DISTANCE = 220.f; // the lifted part grows by d / (d - z)
constexpr float SHADOW_REACH = 4.f;	  // beyond the curl, on the page under it
constexpr float SHADOW_ALPHA = 0.32f;
constexpr float PI = static_cast<float>(M_PI);

// Rows of `columns` + 1 vertices.
size_t meshIndex(int i, int j, int columns) {
	return static_cast<size_t>(j) * static_cast<size_t>(columns + 1) + static_cast<size_t>(i);
}

struct Vec2 {
	float x, y;
};

float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }

// The fold: points with s = (p - origin) . normal > 0 curl over.
struct Fold {
	Vec2 origin{}, normal{};
	float radius = 0;
	bool flat = true;
};

Fold foldOf(const PageCurl& c) {
	Fold f;
	Vec2 d = {c.w - c.cornerX, -c.cornerY}; // from where the corner is pulled to the corner
	float len = std::hypot(d.x, d.y);
	if (len < 1e-3f)
		return f;
	f.flat = false;
	f.normal = {d.x / len, d.y / len};
	f.radius = MAX_RADIUS * std::sin(PI * curlProgress(c));
	// The cylinder sits so the corner lands where it is pulled: half its half-turn back from the bisector.
	float shift = -PI * f.radius / 2;
	// The spine stays put: no point of it may be on the curling side.
	Vec2 mid = {(c.w + c.cornerX) / 2, c.cornerY / 2};
	float spine = std::max(dot({-mid.x, -mid.y}, f.normal), dot({-mid.x, c.h - mid.y}, f.normal));
	shift = std::max(shift, spine);
	f.origin = {mid.x + shift * f.normal.x, mid.y + shift * f.normal.y};
	return f;
}

struct Vertex {
	float x, y, z;	 // canvas, before the perspective
	float shade = 1; // light on the surface
};

Vertex bend(const Fold& f, float x, float y) {
	if (f.flat)
		return {x, y, 0.f};
	float s = dot({x - f.origin.x, y - f.origin.y}, f.normal);
	if (s <= 0)
		return {x, y, 0.f};
	Vec2 base = {x - s * f.normal.x, y - s * f.normal.y}; // on the fold line
	float r = f.radius;
	float along = 0, z = 0, theta = PI;
	if (r > 1e-3f && s < PI * r) {
		theta = s / r;
		along = r * std::sin(theta);
		z = r * (1 - std::cos(theta));
	} else {
		along = -(s - PI * r);
		z = 2 * r;
	}
	z += 0.002f * s; // the turned part lies over the flat part even with no curl left
	float shade = 0.62f + 0.38f * std::abs(std::cos(theta));
	return {base.x + along * f.normal.x, base.y + along * f.normal.y, z, shade};
}

// A polygon with an alpha per corner, clipped to the page (Sutherland-Hodgman).
struct ShadowPoint {
	float x, y, a;
};
using Polygon = std::vector<ShadowPoint>;

Polygon clip(const Polygon& in, int axis, float bound, bool keepBelow) {
	Polygon out;
	auto inside = [&](const ShadowPoint& p) {
		float v = axis == 0 ? p.x : p.y;
		return keepBelow ? v <= bound : v >= bound;
	};
	for (size_t i = 0; i < in.size(); i++) {
		const ShadowPoint& a = in[i];
		const ShadowPoint& b = in[(i + 1) % in.size()];
		bool ia = inside(a);
		bool ib = inside(b);
		if (ia)
			out.push_back(a);
		if (ia != ib) {
			float va = axis == 0 ? a.x : a.y;
			float vb = axis == 0 ? b.x : b.y;
			float t = (bound - va) / (vb - va);
			out.push_back({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.a + (b.a - a.a) * t});
		}
	}
	return out;
}

} // namespace

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

void drawPageCurl(const PageCurl& c, const PageCurlPlacement& at) {
	const Fold fold = foldOf(c);
	const float side = at.mirror ? -1.f : 1.f;
	auto toCanvas = [&](float x, float y, float z, float& outX, float& outY) {
		float cx = at.spineX + side * x;
		float cy = at.bottomY + y;
		float grow = EYE_DISTANCE / (EYE_DISTANCE - z);
		outX = at.eyeX + (cx - at.eyeX) * grow;
		outY = at.eyeY + (cy - at.eyeY) * grow;
	};

	glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);

	// The shadow of the curl on the revealed page: darkest at the fold, fading out past the cylinder.
	if (!fold.flat) {
		Vec2 n = fold.normal;
		Vec2 t = {-n.y, n.x};
		float reach = fold.radius + SHADOW_REACH;
		float span = 2 * (c.w + c.h);
		Vec2 o = fold.origin;
		Polygon band = {
			{o.x - t.x * span, o.y - t.y * span, SHADOW_ALPHA},
			{o.x + t.x * span, o.y + t.y * span, SHADOW_ALPHA},
			{o.x + t.x * span + n.x * reach, o.y + t.y * span + n.y * reach, 0.f},
			{o.x - t.x * span + n.x * reach, o.y - t.y * span + n.y * reach, 0.f},
		};
		band = clip(clip(clip(clip(band, 0, 0.f, false), 0, c.w, true), 1, 0.f, false), 1, c.h, true);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBegin(GL_POLYGON);
		for (const ShadowPoint& p : band) {
			float x = 0;
			float y = 0;
			toCanvas(p.x, p.y, 0.f, x, y);
			glColor4f(0, 0, 0, p.a);
			glVertex2f(x, y);
		}
		glEnd();
	}

	// Columns at the same step on into the overhang; column COLUMNS is the free edge.
	const float step = c.w / COLUMNS;
	const int columns = COLUMNS + static_cast<int>(std::ceil(c.overhang / step - 1e-3f));
	const float reach = c.w + c.overhang;
	std::vector<Vertex> mesh(meshIndex(columns, ROWS, columns) + 1, Vertex{0, 0, 0});
	auto columnX = [&](int i) { return std::min(reach, step * static_cast<float>(i)); };
	for (int j = 0; j <= ROWS; j++)
		for (int i = 0; i <= columns; i++) {
			float x = columnX(i);
			float y = c.h * static_cast<float>(j) / ROWS;
			Vertex v = bend(fold, x, y);
			toCanvas(v.x, v.y, v.z, v.x, v.y);
			mesh[meshIndex(i, j, columns)] = v;
		}

	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glEnable(GL_ALPHA_TEST); // the overhang is clear but for what hangs out of the page
	glAlphaFunc(GL_GREATER, 0.5f);
	glFrontFace(at.mirror ? GL_CW : GL_CCW);
	// Front: u runs left to right on the canvas, so from the spine on a right page. Back: laid out as the page on
	// the other side of the spine, the spine at its other end.
	for (int pass = 0; pass < 2; pass++) {
		bool front = pass == 0;
		glCullFace(front ? GL_BACK : GL_FRONT);
		glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(front ? at.front : at.back));
		for (int j = 0; j < ROWS; j++) {
			glBegin(GL_TRIANGLE_STRIP);
			for (int i = 0; i <= columns; i++) {
				float fromSpine = columnX(i) / reach;
				float u = (front != at.mirror) ? fromSpine : 1 - fromSpine;
				for (int k = 1; k >= 0; k--) {
					const Vertex& v = mesh[meshIndex(i, j + k, columns)];
					glColor4f(v.shade, v.shade, v.shade, 1.f);
					glTexCoord2f(u, static_cast<float>(j + k) / ROWS);
					glVertex3f(v.x, v.y, v.z);
				}
			}
			glEnd();
		}
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	// A faint edge, so the turning page stands out from the paper under it. A hair above the page, hidden where the
	// page covers itself.
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glColor4f(0, 0, 0, 0.18f);
	auto edge = [&](int i, int j) {
		const Vertex& v = mesh[meshIndex(i, j, columns)];
		glVertex3f(v.x, v.y, v.z + 0.05f);
	};
	glBegin(GL_LINE_STRIP);
	for (int i = 0; i <= COLUMNS; i++)
		edge(i, 0);
	for (int j = 0; j <= ROWS; j++)
		edge(COLUMNS, j);
	for (int i = COLUMNS; i >= 0; i--)
		edge(i, ROWS);
	glEnd();
	glPopAttrib();
}
