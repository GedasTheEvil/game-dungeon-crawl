#ifndef MD3_MESH_H
#define MD3_MESH_H
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

struct VF {
	std::vector<float> v; // positions, 3 floats per vertex
	std::vector<float> n; // normals, 3 floats per vertex
};

// Scale, then offset, that Centrify applied. Shared by the files of one monster so its walk, attack
// and die animations line up (see loadClips).
struct ModelNormalization {
	float scale = 1.0f;
	float x = 0.0f, y = 0.0f, z = 0.0f;
};

// The vertex data of an MD3 file (tools/blender/md3.py writes them), without GL: the vertices of every surface once
// per frame, and the triangles as indices into them. AnimatedModel draws it; the sim reads its measures (ModelInfo).
class Md3Mesh {
  private:
	float scale = 0.0f; // largest extent of frame 0, cached (getScale)
	float getScale();

  public:
	int frameCount = 0;
	int vertexCount = 0; // per frame
	std::vector<VF> frames;
	std::vector<float> texCoords; // 2 floats per vertex, the same in every frame
	std::vector<uint32_t> indices; // 3 per triangle

	bool Load(const char fileName[]); // false (and logged): not a readable MD3 v15 file
	void Scale(float sc);
	void Translate(float x, float y, float z);
	ModelNormalization Centrify(); // frame 0 to unit size, centred in x/z, base at y = 0
	void Normalize(const ModelNormalization& n);
	[[nodiscard]] std::pair<float, float> YRange(int f) const; // lowest and highest y of frame f
	[[nodiscard]] std::pair<float, float> HalfXZ(int f) const; // farthest |x| and |z| of frame f from the origin
	[[nodiscard]] std::array<float, 3> Vertex(int f, int i) const; // vertex i of frame f
	[[nodiscard]] int TriangleCount() const { return static_cast<int>(indices.size() / 3); }
};

// The unit normal of a packed MD3 normal code: 3 floats.
const float* Md3DecodeNormal(int16_t packed);

#endif
