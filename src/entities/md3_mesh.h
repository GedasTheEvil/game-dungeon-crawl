#ifndef MD3_MESH_H
#define MD3_MESH_H
#include <array>
#include <utility>
#include <vector>

struct VF {
	std::vector<float> v; // positions, 3 floats per corner
	std::vector<float> n; // normals, 3 floats per corner
};

// Scale, then offset, that Centrify applied. Shared by the files of one monster so its walk, attack
// and die animations line up (see loadClips).
struct ModelNormalization {
	float scale = 1.0f;
	float x = 0.0f, y = 0.0f, z = 0.0f;
};

// The vertex data of an MD3 file (tools/blender/md3.py writes them), without GL: the indexed triangles expanded into
// per-corner arrays, one set per frame. AnimatedModel draws it; the sim reads its measures (ModelInfo).
class Md3Mesh {
  private:
	float scale = 0.0f; // largest extent of frame 0, cached (getScale)
	float getScale();

  public:
	int frameCount = 0;
	int cornerCount = 0; // 3 per triangle
	std::vector<VF> frames;
	std::vector<float> texCoords;

	bool Load(const char fileName[]); // false (and logged): not a readable MD3 v15 file
	void Scale(float sc);
	void Translate(float x, float y, float z);
	ModelNormalization Centrify(); // frame 0 to unit size, centred in x/z, base at y = 0
	void Normalize(const ModelNormalization& n);
	[[nodiscard]] std::pair<float, float> YRange(int f) const;	   // lowest and highest y of frame f
	[[nodiscard]] std::pair<float, float> HalfXZ(int f) const;	   // farthest |x| and |z| of frame f from the origin
	[[nodiscard]] std::array<float, 3> Vertex(int f, int i) const; // corner i of frame f
};

#endif
