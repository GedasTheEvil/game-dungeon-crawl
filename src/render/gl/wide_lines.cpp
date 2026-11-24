#include "wide_lines.h"

#include <algorithm>
#include <cmath>

namespace {
// The part of every geometry shader after its layout lines. Each line becomes a quad (a triangle strip) with the
// corners Mesa's llvmpipe gives it (the render snapshots are llvmpipe's), worked out in its window space and put back
// into clip space, so depth and the varyings stay the line's: a plain wide line as llvmpipe's line setup draws it
// (lp_setup_line.c), a smooth one as the draw module's antialiased line stage (draw_pipe_aaline.c), a triangle's
// back face edges as its unfilled and polygon offset stages (draw_pipe_unfilled.c, draw_pipe_offset.c).
const char* const SEGMENT_SRC = R"(
uniform vec4 uViewport; // x, y, width, height in pixels
uniform float uFlipY; // 1: the window's own framebuffer, which llvmpipe addresses from the top
uniform float uLineHalf; // plain: half the whole pixels; smooth: half the quad's width
uniform float uLineSmooth;
uniform vec2 uDepthOffset; // back edges: the window depth offset per unit of the face's slope, and its constant
noperspective out vec4 gAa; // smooth: across (-x..x), x, along (-z..z), z: the aaline stage's texture coordinate
float gZOffset = 0.0; // the face's depth offset, window space
// In llvmpipe's window space, rounded as Mesa's viewport transform rounds: y down on the window's framebuffer (from
// the viewport's top; the tests below take only fractions, so any whole offset does).
vec3 toRaster(vec4 clip) {
	vec3 ndc = clip.xyz * (1.0 / clip.w);
	vec2 halfSize = uViewport.zw * 0.5;
	vec2 xy = uFlipY > 0.5 ? vec2(ndc.x * halfSize.x + (uViewport.x + halfSize.x), ndc.y * -halfSize.y + halfSize.y)
						   : ndc.xy * halfSize + (uViewport.xy + halfSize);
	return vec3(xy, ndc.z * 0.5 + 0.5);
}
// Vertex i moved by offset (raster pixels), its depth by the face's offset.
void emitAt(int i, vec2 offset, vec4 aa) {
	passOn(i);
	vec4 p = gl_in[i].gl_Position;
	if (uFlipY > 0.5)
		offset.y = -offset.y;
	p.xy += offset * 2.0 / uViewport.zw * p.w;
	if (gZOffset != 0.0) // Mesa's window depth, offset and clamped; back in clip space
		p.z = (clamp(p.z / p.w * 0.5 + 0.5 + gZOffset, 0.0, 1.0) * 2.0 - 1.0) * p.w;
	gl_Position = p;
	gAa = aa;
	EmitVertex();
}
bool sgn(float x) { return x >= 0.0; }
float fracf(float f) { return f - floor(f); }
// A plain line: 2 * uLineHalf whole pixels along the minor axis, and at each end the diamond exit rule (a pixel is the
// line's where the line leaves its diamond): the quad's ends moved along the line to keep or drop the end pixels.
void plainSegment(int a, int b) {
	bool swapped = false;
	vec2 v1 = toRaster(gl_in[a].gl_Position).xy, v2 = toRaster(gl_in[b].gl_Position).xy;
	float dx = v1.x - v2.x, dy = v1.y - v2.y;
	if (dx * dx + dy * dy == 0.0)
		return;
	float x1diff = v1.x - floor(v1.x) - 0.5, y1diff = v1.y - floor(v1.y) - 0.5;
	float x2diff = v2.x - floor(v2.x) - 0.5, y2diff = v2.y - floor(v2.y) - 0.5;
	bool drawStart, drawEnd, willDrawStart, willDrawEnd;
	vec2 off1 = vec2(0.0), off2 = vec2(0.0);
	vec2 across;
	if (abs(dx) >= abs(dy)) {
		float dydx = dy / dx;
		if (y2diff == -0.5 && dy < 0.0)
			y2diff = 0.5;
		if (abs(x1diff) + abs(y1diff) < 0.5)
			drawStart = true;
		else if (sgn(x1diff) == sgn(-dx))
			drawStart = false;
		else if (sgn(-y1diff) != sgn(dy))
			drawStart = true;
		else {
			float yi = fracf(v1.y) + x1diff * dydx;
			drawStart = yi < 1.0 && yi > 0.0;
		}
		if (abs(x2diff) + abs(y2diff) < 0.5)
			drawEnd = false;
		else if (sgn(x2diff) != sgn(-dx))
			drawEnd = false;
		else if (sgn(-y2diff) == sgn(dy))
			drawEnd = true;
		else {
			float yi = fracf(v2.y) + x2diff * dydx;
			drawEnd = yi < 1.0 && yi > 0.0;
		}
		float xo = 0.0, xoe = 0.0;
		if (dx < 0.0) { // v2 right of v1: the ends swap
			vec2 t = v1;
			v1 = v2;
			v2 = t;
			swapped = true;
			willDrawStart = x1diff <= 0.0;
			if (willDrawStart != drawStart)
				xoe = -x1diff - 0.5;
			willDrawEnd = x2diff > 0.0;
			if (willDrawEnd != drawEnd)
				xo = -x2diff - 0.5;
		} else {
			willDrawStart = x1diff > 0.0;
			if (willDrawStart != drawStart)
				xo = -x1diff + 0.5;
			willDrawEnd = x2diff <= 0.0;
			if (willDrawEnd != drawEnd)
				xoe = -x2diff + 0.5;
		}
		off1 = vec2(xo, xo * dydx);
		off2 = vec2(xoe, xoe * dydx);
		across = vec2(0.0, uLineHalf);
	} else {
		float dxdy = dx / dy;
		if (x2diff == -0.5 && dx < 0.0)
			x2diff = 0.5;
		if (abs(x1diff) + abs(y1diff) < 0.5)
			drawStart = true;
		else if (sgn(-y1diff) == sgn(dy))
			drawStart = false;
		else if (sgn(x1diff) != sgn(-dx))
			drawStart = true;
		else {
			float xi = fracf(v1.x) + y1diff * dxdy;
			drawStart = xi < 1.0 && xi > 0.0;
		}
		if (abs(x2diff) + abs(y2diff) < 0.5)
			drawEnd = false;
		else if (sgn(-y2diff) != sgn(dy))
			drawEnd = false;
		else if (sgn(x2diff) == sgn(-dx))
			drawEnd = true;
		else {
			float xi = fracf(v2.x) + y2diff * dxdy;
			drawEnd = xi < 1.0 && xi >= 0.0;
		}
		bool bottomEdgeRule = uFlipY > 0.5; // GL's fill rule on a framebuffer addressed from the top
		float yo = 0.0, yoe = 0.0;
		if (dy > 0.0) { // v2 above v1: the ends swap
			vec2 t = v1;
			v1 = v2;
			v2 = t;
			swapped = true;
			willDrawStart = bottomEdgeRule ? y1diff >= 0.0 : y1diff > 0.0;
			willDrawEnd = bottomEdgeRule ? y2diff < 0.0 : y2diff <= 0.0;
			if (willDrawStart != drawStart)
				yoe = -y1diff + 0.5;
			if (willDrawEnd != drawEnd)
				yo = -y2diff + 0.5;
		} else {
			willDrawStart = bottomEdgeRule ? y1diff < 0.0 : y1diff <= 0.0;
			willDrawEnd = bottomEdgeRule ? y2diff >= 0.0 : y2diff > 0.0;
			if (willDrawStart != drawStart)
				yo = -y1diff - 0.5;
			if (willDrawEnd != drawEnd)
				yoe = -y2diff - 0.5;
		}
		off1 = vec2(yo * dxdy, yo);
		off2 = vec2(yoe * dxdy, yoe);
		across = vec2(uLineHalf, 0.0);
	}
	// Constant vertex indices (a and b, the swap in a branch): llvmpipe draws nothing for a computed one.
	vec4 none = vec4(0.0);
	if (swapped) {
		emitAt(b, off1 - across, none);
		emitAt(b, off1 + across, none);
		emitAt(a, off2 - across, none);
		emitAt(a, off2 + across, none);
	} else {
		emitAt(a, off1 - across, none);
		emitAt(a, off1 + across, none);
		emitAt(b, off2 - across, none);
		emitAt(b, off2 + across, none);
	}
	EndPrimitive();
}
// A smooth line: half a pixel longer at each end and uLineHalf to each side, the coverage fading over the last pixel
// across and along; split along the same diagonal as the aaline stage.
void smoothSegment(int a, int b) {
	vec2 d = toRaster(gl_in[b].gl_Position).xy - toRaster(gl_in[a].gl_Position).xy;
	float len = sqrt(d.x * d.x + d.y * d.y);
	vec2 cs = d / len; // cos, sin
	float hl = 0.5 * len + 0.5;
	float hw = uLineHalf;
	vec2 l = 0.5 * cs;
	vec2 w = hw * vec2(-cs.y, cs.x);
	emitAt(a, -l + w, vec4(-hw, hw, -hl, hl));
	emitAt(a, -l - w, vec4(hw, hw, -hl, hl));
	emitAt(b, l + w, vec4(-hw, hw, hl, hl));
	emitAt(b, l - w, vec4(hw, hw, hl, hl));
	EndPrimitive();
}
void segment(int a, int b) {
	if (uLineSmooth > 0.5)
		smoothSegment(a, b);
	else
		plainSegment(a, b);
}
)";

