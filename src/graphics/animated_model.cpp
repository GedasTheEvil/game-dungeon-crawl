#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "animated_model.h"
#include "textures.h"
#include "../core/logger.h"
#include "../core/timer.h"
#include <GL/glu.h>

using namespace std;

namespace {
// Quake 3 MD3 layout (little endian). Game conventions (Y-up coordinates, flipped t, normals in
// every frame) are described in tools/blender/md3.py, which writes these files.
struct Md3Header {
	char ident[4];
	int32_t version;
	char name[64];
	int32_t flags, numFrames, numTags, numSurfaces, numSkins, ofsFrames, ofsTags, ofsSurfaces, ofsEnd;
};

struct Md3Surface {
	char ident[4];
	char name[64];
	int32_t flags, numFrames, numShaders, numVerts, numTriangles, ofsTriangles, ofsShaders, ofsSt, ofsXyzNormal, ofsEnd;
};

struct Md3Vertex {
	int16_t x, y, z, normal;
};

static_assert(sizeof(Md3Header) == 108 && sizeof(Md3Surface) == 108 && sizeof(Md3Vertex) == 8, "MD3 layout");
constexpr float MD3_XYZ_SCALE = 1.0f / 64.0f;

// Each file is stored scaled to fill the int16 range; the header name "<name>;unit=<float>" holds the
// factor back to the original units, so the walk/attack/die files of a model share one scale.
float headerUnit(const char (&name)[64]) {
	string text(name, strnlen(name, sizeof(name)));
	size_t at = text.find(";unit=");
	if (at == string::npos)
		return 1.0f;
	float unit = strtof(text.c_str() + at + 6, nullptr);
	return unit > 0.0f ? unit : 1.0f;
}

// Copies a T from data at offset; false if it would read past the end of the file.
template <typename T> bool readAt(const vector<char>& data, size_t offset, T& out) {
	if (offset > data.size() || data.size() - offset < sizeof(T))
		return false;
	memcpy(&out, data.data() + offset, sizeof(T));
	return true;
}

// 16-bit lat/long normal: high byte azimuth, low byte polar angle, 255 steps per full turn.
void decodeNormal(int16_t packed, float* out) {
	auto code = static_cast<uint16_t>(packed);
	float step = 2.0f * static_cast<float>(M_PI) / 255.0f;
	float azimuth = static_cast<float>(code >> 8) * step;
	float polar = static_cast<float>(code & 0xFF) * step;
	out[0] = cosf(azimuth) * sinf(polar);
	out[1] = sinf(azimuth) * sinf(polar);
	out[2] = cosf(polar);
}
} // namespace

