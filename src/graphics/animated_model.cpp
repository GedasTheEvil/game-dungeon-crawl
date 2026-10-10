#define GL_GLEXT_PROTOTYPES // GL 1.5 buffer entry points, exported by libGL on Linux
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>
#include "animated_model.h"
#include "textures.h"
#include "../core/logger.h"
#include "../core/timer.h"
#include <GL/glu.h>

using namespace std;

namespace {
// The arrays of one frame: client memory, or offsets into the bound buffer. indexed: count indices of the bound
// element buffer, else count corners in order.
void drawCorners(const void* v, const void* n, const void* texCoords, int count, bool indexed) {
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, v);
	glNormalPointer(GL_FLOAT, 0, n);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
	if (indexed)
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	else
		glDrawArrays(GL_TRIANGLES, 0, count);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

// GL takes an offset into the bound buffer as a pointer.
const void* bufferOffset(size_t bytes) { return std::bit_cast<const void*>(static_cast<std::uintptr_t>(bytes)); }
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
	const auto frame = static_cast<int>(p.frame);
	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textureId));

	if (!compiled) {
		drawCorners(mesh.frames[frame].v.data(), mesh.frames[frame].n.data(), mesh.texCoords.data(), mesh.cornerCount,
					false);
		return;
	}
	const size_t frameBytes = 6 * sizeof(float) * static_cast<size_t>(vertexCount);
	const size_t v = 2 * sizeof(float) * static_cast<size_t>(vertexCount) + static_cast<size_t>(frame) * frameBytes;
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	drawCorners(bufferOffset(v), bufferOffset(v + frameBytes / 2), bufferOffset(0), mesh.cornerCount, true);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
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

	// The corners repeat the shared vertices of the triangles: one vertex per distinct corner (the same texture
	// coordinates, positions and normals in every frame), drawn through an index per corner. Software GL transforms
	// each vertex of the buffer once a draw, not each corner.
	const auto corners = static_cast<size_t>(mesh.cornerCount);
	const size_t frames = mesh.frames.size();
	std::vector<GLuint> indices(corners);
	std::vector<size_t> firstCorner; // of each vertex
	std::unordered_map<std::string, GLuint> seen;
	std::string key;
	for (size_t c = 0; c < corners; c++) {
		key.assign(reinterpret_cast<const char*>(&mesh.texCoords[2 * c]), 2 * sizeof(float));
		for (const VF& f : mesh.frames) {
			key.append(reinterpret_cast<const char*>(&f.v[3 * c]), 3 * sizeof(float));
			key.append(reinterpret_cast<const char*>(&f.n[3 * c]), 3 * sizeof(float));
		}
		const auto [it, added] = seen.try_emplace(key, static_cast<GLuint>(firstCorner.size()));
		if (added)
			firstCorner.push_back(c);
		indices[c] = it->second;
	}
	vertexCount = static_cast<int>(firstCorner.size());

	// One buffer: the texture coordinates (shared by the frames), then per frame its positions and its normals.
	const size_t vertices = firstCorner.size();
	std::vector<float> data;
	data.reserve(vertices * (2 + 6 * frames));
	for (size_t c : firstCorner)
		data.insert(data.end(), &mesh.texCoords[2 * c], &mesh.texCoords[2 * c] + 2);
	for (const VF& f : mesh.frames) {
		for (size_t c : firstCorner)
			data.insert(data.end(), &f.v[3 * c], &f.v[3 * c] + 3);
		for (size_t c : firstCorner)
			data.insert(data.end(), &f.n[3 * c], &f.n[3 * c] + 3);
	}
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.size() * sizeof(float)), data.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)), indices.data(),
				 GL_STATIC_DRAW);
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
