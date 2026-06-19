#ifndef ANI_c
#define ANI_c

#include <memory>
#include <vector>
#include "../core/timer.h"

struct VF {
	std::vector<float> v;
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
	std::vector<float> Normals;
	std::vector<float> TexCords;
	int VCount;
	std::vector<int> List;
	void Scale(float sc);
	void Translate(float x, float y, float z);

  public:
	bool bounds;
	bool loop;
	AnimatedModel();
	~AnimatedModel();
	int Load(const char FileName[]);
	void Show();
	void Advance_Animation();
	void setSpeed(int nSpeed);
	float getScale();
	void BindTexture(int t);
	void Compile();
	void Centrify();
	void Reset();
};

#endif
