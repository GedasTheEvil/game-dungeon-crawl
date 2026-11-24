// The GL core path's CPU side (plan gl-core-foundation): Mat4 against the matrices GL and GLU made, MatrixStack's
// verbs, project against gluProject's numbers, Batch's quads split into triangles, and WideLines' stages (plan
// gl-core-profile). No GL context.
#include "../../external/doctest/doctest.h"
#include "../../src/render/gl/batch.h"
#include "../../src/render/gl/mat4.h"
#include "../../src/render/gl/matrix_stack.h"
#include "../../src/render/gl/wide_lines.h"
#include <array>
#include <cmath>
#include <numbers>
#include <string>
#include <vector>

namespace {
void checkMatrix(const Mat4& got, const std::array<float, 16>& want) {
	for (size_t i = 0; i < 16; i++) {
		INFO("element ", i);
		CHECK(got.m[i] == doctest::Approx(want[i]).epsilon(1e-6));
	}
}

// The scene's camera (graphics/draw.cpp) at 640x360: gluPerspective(45, 16:9, 10, 300).
Mat4 sceneProjection() { return Mat4::perspective(45.0, 640.0 / 360.0, 10.0, 300.0); }

BatchVertex at(float x, float y) {
	BatchVertex v;
	v.x = x;
	v.y = y;
	return v;
}

std::vector<float> xs(const std::vector<BatchVertex>& vertices) {
	std::vector<float> out;
	for (const BatchVertex& v : vertices)
		out.push_back(v.x);
	return out;
}
} // namespace

TEST_CASE("Mat4: perspective is gluPerspective's matrix, ortho glOrtho's") {
	// Read back from GL after gluPerspective(45, 640 / 360., 10, 300).
	checkMatrix(
		sceneProjection(),
		{1.35799515f, 0, 0, 0, 0, 2.41421366f, 0, 0, 0, 0, -1.06896555f, -1, 0, 0, -20.6896553f, 0}
	);
	// glOrtho(0, 640, 0, 360, -1, 1): the UI's.
	checkMatrix(
		Mat4::ortho(0, 640, 0, 360, -1, 1),
		{2.f / 640, 0, 0, 0, 0, 2.f / 360, 0, 0, 0, 0, -1, 0, -1, -1, 0, 1}
	);
}

TEST_CASE("Mat4: rotations as glRotatef, axis-aligned and not") {
	const auto s = static_cast<float>(std::sin(std::numbers::pi / 6));
	const auto c = static_cast<float>(std::cos(std::numbers::pi / 6));
	checkMatrix(Mat4::rotation(30, 0, 1, 0), {c, 0, -s, 0, 0, 1, 0, 0, s, 0, c, 0, 0, 0, 0, 1});
	checkMatrix(Mat4::rotation(30, 0, 0, -2), {c, -s, 0, 0, s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});
	// Round a diagonal, the general formula: a third of a turn round (1, 1, 1) cycles the axes.
	checkMatrix(Mat4::rotation(120, 1, 1, 1), {0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1});
	// No axis: no rotation, as GL.
	CHECK(Mat4::rotation(45, 0, 0, 0) == Mat4::identity());
}

// Plan gl-core-world: Mesa takes sinf and cosf of the angle as a float, so a quarter turn keeps a cos of -4.4e-8 (the
// float pi / 2 is past the real one), not the double's 6e-17; a model turned by 90 degrees otherwise lands a pixel
// off. The bits as glGetFloatv(GL_MODELVIEW_MATRIX) read them after glRotatef(90, 0, 1, 0) on llvmpipe.
TEST_CASE("Mat4: a quarter turn has GL's bits") {
	const Mat4 r = Mat4::rotation(90, 0, 1, 0);
	CHECK(r.at(0, 0) == -0x1.777a5cp-25f);
	CHECK(r.at(2, 2) == -0x1.777a5cp-25f);
	CHECK(r.at(0, 2) == 1.f);
	CHECK(r.at(2, 0) == -1.f);
}

