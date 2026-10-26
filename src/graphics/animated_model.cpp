#define GL_GLEXT_PROTOTYPES // GL 1.5 buffer entry points, exported by libGL on Linux
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "animated_model.h"
#include "lighting.h"
#include "textures.h"
#include <GL/glext.h>
#include "../core/logger.h"
#include "../core/timer.h"
#include <GL/glu.h>

using namespace std;

namespace {
// The arrays of one frame and the triangles' indices: client memory, or offsets into the bound buffers.
void drawTriangles(const void* v, const void* n, const void* texCoords, const void* indices, int count) {
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, v);
	glNormalPointer(GL_FLOAT, 0, n);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
	glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, indices);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

// GL takes an offset into the bound buffer as a pointer.
const void* bufferOffset(size_t bytes) { return std::bit_cast<const void*>(static_cast<std::uintptr_t>(bytes)); }

// One key frame's positions and normals: client memory, or offsets into the bound buffer.
struct FrameArrays {
	const void* v;
	const void* n;
};

int gAnimationRate = AnimatedModel::DEFAULT_ANIMATION_RATE;
} // namespace

//============================================================
AnimatedModel::AnimatedModel() {
	speed = 1;
	compiled = false;
	texture = 0;
	loop = true;
	playback.stepStart = GameClock::now();
}
////============================================================
AnimatedModel::~AnimatedModel() {
	if (glContextCurrent() && compiled) {
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ibo);
	}
}
//============================================================
int AnimatedModel::Load(const char fileName[]) { return mesh.Load(fileName) ? 1 : 0; }
//============================================================
void AnimatedModel::Show() const { Show(playback); }
//============================================================
void AnimatedModel::Show(const AnimPlayback& p) const { Show(p, texture); }
//============================================================
void AnimatedModel::Show(const AnimPlayback& p, int textureId) const {
	const FrameBlend f = BlendOf(p, mesh.frameCount, loop, GameClock::now(), gAnimationRate);
	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textureId));

	// The positions and normals of key frames a and b, the texture coordinates, the indices.
	FrameArrays a{};
	FrameArrays b{};
	const void* texCoords = mesh.texCoords.data();
	const void* indices = mesh.indices.data();
	if (compiled) {
		const auto vertices = static_cast<size_t>(mesh.vertexCount);
		const size_t frameBytes = 6 * sizeof(float) * vertices;
		auto arrays = [&](int frame) {
			const size_t v = 2 * sizeof(float) * vertices + static_cast<size_t>(frame) * frameBytes;
			return FrameArrays{bufferOffset(v), bufferOffset(v + frameBytes / 2)};
		};
		a = arrays(f.a);
		b = arrays(f.b);
		texCoords = bufferOffset(0);
		indices = nullptr;
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	} else {
		a = {mesh.frames[f.a].v.data(), mesh.frames[f.a].n.data()};
		b = {mesh.frames[f.b].v.data(), mesh.frames[f.b].n.data()};
	}
	const bool blend = f.t > 0.f && f.b != f.a && Lighting::setBlend(f.t);
	if (blend) {
		glEnableVertexAttribArray(Lighting::NEXT_POSITION_ATTRIB);
		glEnableVertexAttribArray(Lighting::NEXT_NORMAL_ATTRIB);
		glVertexAttribPointer(Lighting::NEXT_POSITION_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, b.v);
		glVertexAttribPointer(Lighting::NEXT_NORMAL_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, b.n);
	}
	drawTriangles(a.v, a.n, texCoords, indices, static_cast<int>(mesh.indices.size()));
	if (blend) {
		glDisableVertexAttribArray(Lighting::NEXT_POSITION_ATTRIB);
		glDisableVertexAttribArray(Lighting::NEXT_NORMAL_ATTRIB);
		Lighting::setBlend(0.f);
	}
	if (compiled) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}
//============================================================
void AnimatedModel::SetAnimationRate(int perSecond) { gAnimationRate = perSecond; }
//============================================================
//============================================================
void AnimatedModel::Advance() { Advance(playback); }
//============================================================
void AnimatedModel::Advance(AnimPlayback& p) const { AdvancePlayback(p, mesh.frameCount, loop, speed); }
//============================================================
void AnimatedModel::setSpeed(int nSpeed) {
	if (nSpeed < 1)
		speed = 1;
	else
		speed = nSpeed;
}
//============================================================
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

	compiled = true;
	// The buffer holds its own copy. Frame 0 stays for the measures (YRange, HalfXZ, Vertex).
	if (!keepFrames) {
		mesh.frames.resize(1);
		mesh.frames.shrink_to_fit();
	}
}
//============================================================
void AnimatedModel::BindTexture(int t) { texture = t; }
//============================================================
void AnimatedModel::Reset() { playback.frame = 0.0; }
//============================================================