// A point as llvmpipe's legacy point rule (lp_setup_point.c): the size in whole pixels, the square of pixels round
// the centre snapped to its 1/256 pixel grid (an even size round the nearest pixel corner).
const char* const POINTS_MAIN = R"(
uniform float uPointSize;
void main() {
	vec2 v = toRaster(gl_in[0].gl_Position).xy;
	vec2 fixedPos = roundEven(v * 256.0);
	if (uFlipY > 0.5)
		fixedPos.y -= 1.0; // the bottom edge rule
	float size = max(1.0, floor((roundEven(uPointSize * 256.0) + 127.0) / 256.0));
	vec2 low;
	if (size == 1.0)
		low = floor(fixedPos / 256.0);
	else if (mod(size, 2.0) == 1.0)
		low = floor(fixedPos / 256.0) - (size - 1.0) / 2.0;
	else
		low = floor((fixedPos + 128.0) / 256.0) - size / 2.0;
	vec4 none = vec4(0.0);
	emitAt(0, low - v, none);
	emitAt(0, vec2(low.x + size, low.y) - v, none);
	emitAt(0, vec2(low.x, low.y + size) - v, none);
	emitAt(0, low + size - v, none);
	EndPrimitive();
}
)";

const char* const LINES_MAIN = R"(
void main() { segment(0, 1); }
)";