TEST_CASE("MatrixStack: the GL stack's verbs, right-multiplied, push and pop") {
	MatrixStack stack;
	CHECK(stack.depth() == 1);
	CHECK(stack.top() == Mat4::identity());
	stack.translate(0, -20, -70);
	stack.push();
	CHECK(stack.depth() == 2);
	stack.rotate(90, 0, 0, 1);
	stack.scale(2, 3, 4);
	// A point (1, 1, 1): scaled to (2, 3, 4), turned a quarter round z to (-3, 2, 4), moved to (-3, -18, -66).
	const Vec4 p = stack.top() * Vec4{1, 1, 1, 1};
	CHECK(p.x == doctest::Approx(-3));
	CHECK(p.y == doctest::Approx(-18));
	CHECK(p.z == doctest::Approx(-66));
	CHECK(p.w == doctest::Approx(1));
	stack.pop();
	checkMatrix(stack.top(), {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, -20, -70, 1});
	stack.pop(); // the last one stays, as glPopMatrix's underflow
	CHECK(stack.depth() == 1);
	stack.mult(Mat4::scaling(2, 2, 2));
	CHECK(stack.top().at(0, 0) == 2.f);
	CHECK(stack.top().at(1, 3) == -20.f); // a scale on the right leaves the translation
	stack.loadIdentity();
	CHECK(stack.top() == Mat4::identity());
	stack.load(Mat4::translation(1, 2, 3));
	CHECK(stack.top().m[14] == 3.f);
}

TEST_CASE("project: gluProject's window coordinates for the scene's camera") {
	MatrixStack mv;
	mv.translate(0, -20, -70);
	mv.rotate(30, 0, 1, 0);
	const std::array<int, 4> viewport{0, 0, 640, 360};
	struct Case {
		Vec3 obj;
		double x, y, z; // gluProject's, with the same matrices
	};
	const Case cases[] = {
		{{0, 16, -22}, 266.3222279628298, 160.48080973993703, 0.91831735951105564},
		{{10, -5, 3}, 380.98213876902309, 29.94927092099238, 0.8916022139514227},
		{{-40, 0, 0}, 18.92908053760948, 6.1766166687011754, 0.82758622407913207},
	};
	for (const Case& k : cases) {
		const auto w = project(k.obj, mv.top(), sceneProjection(), viewport);
		REQUIRE(w);
		CHECK(w->x == doctest::Approx(k.x).epsilon(1e-12));
		CHECK(w->y == doctest::Approx(k.y).epsilon(1e-12));
		CHECK(w->z == doctest::Approx(k.z).epsilon(1e-12));
	}
	// On the eye's plane w is 0: no point, as gluProject's GL_FALSE.
	CHECK_FALSE(project({0, 0, 0}, Mat4::identity(), sceneProjection(), viewport));
}

TEST_CASE("Batch: quads, quad strips and polygons become triangles, the rest stays") {
	const std::vector<BatchVertex> quads =
		{at(0, 0), at(1, 0), at(2, 0), at(3, 0), at(4, 0), at(5, 0), at(6, 0), at(7, 0)};
	std::vector<BatchVertex> out;
	// Each quad a, b, c, d as a, b, c and a, c, d: the split llvmpipe makes (the render snapshots match).
	CHECK(Batch::expand(Primitive::Quads, quads, out) == Primitive::Triangles);
	CHECK(xs(out) == std::vector<float>{0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7});
	// An unfinished quad is dropped, as GL drops it.
	CHECK(Batch::expand(Primitive::Quads, std::span(quads).first(6), out) == Primitive::Triangles);
	CHECK(xs(out) == std::vector<float>{0, 1, 2, 0, 2, 3});
	// A strip of two quads: 0 1 3 2, then 2 3 5 4.
	CHECK(Batch::expand(Primitive::QuadStrip, std::span(quads).first(6), out) == Primitive::Triangles);
	CHECK(xs(out) == std::vector<float>{0, 1, 3, 0, 3, 2, 2, 3, 5, 2, 5, 4});
	CHECK(Batch::expand(Primitive::Polygon, std::span(quads).first(5), out) == Primitive::Triangles);
	CHECK(xs(out) == std::vector<float>{0, 1, 2, 0, 2, 3, 0, 3, 4});
	CHECK(Batch::expand(Primitive::TriangleFan, std::span(quads).first(5), out) == Primitive::TriangleFan);
	CHECK(xs(out) == std::vector<float>{0, 1, 2, 3, 4});
}

