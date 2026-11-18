#ifndef BATCH_H
#define BATCH_H

#include <cstdint>
#include <span>
#include <vector>
#include "default_shaders.h"

// Immediate mode for the GL core path (plan gl-core-foundation). Same shape as glBegin: begin(primitive), then
// colour, texCoord, normal (each kept for the vertices after it, also across begin and end, as GL's current values)
// and vertex; end() uploads the vertices to the batch's streaming buffer and draws them with the default program of
// `style`, under gProj and gMv's tops. Quads, quad strips and polygons become triangles here, so call sites keep them.
// A port: glBegin -> batch.begin, glVertex3f -> batch.vertex, glColor4f -> batch.colour, glEnable(GL_TEXTURE_2D) ->
// style.kind. Blend, depth and cull stay GL state (GlState).

enum class Primitive : std::uint8_t {
	Points,
	Lines,
	LineStrip,
	LineLoop,
	Triangles,
	TriangleStrip,
	TriangleFan,
	Quads,
	QuadStrip,
	Polygon,
};

struct BatchVertex {
	float x = 0.f, y = 0.f, z = 0.f;
	float nx = 0.f, ny = 0.f, nz = 1.f;
	float r = 1.f, g = 1.f, b = 1.f, a = 1.f;
	float u = 0.f, v = 0.f;
};

class Batch {
  public:
	// Long-lived (a file's static): its vertex array and buffer, made at the first draw, go with the GL context.
	Batch() = default;
	~Batch() = default;
	Batch(const Batch&) = delete;
	Batch& operator=(const Batch&) = delete;

	DefaultShader::Style style;

	void begin(Primitive primitive);
	void vertex(float x, float y, float z = 0.f);
	void colour(float r, float g, float b, float a = 1.f) {
		current.r = r, current.g = g, current.b = b, current.a = a;
	}
	void texCoord(float u, float v) { current.u = u, current.v = v; }
	void normal(float x, float y, float z) { current.nx = x, current.ny = y, current.nz = z; }
	void end();
	// The vertices since begin, not yet drawn (a test reads them).
	[[nodiscard]] std::span<const BatchVertex> pending() const { return vertices; }

	// The vertices of `in`, drawn as `primitive`: what end() uploads (out) and the primitive it draws them as. A quad
	// a, b, c, d (also each of a quad strip's) becomes a, b, c and a, c, d, as Mesa splits GL_QUADS; a polygon fans
	// from its first vertex.
	static Primitive expand(Primitive primitive, std::span<const BatchVertex> in, std::vector<BatchVertex>& out);

  private:
	BatchVertex current;
	Primitive primitive = Primitive::Triangles;
	bool open = false;
	std::vector<BatchVertex> vertices;
	std::vector<BatchVertex> expanded;
	unsigned vao = 0, vbo = 0;
};

// A clip-space quad over the whole viewport (-1..1), its corners at attribute 0, for a full-screen pass with its own
// program (brightness, ink, motion effects): their vertex shaders take the uv from the position.
void drawScreenQuad();

#endif
