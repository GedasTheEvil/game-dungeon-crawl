#ifndef ANI_c
#define ANI_c

#include <memory>
#include <vector>
#include "../core/timer.h"

struct VF {
	std::vector<float> v; // positions, 3 floats per corner
	std::vector<float> n; // normals, 3 floats per corner
};

// Scale, then offset, that Centrify applied. Shared by the files of one monster so its walk, attack
// and die animations line up (see monster::loadModel).
struct ModelNormalization {
	float scale = 1.0f;
	float x = 0.0f, y = 0.0f, z = 0.0f;
};

class AnimatedModel {
  protected:
	std::unique_ptr<timer> frameChange;
	float frame;
	int speed;
	float scale;
	int frameC;
	int texture;
	bool compiled;
	std::vector<VF> Ver;
	std::vector<float> TexCords;
	int VCount;
	std::vector<int> List;
	void Scale(float sc);
	void Translate(float x, float y, float z);
	[[nodiscard]] const float* frameNormals(int f) const;

  public:
	bool bounds;
	bool loop;
	AnimatedModel();
	~AnimatedModel();
	int Load(const char FileName[]); // MD3 (see tools/blender/md3.py)
	void Show();
	void Advance_Animation();
	void setSpeed(int nSpeed);
	float getScale();
	void BindTexture(int t);
	void Compile();
	ModelNormalization Centrify(); // frame 0 to unit size, centred in x/z, base at y = 0
	void Normalize(const ModelNormalization& n);
	void Reset();
};

#endif
