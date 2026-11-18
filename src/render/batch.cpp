#include "batch.h"
#include "gl_includes.h"
#include "matrix_stack.h"

#include <cstddef>

namespace {
GLenum glMode(Primitive primitive) {
	switch (primitive) {
	case Primitive::Points:
		return GL_POINTS;
	case Primitive::Lines:
		return GL_LINES;
	case Primitive::LineStrip:
		return GL_LINE_STRIP;
	case Primitive::LineLoop:
		return GL_LINE_LOOP;
	case Primitive::TriangleStrip:
		return GL_TRIANGLE_STRIP;
	case Primitive::TriangleFan:
		return GL_TRIANGLE_FAN;
	case Primitive::Triangles:
	case Primitive::Quads:
	case Primitive::QuadStrip:
	case Primitive::Polygon:
		return GL_TRIANGLES;
	}
	return GL_TRIANGLES;
}

// A quad a, b, c, d (in GL_QUADS order) as two triangles split along a-c, in the vertex order Mesa's llvmpipe splits
// GL_QUADS in: a colour that changes across the quad shades the same, and the rasteriser rounds the same (the render
// snapshots tell; b-d or another order moves pixels by one).
void quad(
	std::vector<BatchVertex>& out,
	const BatchVertex& a,
	const BatchVertex& b,
	const BatchVertex& c,
	const BatchVertex& d
) {
	out.insert(out.end(), {a, b, c, a, c, d});
}

// The vertex array of the default programs: one interleaved BatchVertex per vertex.
void setUpAttributes() {
	const auto stride = static_cast<GLsizei>(sizeof(BatchVertex));
	auto at = [](size_t offset) {
		// NOLINTNEXTLINE(performance-no-int-to-ptr): GL takes a buffer offset as a pointer, by its API's design.
		return reinterpret_cast<const void*>(offset);
	};
	glEnableVertexAttribArray(DefaultShader::POSITION);
	glVertexAttribPointer(DefaultShader::POSITION, 3, GL_FLOAT, GL_FALSE, stride, at(offsetof(BatchVertex, x)));
	glEnableVertexAttribArray(DefaultShader::NORMAL);
	glVertexAttribPointer(DefaultShader::NORMAL, 3, GL_FLOAT, GL_FALSE, stride, at(offsetof(BatchVertex, nx)));
	glEnableVertexAttribArray(DefaultShader::COLOUR);
	glVertexAttribPointer(DefaultShader::COLOUR, 4, GL_FLOAT, GL_FALSE, stride, at(offsetof(BatchVertex, r)));
	glEnableVertexAttribArray(DefaultShader::TEX_COORD);
	glVertexAttribPointer(DefaultShader::TEX_COORD, 2, GL_FLOAT, GL_FALSE, stride, at(offsetof(BatchVertex, u)));
}

// The bindings a draw changes, put back after it: the not yet ported code around it still draws with its own program
// (the lighting shader), client arrays and buffers (plan gl-core-profile drops this).
struct Bindings {
	GLint program = 0, vao = 0, arrayBuffer = 0;
	Bindings() {
		glGetIntegerv(GL_CURRENT_PROGRAM, &program);
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
		glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
	}
	Bindings(const Bindings&) = delete;
	Bindings& operator=(const Bindings&) = delete;
	~Bindings() {
		glBindVertexArray(static_cast<GLuint>(vao));
		glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(arrayBuffer));
		glUseProgram(static_cast<GLuint>(program));
	}
};
} // namespace

void Batch::begin(Primitive p) {
	primitive = p;
	open = true;
	vertices.clear();
}

void Batch::vertex(float x, float y, float z) {
	current.x = x;
	current.y = y;
	current.z = z;
	vertices.push_back(current);
}

Primitive Batch::expand(Primitive p, std::span<const BatchVertex> in, std::vector<BatchVertex>& out) {
	out.clear();
	switch (p) {
	case Primitive::Quads:
		for (size_t i = 0; i + 3 < in.size(); i += 4)
			quad(out, in[i], in[i + 1], in[i + 2], in[i + 3]);
		return Primitive::Triangles;
	case Primitive::QuadStrip: // quad k is 2k, 2k + 1, 2k + 3, 2k + 2
		for (size_t i = 0; i + 3 < in.size(); i += 2)
			quad(out, in[i], in[i + 1], in[i + 3], in[i + 2]);
		return Primitive::Triangles;
	case Primitive::Polygon: // convex: a fan from the first vertex
		for (size_t i = 1; i + 1 < in.size(); i++)
			out.insert(out.end(), {in[0], in[i], in[i + 1]});
		return Primitive::Triangles;
	default:
		out.assign(in.begin(), in.end());
		return p;
	}
}

void Batch::end() {
	if (!open)
		return;
	open = false;
	const Primitive drawn = expand(primitive, vertices, expanded);
	if (expanded.empty())
		return;
	const Bindings saved;
	if (!(program != nullptr ? program : DefaultShader::use)(style, gProj.top(), gMv.top()))
		return;
	if (vao == 0) {
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		setUpAttributes();
	} else {
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
	}
	// A new store every draw (orphaning): the driver need not wait for the last draw from it.
	glBufferData(
		GL_ARRAY_BUFFER,
		static_cast<GLsizeiptr>(expanded.size() * sizeof(BatchVertex)),
		expanded.data(),
		GL_STREAM_DRAW
	);
	glDrawArrays(glMode(drawn), 0, static_cast<GLsizei>(expanded.size()));
}

void drawScreenQuad() {
	static GLuint vao = 0;
	static GLuint vbo = 0;
	const Bindings saved;
	if (vao == 0) {
		// The quad -1,-1 / 1,-1 / 1,1 / -1,1 split as quad() does: the same triangles the legacy GL_QUADS drew (a
		// strip moves a blurred pass's pixels by one).
		constexpr float CORNERS[] = {-1.f, -1.f, 1.f, -1.f, 1.f, 1.f, -1.f, -1.f, 1.f, 1.f, -1.f, 1.f};
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(CORNERS), CORNERS, GL_STATIC_DRAW);
		glEnableVertexAttribArray(DefaultShader::POSITION);
		glVertexAttribPointer(DefaultShader::POSITION, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	}
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}
