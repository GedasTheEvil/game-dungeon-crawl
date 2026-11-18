#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "animated_model.h"
#include "lighting.h"
#include "textures.h"
#include "gl_includes.h"
#include "matrix_stack.h"
#include "../core/logger.h"
#include "../core/timer.h"

using namespace std;

namespace {
// GL takes an offset into the bound buffer as a pointer.
const void* bufferOffset(size_t bytes) { return std::bit_cast<const void*>(static_cast<std::uintptr_t>(bytes)); }

int gAnimationRate = AnimatedModel::DEFAULT_ANIMATION_RATE;
} // namespace

AnimatedModel::AnimatedModel() {
	speed = 1;
	compiled = false;
	texture = 0;
	loop = true;
	playback.stepStart = GameClock::now();
}
AnimatedModel::~AnimatedModel() {
	if (glContextCurrent() && compiled) {
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ibo);
		glDeleteVertexArrays(1, &vao);
	}
}
int AnimatedModel::Load(const char fileName[]) { return mesh.Load(fileName) ? 1 : 0; }
void AnimatedModel::Show() const { Show(playback); }
void AnimatedModel::Show(const AnimPlayback& p) const { Show(p, texture); }
void AnimatedModel::Show(const AnimPlayback& p, int textureId) const {
	DefaultShader::Style style;
	const BatchVertex& current = Lighting::batch().currentValues(); // the world's current colour, as glColor was
	style.tint[0] = current.r;
	style.tint[1] = current.g;
	style.tint[2] = current.b;
	style.tint[3] = current.a;
	Lighting::setMetalMask(true);
	draw(p, textureId, style, Lighting::use);
	Lighting::setMetalMask(false);
}
void AnimatedModel::Show(const AnimPlayback& p, int textureId, const DefaultShader::Style& style) const {
	draw(p, textureId, style, DefaultShader::use);
}
void AnimatedModel::draw(
	const AnimPlayback& p,
	int textureId,
	const DefaultShader::Style& style,
	Program program
) const {
	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textureId));
	if (!compiled)
		return; // every model is compiled after its load
	const FrameBlend f = BlendOf(p, mesh.frameCount, loop, GameClock::now(), gAnimationRate);
	const auto vertices = static_cast<size_t>(mesh.vertexCount);
	const size_t frameBytes = 6 * sizeof(float) * vertices;
	// Key frame k's positions and normals in the buffer: after the texture coordinates, per frame its positions then
	// its normals.
	auto positions = [&](int frame) { return 2 * sizeof(float) * vertices + static_cast<size_t>(frame) * frameBytes; };
	// The blend toward the next key frame is the world shader's (the default programs draw key frame a).
	const bool blend = f.t > 0.f && f.b != f.a && Lighting::setBlend(f.t);
	if (program(style, gProj.top(), gMv.top())) {
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		const size_t a = positions(f.a);
		glVertexAttribPointer(DefaultShader::POSITION, 3, GL_FLOAT, GL_FALSE, 0, bufferOffset(a));
		glVertexAttribPointer(DefaultShader::NORMAL, 3, GL_FLOAT, GL_FALSE, 0, bufferOffset(a + frameBytes / 2));
		if (blend) {
			const size_t b = positions(f.b);
			glEnableVertexAttribArray(Lighting::NEXT_POSITION_ATTRIB);
			glEnableVertexAttribArray(Lighting::NEXT_NORMAL_ATTRIB);
			glVertexAttribPointer(Lighting::NEXT_POSITION_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, bufferOffset(b));
			glVertexAttribPointer(
				Lighting::NEXT_NORMAL_ATTRIB,
				3,
				GL_FLOAT,
				GL_FALSE,
				0,
				bufferOffset(b + frameBytes / 2)
			);
		} else {
			glDisableVertexAttribArray(Lighting::NEXT_POSITION_ATTRIB);
			glDisableVertexAttribArray(Lighting::NEXT_NORMAL_ATTRIB);
		}
		glVertexAttrib4f(DefaultShader::COLOUR, 1.f, 1.f, 1.f, 1.f); // no colour array: the tint is the colour
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
	}
	if (blend)
		Lighting::setBlend(0.f);
}
void AnimatedModel::SetAnimationRate(int perSecond) { gAnimationRate = perSecond; }
void AnimatedModel::Advance() { Advance(playback); }
void AnimatedModel::Advance(AnimPlayback& p) const { AdvancePlayback(p, mesh.frameCount, loop, speed); }
void AnimatedModel::setSpeed(int nSpeed) {
	if (nSpeed < 1)
		speed = 1;
	else
		speed = nSpeed;
}
void AnimatedModel::Compile(bool keepFrames) {
	if (compiled)
		return; // avoid too many compilations

	// One buffer: the texture coordinates (shared by the frames), then per frame its positions and its normals.
	std::vector<float> data;
	data.reserve(mesh.texCoords.size() + mesh.frames.size() * 6 * static_cast<size_t>(mesh.vertexCount));
	data.insert(data.end(), mesh.texCoords.begin(), mesh.texCoords.end());
	for (const VF& f : mesh.frames) {
		data.insert(data.end(), f.v.begin(), f.v.end());
		data.insert(data.end(), f.n.begin(), f.n.end());
	}
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.size() * sizeof(float)), data.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER,
		static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(GLuint)),
		mesh.indices.data(),
		GL_STATIC_DRAW
	);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	// The vertex array: the indices and the texture coordinates for good, the frames' arrays set per draw (Show).
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(DefaultShader::POSITION);
	glEnableVertexAttribArray(DefaultShader::NORMAL);
	glEnableVertexAttribArray(DefaultShader::TEX_COORD);
	glVertexAttribPointer(DefaultShader::TEX_COORD, 2, GL_FLOAT, GL_FALSE, 0, bufferOffset(0));
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	compiled = true;
	// The buffer holds its own copy. Frame 0 stays for the measures (YRange, HalfXZ, Vertex).
	if (!keepFrames) {
		mesh.frames.resize(1);
		mesh.frames.shrink_to_fit();
	}
}
void AnimatedModel::BindTexture(int t) { texture = t; }
void AnimatedModel::Reset() { playback.frame = 0.0; }