//============================================================
AnimatedModel::AnimatedModel() {
	VCount = 0;
	speed = 1;
	scale = 0.0f;
	frameC = 0;
	compiled = false;
	texture = 0;
	loop = true;
	playback.stepStart = GameClock::now();
}
////============================================================
AnimatedModel::~AnimatedModel() {
	if (glContextCurrent())
		for (int list : List)
			glDeleteLists(static_cast<GLuint>(list), 1);
}
//============================================================
int AnimatedModel::Load(const char fileName[]) {
	ifstream in(fileName, ios::binary);
	vector<char> data((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());

	auto fail = [&](const char* why) {
		LOG_ERRORF("model", "Cannot load %s: %s", fileName, why);
		VCount = 0;
		return 0;
	};

	Md3Header header{};
	if (!readAt(data, 0, header) || memcmp(header.ident, "IDP3", 4) != 0 || header.version != 15)
		return fail("not an MD3 v15 file");
	if (header.numFrames < 1 || header.numSurfaces < 1)
		return fail("no frames or surfaces");

	frameC = header.numFrames;
	const float xyzScale = MD3_XYZ_SCALE * headerUnit(header.name);
	VCount = 0;
	Ver.assign(frameC, VF{});
	TexCords.clear();

	// Exact sizes up front: grown by push_back, every frame's arrays held up to twice their size.
	size_t corners = 0;
	for (size_t s = 0, ofs = static_cast<size_t>(header.ofsSurfaces); s < static_cast<size_t>(header.numSurfaces);
		 s++) {
		Md3Surface surf{};
		if (!readAt(data, ofs, surf) || surf.ofsEnd <= 0)
			break; // the loop below reports it
		corners += static_cast<size_t>(surf.numTriangles) * 3;
		ofs += static_cast<size_t>(surf.ofsEnd);
	}
	TexCords.reserve(corners * 2);
	for (VF& frame : Ver) {
		frame.v.reserve(corners * 3);
		frame.n.reserve(corners * 3);
	}

	// Expand every surface's indexed triangles into the per-corner arrays the renderer draws.
	size_t surfaceOfs = static_cast<size_t>(header.ofsSurfaces);
	for (int s = 0; s < header.numSurfaces; s++) {
		Md3Surface surf{};
		if (!readAt(data, surfaceOfs, surf) || memcmp(surf.ident, "IDP3", 4) != 0 || surf.ofsEnd <= 0)
			return fail("bad surface header");
		if (surf.numFrames != frameC)
			return fail("surface frame count differs from the model's");

		for (int t = 0; t < surf.numTriangles; t++) {
			int32_t corners[3];
			if (!readAt(data,
						surfaceOfs + static_cast<size_t>(surf.ofsTriangles) + static_cast<size_t>(t) * sizeof(corners),
						corners))
				return fail("truncated triangles");

			for (int32_t v : corners) {
				if (v < 0 || v >= surf.numVerts)
					return fail("triangle index out of range");

				float st[2];
				if (!readAt(data, surfaceOfs + static_cast<size_t>(surf.ofsSt) + static_cast<size_t>(v) * sizeof(st),
							st))
					return fail("truncated texture coordinates");
				TexCords.push_back(st[0]);
				TexCords.push_back(1.0f - st[1]);

				for (int f = 0; f < frameC; f++) {
					Md3Vertex mv{};
					size_t index = static_cast<size_t>(f) * static_cast<size_t>(surf.numVerts) + static_cast<size_t>(v);
					if (!readAt(data, surfaceOfs + static_cast<size_t>(surf.ofsXyzNormal) + index * sizeof(Md3Vertex),
								mv))
						return fail("truncated vertices");
					Ver[f].v.push_back(static_cast<float>(mv.x) * xyzScale);
					Ver[f].v.push_back(static_cast<float>(mv.y) * xyzScale);
					Ver[f].v.push_back(static_cast<float>(mv.z) * xyzScale);
					float n[3];
					decodeNormal(mv.normal, n);
					Ver[f].n.insert(Ver[f].n.end(), n, n + 3);
				}
			}
		}
		VCount += surf.numTriangles * 3;
		surfaceOfs += static_cast<size_t>(surf.ofsEnd);
	}

	return 1;
}
//============================================================
void AnimatedModel::Show() const { Show(playback); }
//============================================================
void AnimatedModel::Show(const AnimPlayback& p) const { Show(p, texture); }
//============================================================
void AnimatedModel::Show(const AnimPlayback& p, int textureId) const {
	const auto frame = static_cast<int>(p.frame);
	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textureId));

	if (!compiled) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, Ver[frame].v.data());
		glNormalPointer(GL_FLOAT, 0, frameNormals(frame));
		glTexCoordPointer(2, GL_FLOAT, 0, TexCords.data());
		glDrawArrays(GL_TRIANGLES, 0, VCount);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
	} else
		glCallList(List[frame]);
}
//============================================================
//============================================================
void AnimatedModel::Advance() { Advance(playback); }
//============================================================
void AnimatedModel::Advance(AnimPlayback& p) const {
	if (frameC == 1)
		return;

	const int now = GameClock::now();
	if (now - p.stepStart < FRAME_STEP_MS)
		return;
	p.stepStart = now;

	const auto frames = static_cast<float>(frameC);
	const float step = 0.04f * static_cast<float>(speed);
	if (!loop && p.frame < frames)
		p.frame += step;

	if (!loop && p.frame >= frames - 1)
		p.frame = frames - 1;

	if (loop)
		p.frame += step;

	if (loop && p.frame >= (frameC - 0.2))
		p.frame = 0.0;
}
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

	List.resize(frameC);

	for (int i = 0; i < frameC; i++) {
		List[i] = static_cast<int>(glGenLists(1));

		glBindTexture(GL_TEXTURE_2D, texture);

		glNewList(List[i], GL_COMPILE);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, Ver[i].v.data());
		glNormalPointer(GL_FLOAT, 0, frameNormals(i));
		glTexCoordPointer(2, GL_FLOAT, 0, TexCords.data());
		glDrawArrays(GL_TRIANGLES, 0, VCount /*/divisor*/);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);

		glEndList();
	}

	compiled = true;
	// The lists hold their own copy. Frame 0 stays for the measures (YRange, HalfXZ, Vertex).
	if (!keepFrames) {
		Ver.resize(1);
		Ver.shrink_to_fit();
	}
}
//============================================================
void AnimatedModel::BindTexture(int t) { texture = t; }
//============================================================
// Largest extent of frame 0, cached.
float AnimatedModel::getScale() {
	if (fabs(scale) > 0.00000000001)
		return scale;

	float minX = 1000.0, maxX = -1000.0;
	float minY = 1000.0, maxY = -1000.0;
	float minZ = 1000.0, maxZ = -1000.0;
	const std::vector<float>& v = Ver[0].v;
	for (int i = 0; i < VCount * 3; i += 3) {
		maxX = std::max(maxX, v[i]);
		minX = std::min(minX, v[i]);
		maxY = std::max(maxY, v[i + 1]);
		minY = std::min(minY, v[i + 1]);
		maxZ = std::max(maxZ, v[i + 2]);
		minZ = std::min(minZ, v[i + 2]);
	}

	float scX = maxX - minX;
	float scY = maxY - minY;
	float scZ = maxZ - minZ;
	if (scX > scY && scX > scZ)
		scale = scX;
	else if (scY > scX && scY > scZ)
		scale = scY;
	else
		scale = scZ;
	return scale;
}
//============================================================
ModelNormalization AnimatedModel::Centrify() {
	float scale = 1 / getScale();
	Scale(scale);

	float minX = 1000.0, maxX = -1000.0;
	float minY = 1000.0, maxY = -1000.0;
	float minZ = 1000.0, maxZ = -1000.0;

	// find maximum dimensions of the model
	for (int i = 0; i < VCount * 3; i += 3) {
		if (Ver[0].v[i] > maxX)
			maxX = Ver[0].v[i];
		if (Ver[0].v[i] < minX)
			minX = Ver[0].v[i];

		if (Ver[0].v[i + 1] > maxY)
			maxY = Ver[0].v[i + 1];
		if (Ver[0].v[i + 1] < minY)
			minY = Ver[0].v[i + 1];

		if (Ver[0].v[i + 2] > maxZ)
			maxZ = Ver[0].v[i + 2];
		if (Ver[0].v[i + 2] < minZ)
			minZ = Ver[0].v[i + 2];
	}

	ModelNormalization applied{scale, -(minX + maxX) / 2, -minY, -(maxZ + minZ) / 2};
	Translate(applied.x, applied.y, applied.z); // apacia bus 0, x centruojam, z 0
	return applied;
}
//============================================================
void AnimatedModel::Normalize(const ModelNormalization& n) {
	Scale(n.scale);
	Translate(n.x, n.y, n.z);
}
//============================================================
std::array<float, 3> AnimatedModel::Vertex(int f, int i) const {
	const std::vector<float>& v = Ver[f].v;
	const size_t at = 3 * static_cast<size_t>(i);
	return {v[at], v[at + 1], v[at + 2]};
}
//============================================================
std::pair<float, float> AnimatedModel::YRange(int f) const {
	std::pair<float, float> range{1000.0f, -1000.0f};
	for (int i = 1; i < VCount * 3; i += 3) {
		range.first = std::min(range.first, Ver[f].v[i]);
		range.second = std::max(range.second, Ver[f].v[i]);
	}
	return range;
}
//============================================================
std::pair<float, float> AnimatedModel::HalfXZ(int f) const {
	std::pair<float, float> half{0.0f, 0.0f};
	for (int i = 0; i < VCount * 3; i += 3) {
		half.first = std::max(half.first, std::fabs(Ver[f].v[i]));
		half.second = std::max(half.second, std::fabs(Ver[f].v[i + 2]));
	}
	return half;
}
//============================================================
void AnimatedModel::Translate(float x, float y, float z) {
	for (VF& frame : Ver) {
		for (int i = 0; i < VCount * 3; i += 3) {
			frame.v[i] += x;
			frame.v[i + 1] += y;
			frame.v[i + 2] += z;
		}
	}
}
//============================================================
void AnimatedModel::Scale(float sc) {
	for (VF& frame : Ver)
		for (int i = 0; i < VCount * 3; i++)
			frame.v[i] *= sc;
}
//============================================================
void AnimatedModel::Reset() { playback.frame = 0.0; }
//============================================================
int AnimatedModel::FrameCount() const { return frameC; }
//============================================================
const float* AnimatedModel::frameNormals(int f) const { return Ver[f].n.data(); }
//============================================================
