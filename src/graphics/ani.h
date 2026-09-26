#ifndef ANI_c
#define ANI_c

#include <memory>
#include <utility>
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

// Playback position of one animation. Monsters of one type share one model, so each keeps its own.
struct AnimPlayback {
	float frame = 0.0f;
	int stepStart = 0; // GameClock ms when the frame last advanced
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
	[[nodiscard]] std::pair<float, float> YRange(int f) const; // lowest and highest y of frame f
	void Reset();
	[[nodiscard]] int FrameCount() const;
	[[nodiscard]] AnimPlayback Playback() const;
	void SetPlayback(const AnimPlayback& playback);
};

#endif
