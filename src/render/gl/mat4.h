#ifndef MAT4_H
#define MAT4_H

#include <array>
#include <optional>

// The GL core path's math (plan gl-core-foundation): a 4x4 float matrix in GL's column-major layout and the few
// transforms the fixed-function pipeline and GLU used to make. Our own rather than glm: glm's headers cost about 0.3 s
// per translation unit, and this is all the drawing needs. No GL here: the unit tests check it.

struct Vec3 {
	float x = 0.f, y = 0.f, z = 0.f;
};

struct Vec4 {
	float x = 0.f, y = 0.f, z = 0.f, w = 0.f;
};

struct Mat4 {
	// Column-major, as glLoadMatrixf and glUniformMatrix4fv take it: m[column * 4 + row]; the translation is 12..14.
	std::array<float, 16> m{1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f};

	[[nodiscard]] float at(int row, int column) const {
		return m[static_cast<size_t>(column) * 4 + static_cast<size_t>(row)];
	}
	[[nodiscard]] const float* data() const { return m.data(); }

	[[nodiscard]] static Mat4 identity() { return {}; }
	[[nodiscard]] static Mat4 fromArray(const float* columnMajor);
	// glTranslatef, glScalef, glRotatef (degrees round the axis, normalised; the axis-aligned cases exact as Mesa's).
	[[nodiscard]] static Mat4 translation(float x, float y, float z);
	[[nodiscard]] static Mat4 scaling(float x, float y, float z);
	[[nodiscard]] static Mat4 rotation(float degrees, float x, float y, float z);
	// gluPerspective (vertical field of view in degrees), in double as GLU computed it; glOrtho and glFrustum in float
	// as Mesa does. The same bits as the calls they replace, so a ported view draws the same pixels.
	[[nodiscard]] static Mat4 perspective(double fovyDegrees, double aspect, double zNear, double zFar);
	[[nodiscard]] static Mat4 ortho(float left, float right, float bottom, float top, float zNear, float zFar);
	[[nodiscard]] static Mat4 frustum(float left, float right, float bottom, float top, float zNear, float zFar);

	[[nodiscard]] Mat4 operator*(const Mat4& rhs) const;
	[[nodiscard]] Vec4 operator*(const Vec4& v) const;
	[[nodiscard]] bool operator==(const Mat4& rhs) const = default;
};

// A point in window coordinates: x, y in pixels from the window's corner, z 0..1 over the depth range.
struct WindowPoint {
	double x = 0.0, y = 0.0, z = 0.0;
};

// gluProject, in double as GLU: where `obj` lands on the window under the model view and projection; none where the
// projected w is 0. viewport: x, y, width, height.
[[nodiscard]] std::optional<WindowPoint>
project(Vec3 obj, const Mat4& modelView, const Mat4& projection, const std::array<int, 4>& viewport);

#endif
