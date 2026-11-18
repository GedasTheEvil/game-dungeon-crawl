#include "mat4.h"

#include <cmath>
#include <numbers>

namespace {
// m[column * 4 + row], as Mesa's M(row, col).
float& set(Mat4& mat, int row, int column) { return mat.m[static_cast<size_t>(column) * 4 + static_cast<size_t>(row)]; }
} // namespace

Mat4 Mat4::fromArray(const float* columnMajor) {
	Mat4 r;
	for (size_t i = 0; i < 16; i++)
		r.m[i] = columnMajor[i];
	return r;
}

Mat4 Mat4::translation(float x, float y, float z) {
	Mat4 r;
	r.m[12] = x;
	r.m[13] = y;
	r.m[14] = z;
	return r;
}

Mat4 Mat4::scaling(float x, float y, float z) {
	Mat4 r;
	r.m[0] = x;
	r.m[5] = y;
	r.m[10] = z;
	return r;
}

// Mesa's _math_matrix_rotate, step for step: sinf and cosf of the angle in radians as a float (cos 90 degrees is then
// -4.4e-8, not 6e-17 as in double; a model turned by 90 degrees lands on other pixels otherwise).
Mat4 Mat4::rotation(float degrees, float x, float y, float z) {
	const auto radians = static_cast<float>(degrees * std::numbers::pi / 180.0);
	const float s = std::sin(radians);
	const float c = std::cos(radians);
	Mat4 r;
	if (x == 0.f && y == 0.f && z != 0.f) {
		set(r, 0, 0) = c;
		set(r, 1, 1) = c;
		set(r, 0, 1) = z < 0.f ? s : -s;
		set(r, 1, 0) = z < 0.f ? -s : s;
		return r;
	}
	if (x == 0.f && z == 0.f && y != 0.f) {
		set(r, 0, 0) = c;
		set(r, 2, 2) = c;
		set(r, 0, 2) = y < 0.f ? -s : s;
		set(r, 2, 0) = y < 0.f ? s : -s;
		return r;
	}
	if (y == 0.f && z == 0.f && x != 0.f) {
		set(r, 1, 1) = c;
		set(r, 2, 2) = c;
		set(r, 1, 2) = x < 0.f ? s : -s;
		set(r, 2, 1) = x < 0.f ? -s : s;
		return r;
	}
	const float mag = std::sqrt(x * x + y * y + z * z);
	if (mag <= 1.0e-4f)
		return r;
	x /= mag;
	y /= mag;
	z /= mag;
	const float oneC = 1.f - c;
	set(r, 0, 0) = (oneC * x * x) + c;
	set(r, 0, 1) = (oneC * x * y) - z * s;
	set(r, 0, 2) = (oneC * z * x) + y * s;
	set(r, 1, 0) = (oneC * x * y) + z * s;
	set(r, 1, 1) = (oneC * y * y) + c;
	set(r, 1, 2) = (oneC * y * z) - x * s;
	set(r, 2, 0) = (oneC * z * x) - y * s;
	set(r, 2, 1) = (oneC * y * z) + x * s;
	set(r, 2, 2) = (oneC * z * z) + c;
	return r;
}

// GLU's gluPerspective (in double, its own pi), then the float the GL keeps.
Mat4 Mat4::perspective(double fovyDegrees, double aspect, double zNear, double zFar) {
	const double radians = fovyDegrees / 2 * std::numbers::pi / 180;
	const double deltaZ = zFar - zNear;
	const double sine = std::sin(radians);
	Mat4 r;
	if (deltaZ == 0 || sine == 0 || aspect == 0)
		return r;
	const double cotangent = std::cos(radians) / sine;
	set(r, 0, 0) = static_cast<float>(cotangent / aspect);
	set(r, 1, 1) = static_cast<float>(cotangent);
	set(r, 2, 2) = static_cast<float>(-(zFar + zNear) / deltaZ);
	set(r, 3, 2) = -1.f;
	set(r, 2, 3) = static_cast<float>(-2 * zNear * zFar / deltaZ);
	set(r, 3, 3) = 0.f;
	return r;
}

// Mesa's _math_matrix_ortho.
Mat4 Mat4::ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
	Mat4 r;
	set(r, 0, 0) = 2.f / (right - left);
	set(r, 0, 3) = -(right + left) / (right - left);
	set(r, 1, 1) = 2.f / (top - bottom);
	set(r, 1, 3) = -(top + bottom) / (top - bottom);
	set(r, 2, 2) = -2.f / (zFar - zNear);
	set(r, 2, 3) = -(zFar + zNear) / (zFar - zNear);
	return r;
}

// Mesa's _math_matrix_frustum.
Mat4 Mat4::frustum(float left, float right, float bottom, float top, float zNear, float zFar) {
	Mat4 r;
	set(r, 0, 0) = (2.f * zNear) / (right - left);
	set(r, 0, 2) = (right + left) / (right - left);
	set(r, 1, 1) = (2.f * zNear) / (top - bottom);
	set(r, 1, 2) = (top + bottom) / (top - bottom);
	set(r, 2, 2) = -(zFar + zNear) / (zFar - zNear);
	set(r, 2, 3) = -(2.f * zFar * zNear) / (zFar - zNear);
	set(r, 3, 2) = -1.f;
	set(r, 3, 3) = 0.f;
	return r;
}

// Mesa's matmul4: each element summed in the same order.
Mat4 Mat4::operator*(const Mat4& rhs) const {
	Mat4 r;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			set(r, i, j) = at(i, 0) * rhs.at(0, j) + at(i, 1) * rhs.at(1, j) + at(i, 2) * rhs.at(2, j) +
						   at(i, 3) * rhs.at(3, j);
	return r;
}

Vec4 Mat4::operator*(const Vec4& v) const {
	auto row = [&](int i) { return at(i, 0) * v.x + at(i, 1) * v.y + at(i, 2) * v.z + at(i, 3) * v.w; };
	return {row(0), row(1), row(2), row(3)};
}

// GLU's gluProject, in double.
std::optional<WindowPoint>
project(Vec3 obj, const Mat4& modelView, const Mat4& projection, const std::array<int, 4>& viewport) {
	auto transform = [](const Mat4& mat, const double in[4], double out[4]) {
		for (int i = 0; i < 4; i++)
			out[i] = in[0] * mat.at(i, 0) + in[1] * mat.at(i, 1) + in[2] * mat.at(i, 2) + in[3] * mat.at(i, 3);
	};
	const double in[4] = {obj.x, obj.y, obj.z, 1.0};
	double eye[4];
	double clip[4];
	transform(modelView, in, eye);
	transform(projection, eye, clip);
	if (clip[3] == 0.0)
		return std::nullopt;
	double ndc[3];
	for (int i = 0; i < 3; i++)
		ndc[i] = clip[i] / clip[3] * 0.5 + 0.5;
	return WindowPoint{ndc[0] * viewport[2] + viewport[0], ndc[1] * viewport[3] + viewport[1], ndc[2]};
}