// A front face (glFrontFace(GL_CCW)) is dropped, as glCullFace(GL_FRONT) did; one of no area counts as a back face.
// Its depth offset is glPolygonOffset's with GL_POLYGON_OFFSET_LINE: the slope of the face, not of the line.
const char* const BACK_EDGES_MAIN = R"(
void main() {
	vec3 v0 = toRaster(gl_in[0].gl_Position);
	vec3 v1 = toRaster(gl_in[1].gl_Position);
	vec3 v2 = toRaster(gl_in[2].gl_Position);
	vec3 e = v0 - v2;
	vec3 f = v1 - v2;
	float det = e.x * f.y - e.y * f.x;
	if (uFlipY > 0.5 ? det < 0.0 : det > 0.0)
		return;
	if (uDepthOffset != vec2(0.0)) {
		if (det == 0.0) // Mesa's offset divides by it: an infinite depth (or none), never drawn
			return;
		float a = e.y * f.z - e.z * f.y;
		float b = e.z * f.x - e.x * f.z;
		float inv = 1.0 / det;
		gZOffset = uDepthOffset.y + max(abs(a * inv), abs(b * inv)) * uDepthOffset.x;
	}
	segment(2, 0);
	segment(0, 1);
	segment(1, 2);
}
)";

constexpr const char* RENAMED = "gs_";

// The width llvmpipe draws: a line that is not smooth is a whole number of pixels wide (rint, ties to even: 2.5 draws
// 2, 3.5 draws 4), at least one.
float plainWidth(const WideLines::LineStyle& line) { return std::max(1.f, std::rint(line.width)); }
// Half the quad of a smooth line: half a pixel wider each side for the fade, a thin one two pixels across.
float smoothHalf(const WideLines::LineStyle& line) { return line.width <= 1.f ? 1.f : 0.5f * line.width + 0.5f; }
// A 24-bit depth buffer's smallest step: glPolygonOffset's unit.
constexpr float DEPTH_STEP = 1.f / static_cast<float>((1 << 24) - 1);
} // namespace