TEST_CASE("Batch: colour, texture coordinate and normal stay for the vertices after them, also past end") {
	Batch batch; // begin and vertex need no GL context; end() does
	batch.colour(0.5f, 0.25f, 0.f);
	batch.begin(Primitive::Triangles);
	batch.texCoord(1.f, 0.f);
	batch.vertex(1.f, 2.f);
	batch.normal(0.f, 1.f, 0.f);
	batch.vertex(3.f, 4.f, 5.f);
	const auto pending = batch.pending();
	REQUIRE(pending.size() == 2);
	CHECK(pending[0].r == 0.5f);
	CHECK(pending[0].a == 1.f);
	CHECK(pending[0].u == 1.f);
	CHECK(pending[0].z == 0.f);
	CHECK(pending[0].ny == 0.f);
	CHECK(pending[1].ny == 1.f);
	CHECK(pending[1].g == 0.25f);
	CHECK(pending[1].z == 5.f);
	batch.begin(Primitive::Lines); // a new begin starts empty, the current values stay
	batch.vertex(0.f, 0.f);
	REQUIRE(batch.pending().size() == 1);
	CHECK(batch.pending()[0].r == 0.5f);
}

// Plan gl-core-profile: what a core context does not draw as the legacy one did goes through a geometry shader; the
// stage follows the widths llvmpipe drew (a plain line in whole pixels, ties to even; a point likewise).
TEST_CASE("WideLines: the stage of a line style, plain widths rounded as llvmpipe") {
	using WideLines::Stage;
	auto stage = [](WideLines::LineStyle s) { return WideLines::stageOf(s); };
	CHECK(stage({}) == Stage::None);
	CHECK(stage({.width = 1.4f}) == Stage::None); // draws 1 pixel: GL's own line
	CHECK(stage({.width = 1.5f}) == Stage::Lines); // draws 2
	CHECK(stage({.width = 2.5f}) == Stage::Lines);
	CHECK(stage({.width = 1.f, .smooth = true}) == Stage::Lines); // smooth at any width
	CHECK(stage({.width = 4.f, .backEdges = true}) == Stage::BackEdges);
	CHECK(stage({.pointSize = 1.4f}) == Stage::None);
	CHECK(stage({.pointSize = 5.f}) == Stage::Points);
}

TEST_CASE("WideLines: a geometry shader per stage, passing the vertex shader's outputs on") {
	const WideLines::Varying varyings[] = {{"vec4", "vColour"}};
	CHECK(WideLines::geometrySource(WideLines::Stage::None, varyings, "#version 330 core\n").empty());
	const std::string lines = WideLines::geometrySource(WideLines::Stage::Lines, varyings, "#version 330 core\n");
	CHECK(lines.starts_with("#version 330 core\nlayout(lines) in;"));
	CHECK(lines.find("in vec4 gs_vColour[];") != std::string::npos);
	CHECK(lines.find("out vec4 vColour;") != std::string::npos);
	const std::string edges = WideLines::geometrySource(WideLines::Stage::BackEdges, varyings, "#version 330 core\n");
	CHECK(edges.find("layout(triangles) in;") != std::string::npos);
	const std::string points = WideLines::geometrySource(WideLines::Stage::Points, varyings, "#version 330 core\n");
	CHECK(points.find("layout(points) in;") != std::string::npos);
	CHECK(WideLines::vertexDefines(WideLines::Stage::Lines, varyings) == "#define vColour gs_vColour\n");
	CHECK(WideLines::vertexDefines(WideLines::Stage::None, varyings).empty());
}
