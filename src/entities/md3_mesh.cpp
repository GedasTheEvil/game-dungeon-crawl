#include "md3_mesh.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include "../core/logger.h"

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

} // namespace

// 16-bit lat/long normal: high byte azimuth, low byte polar angle, 255 steps per full turn. Every code is looked up
// in a table, built once (thread safe: a function-local static).
const float* Md3DecodeNormal(int16_t packed) {
	static const vector<float> TABLE = [] {
		vector<float> t(size_t{3} * 65536);
		const float step = 2.0f * static_cast<float>(M_PI) / 255.0f;
		for (size_t code = 0; code < 65536; code++) {
			const float azimuth = static_cast<float>(code >> 8) * step;
			const float polar = static_cast<float>(code & 0xFF) * step;
			t[3 * code] = cosf(azimuth) * sinf(polar);
			t[3 * code + 1] = sinf(azimuth) * sinf(polar);
			t[3 * code + 2] = cosf(polar);
		}
		return t;
	}();
	return &TABLE[3 * static_cast<size_t>(static_cast<uint16_t>(packed))];
}

//============================================================
bool Md3Mesh::Load(const char fileName[]) {
	ifstream in(fileName, ios::binary);
	vector<char> data((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());

	auto fail = [&](const char* why) {
		LOG_ERRORF("model", "Cannot load %s: %s", fileName, why);
		vertexCount = 0;
		indices.clear();
		return false;
	};

	Md3Header header{};
	if (!readAt(data, 0, header) || memcmp(header.ident, "IDP3", 4) != 0 || header.version != 15)
		return fail("not an MD3 v15 file");
	if (header.numFrames < 1 || header.numSurfaces < 1)
		return fail("no frames or surfaces");

	frameCount = header.numFrames;
	const float xyzScale = MD3_XYZ_SCALE * headerUnit(header.name);
	vertexCount = 0;
	frames.assign(frameCount, VF{});
	texCoords.clear();
	indices.clear();

	// Exact sizes up front: grown by push_back, every frame's arrays held up to twice their size.
	size_t vertices = 0, corners = 0;
	for (size_t s = 0, ofs = static_cast<size_t>(header.ofsSurfaces); s < static_cast<size_t>(header.numSurfaces);
		 s++) {
		Md3Surface surf{};
		if (!readAt(data, ofs, surf) || surf.ofsEnd <= 0)
			break; // the loop below reports it
		vertices += static_cast<size_t>(max(surf.numVerts, 0));
		corners += static_cast<size_t>(max(surf.numTriangles, 0)) * 3;
		ofs += static_cast<size_t>(surf.ofsEnd);
	}
	texCoords.reserve(vertices * 2);
	indices.reserve(corners);
	for (VF& frame : frames) {
		frame.v.reserve(vertices * 3);
		frame.n.reserve(vertices * 3);
	}

	// The surfaces one after another: their vertices, and their triangles' indices offset by the surface's first
	// vertex. Equal vertices of two surfaces (a seam) stay apart.
	size_t surfaceOfs = static_cast<size_t>(header.ofsSurfaces);
	for (int s = 0; s < header.numSurfaces; s++) {
		Md3Surface surf{};
		if (!readAt(data, surfaceOfs, surf) || memcmp(surf.ident, "IDP3", 4) != 0 || surf.ofsEnd <= 0)
			return fail("bad surface header");
		if (surf.numFrames != frameCount)
			return fail("surface frame count differs from the model's");
		if (surf.numVerts < 0 || surf.numTriangles < 0)
			return fail("bad surface header");

		const auto first = static_cast<uint32_t>(vertexCount);
		for (int t = 0; t < surf.numTriangles; t++) {
			int32_t corner[3];
			if (!readAt(
					data,
					surfaceOfs + static_cast<size_t>(surf.ofsTriangles) + static_cast<size_t>(t) * sizeof(corner),
					corner
				))
				return fail("truncated triangles");
			for (int32_t v : corner) {
				if (v < 0 || v >= surf.numVerts)
					return fail("triangle index out of range");
				indices.push_back(first + static_cast<uint32_t>(v));
			}
		}

		for (int v = 0; v < surf.numVerts; v++) {
			float st[2];
			if (!readAt(data, surfaceOfs + static_cast<size_t>(surf.ofsSt) + static_cast<size_t>(v) * sizeof(st), st))
				return fail("truncated texture coordinates");
			texCoords.push_back(st[0]);
			texCoords.push_back(1.0f - st[1]);
		}

		for (int f = 0; f < frameCount; f++) {
			const size_t at = surfaceOfs + static_cast<size_t>(surf.ofsXyzNormal) +
							  static_cast<size_t>(f) * static_cast<size_t>(surf.numVerts) * sizeof(Md3Vertex);
			for (int v = 0; v < surf.numVerts; v++) {
				Md3Vertex mv{};
				if (!readAt(data, at + static_cast<size_t>(v) * sizeof(Md3Vertex), mv))
					return fail("truncated vertices");
				frames[f].v.push_back(static_cast<float>(mv.x) * xyzScale);
				frames[f].v.push_back(static_cast<float>(mv.y) * xyzScale);
				frames[f].v.push_back(static_cast<float>(mv.z) * xyzScale);
				const float* n = Md3DecodeNormal(mv.normal);
				frames[f].n.insert(frames[f].n.end(), n, n + 3);
			}
		}
		vertexCount += surf.numVerts;
		surfaceOfs += static_cast<size_t>(surf.ofsEnd);
	}

	return true;
}
//============================================================
// Largest extent of frame 0, cached.
float Md3Mesh::getScale() {
	if (fabs(scale) > 0.00000000001)
		return scale;

	float minX = 1000.0, maxX = -1000.0;
	float minY = 1000.0, maxY = -1000.0;
	float minZ = 1000.0, maxZ = -1000.0;
	const std::vector<float>& v = frames[0].v;
	for (int i = 0; i < vertexCount * 3; i += 3) {
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
ModelNormalization Md3Mesh::Centrify() {
	float scale = 1 / getScale();
	Scale(scale);

	float minX = 1000.0, maxX = -1000.0;
	float minY = 1000.0, maxY = -1000.0;
	float minZ = 1000.0, maxZ = -1000.0;

	// find maximum dimensions of the model
	for (int i = 0; i < vertexCount * 3; i += 3) {
		if (frames[0].v[i] > maxX)
			maxX = frames[0].v[i];
		if (frames[0].v[i] < minX)
			minX = frames[0].v[i];

		if (frames[0].v[i + 1] > maxY)
			maxY = frames[0].v[i + 1];
		if (frames[0].v[i + 1] < minY)
			minY = frames[0].v[i + 1];

		if (frames[0].v[i + 2] > maxZ)
			maxZ = frames[0].v[i + 2];
		if (frames[0].v[i + 2] < minZ)
			minZ = frames[0].v[i + 2];
	}

	ModelNormalization applied{scale, -(minX + maxX) / 2, -minY, -(maxZ + minZ) / 2};
	Translate(applied.x, applied.y, applied.z); // apacia bus 0, x centruojam, z 0
	return applied;
}
//============================================================
void Md3Mesh::Normalize(const ModelNormalization& n) {
	Scale(n.scale);
	Translate(n.x, n.y, n.z);
}
//============================================================
std::array<float, 3> Md3Mesh::Vertex(int f, int i) const {
	const std::vector<float>& v = frames[f].v;
	const size_t at = 3 * static_cast<size_t>(i);
	return {v[at], v[at + 1], v[at + 2]};
}
//============================================================
std::pair<float, float> Md3Mesh::YRange(int f) const {
	std::pair<float, float> range{1000.0f, -1000.0f};
	for (int i = 1; i < vertexCount * 3; i += 3) {
		range.first = std::min(range.first, frames[f].v[i]);
		range.second = std::max(range.second, frames[f].v[i]);
	}
	return range;
}
//============================================================
std::pair<float, float> Md3Mesh::HalfXZ(int f) const {
	std::pair<float, float> half{0.0f, 0.0f};
	for (int i = 0; i < vertexCount * 3; i += 3) {
		half.first = std::max(half.first, std::fabs(frames[f].v[i]));
		half.second = std::max(half.second, std::fabs(frames[f].v[i + 2]));
	}
	return half;
}
//============================================================
void Md3Mesh::Translate(float x, float y, float z) {
	for (VF& frame : frames) {
		for (int i = 0; i < vertexCount * 3; i += 3) {
			frame.v[i] += x;
			frame.v[i + 1] += y;
			frame.v[i + 2] += z;
		}
	}
}
//============================================================
void Md3Mesh::Scale(float sc) {
	for (VF& frame : frames)
		for (int i = 0; i < vertexCount * 3; i++)
			frame.v[i] *= sc;
}
//============================================================
