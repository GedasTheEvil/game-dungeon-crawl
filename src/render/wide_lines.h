#ifndef WIDE_LINES_H
#define WIDE_LINES_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include "gl_includes.h"

// Lines wider than a pixel, smooth lines, a model's back face edges and big points in a GL core context (plan
// gl-core-profile): a forward-compatible core context has no glLineWidth above 1, no reliable GL_LINE_SMOOTH, no
// glPolygonMode(GL_BACK, ...), and draws every point as a sprite. A geometry shader draws each line as a quad instead,
// `width` pixels across, with the pixels and the fade Mesa's llvmpipe gave the legacy lines (the render snapshots are
// llvmpipe's): a plain line along its minor axis in whole pixels, its ends by the diamond exit rule; a smooth one
// across, fading over a pixel. The default programs and the world shader each have a variant per Stage. A point bigger
// than a pixel is the square llvmpipe gave a legacy point (whole pixels round its snapped centre).
namespace WideLines {

// How a draw's lines (and points) look. Batch keeps a primitive's part: the width and smoothing for lines, the point
// size for points, backEdges for triangles.
struct LineStyle {
	float width = 1.f; // in pixels (glLineWidth)
	bool smooth = false; // GL_LINE_SMOOTH
	// A triangle draw shows only the edges of its back faces, as lines (glCullFace(GL_FRONT) with
	// glPolygonMode(GL_BACK, GL_LINE)): a model's outline. Culling stays off for it: the shader drops the front faces.
	bool backEdges = false;
	// The back edges' depth offset (glPolygonOffset with GL_POLYGON_OFFSET_LINE): factor times the face's depth slope,
	// plus units of the depth buffer's step. GL's own polygon offset stays off for them.
	float offsetFactor = 0.f;
	float offsetUnits = 0.f;
	float pointSize = 1.f; // in pixels (glPointSize)
};

enum class Stage : std::uint8_t {
	None, // GL's own lines (1 pixel, not smooth) and every other primitive
	Lines, // a line draw as quads
	BackEdges, // a triangle draw's back faces' edges as quads
	Points, // a point draw as squares
};
constexpr std::size_t STAGES = 4;

[[nodiscard]] Stage stageOf(const LineStyle& line);

// A vertex shader output the geometry shader passes on, as GLSL declares it ("vec4", "vColour").
struct Varying {
	const char* type;
	const char* name;
};

// The defines a program of `stage` puts after its version line. The vertex shader's: its outputs renamed to the
// geometry shader's inputs. The fragment shader's: WIDE_LINES, for lineCoverage (FRAGMENT_COVERAGE).
[[nodiscard]] std::string vertexDefines(Stage stage, std::span<const Varying> varyings);
[[nodiscard]] const char* fragmentDefines(Stage stage);
// The geometry shader of `stage` (none for Stage::None: empty), its first line `version`.
[[nodiscard]] std::string geometrySource(Stage stage, std::span<const Varying> varyings, const char* version);
// GLSL for a fragment shader: float lineCoverage(), the share of the pixel a smooth line covers (1 elsewhere); the
// fragment's alpha takes it.
extern const char* const FRAGMENT_COVERAGE;

// The uniforms of a program with a geometry stage.
struct Uniforms {
	GLint viewport = -1, flipY = -1, halfWidth = -1, smooth = -1, depthOffset = -1, pointSize = -1;
	void find(GLuint program);
	// The line style and the viewport's size, into the bound program.
	void set(const LineStyle& line) const;
};

} // namespace WideLines

#endif