namespace WideLines {

const char* const FRAGMENT_COVERAGE = R"(
#ifdef WIDE_LINES
uniform float uLineSmooth;
noperspective in vec4 gAa;
float lineCoverage() {
	if (uLineSmooth < 0.5)
		return 1.0;
	vec2 t = clamp(gAa.yw - abs(gAa.xz), 0.0, 1.0);
	return t.x * min(t.y, gAa.w * 2.0 - 1.0); // a line shorter than a pixel: its length
}
#else
float lineCoverage() { return 1.0; }
#endif
)";

Stage stageOf(const LineStyle& line) {
	if (line.backEdges)
		return Stage::BackEdges;
	if (std::max(1.f, std::floor((std::nearbyint(line.pointSize * 256.f) + 127.f) / 256.f)) > 1.f)
		return Stage::Points;
	if (line.smooth || plainWidth(line) > 1.f)
		return Stage::Lines;
	return Stage::None;
}

std::string vertexDefines(Stage stage, std::span<const Varying> varyings) {
	std::string out;
	if (stage == Stage::None)
		return out;
	for (const Varying& v : varyings)
		out += std::string("#define ") + v.name + " " + RENAMED + v.name + "\n";
	return out;
}

const char* fragmentDefines(Stage stage) { return stage == Stage::None ? "" : "#define WIDE_LINES\n"; }

std::string geometrySource(Stage stage, std::span<const Varying> varyings, const char* version) {
	if (stage == Stage::None)
		return {};
	std::string out = version;
	switch (stage) {
	case Stage::Lines:
		out += "layout(lines) in;\nlayout(triangle_strip, max_vertices = 4) out;\n";
		break;
	case Stage::BackEdges:
		out += "layout(triangles) in;\nlayout(triangle_strip, max_vertices = 12) out;\n";
		break;
	case Stage::Points:
	case Stage::None:
		out += "layout(points) in;\nlayout(triangle_strip, max_vertices = 4) out;\n";
		break;
	}
	std::string copy = "void passOn(int i) {\n";
	for (const Varying& v : varyings) {
		out += std::string("in ") + v.type + " " + RENAMED + v.name + "[];\n";
		out += std::string("out ") + v.type + " " + v.name + ";\n";
		copy += std::string("\t") + v.name + " = " + RENAMED + v.name + "[i];\n";
	}
	out += copy + "}\n";
	out += SEGMENT_SRC;
	out += stage == Stage::Lines ? LINES_MAIN : stage == Stage::BackEdges ? BACK_EDGES_MAIN : POINTS_MAIN;
	return out;
}

void Uniforms::find(GLuint program) {
	viewport = glGetUniformLocation(program, "uViewport");
	flipY = glGetUniformLocation(program, "uFlipY");
	halfWidth = glGetUniformLocation(program, "uLineHalf");
	smooth = glGetUniformLocation(program, "uLineSmooth");
	depthOffset = glGetUniformLocation(program, "uDepthOffset");
	pointSize = glGetUniformLocation(program, "uPointSize");
}

void Uniforms::set(const LineStyle& line) const {
	GLint view[4] = {0, 0, 1, 1};
	glGetIntegerv(GL_VIEWPORT, view);
	GLint target = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &target);
	glUniform4f(
		viewport,
		static_cast<float>(view[0]),
		static_cast<float>(view[1]),
		static_cast<float>(view[2]),
		static_cast<float>(view[3])
	);
	glUniform1f(flipY, target == 0 ? 1.f : 0.f);
	glUniform1f(halfWidth, line.smooth ? smoothHalf(line) : 0.5f * plainWidth(line));
	glUniform1f(smooth, line.smooth ? 1.f : 0.f);
	glUniform2f(depthOffset, line.offsetFactor, line.offsetUnits * 2.f * DEPTH_STEP);
	glUniform1f(pointSize, line.pointSize);
}

} // namespace WideLines
