#ifndef ANIMATED_MODEL_H
#define ANIMATED_MODEL_H
#include <array>
#include <memory>
#include <utility>
#include <vector>
#include "../core/timer.h"
#include "../entities/md3_mesh.h"
#include "../entities/model_info.h"

class AnimatedModel {
  protected:
	AnimPlayback playback; // of Show() / Advance() without an argument
	int speed;
	int texture;
	bool compiled;
	Md3Mesh mesh;
	unsigned vbo = 0;	 // the frames' distinct vertices, after Compile
	unsigned ibo = 0;	 // a vertex per corner
	int vertexCount = 0; // in vbo, per frame

  public:
	bool loop;
	AnimatedModel();
	~AnimatedModel();
	AnimatedModel(const AnimatedModel&) = delete; // owns its buffers
	AnimatedModel& operator=(const AnimatedModel&) = delete;
	int Load(const char filename[]); // MD3 (see tools/blender/md3.py)
	void Show() const;
	void Show(const AnimPlayback& p) const;				   // a shared model: the caller keeps the playback
	void Show(const AnimPlayback& p, int textureId) const; // with another texture on the same UVs
	void Advance();
	void Advance(AnimPlayback& p) const;
	void setSpeed(int nSpeed);
	void BindTexture(int t);
	// Draws from a vertex buffer from now on and frees frames 1.. of the vertex data, unless keepFrames (Vertex of
	// any frame).
	void Compile(bool keepFrames = false);
	// A mesh already loaded (loadClips): its frames are taken over.
	void Adopt(Md3Mesh&& loaded) { mesh = std::move(loaded); }
	ModelNormalization Centrify() { return mesh.Centrify(); } // frame 0 to unit size, centred in x/z, base at y = 0
	void Normalize(const ModelNormalization& n) { mesh.Normalize(n); }
	// Lowest and highest y of frame f.
	[[nodiscard]] std::pair<float, float> YRange(int f) const { return mesh.YRange(f); }
	// Farthest |x| and |z| of frame f from the origin.
	[[nodiscard]] std::pair<float, float> HalfXZ(int f) const { return mesh.HalfXZ(f); }
	[[nodiscard]] int VertexCount() const { return mesh.cornerCount; }
	// Corner i of frame f (see Compile).
	[[nodiscard]] std::array<float, 3> Vertex(int f, int i) const { return mesh.Vertex(f, i); }
	void Reset();
	[[nodiscard]] int FrameCount() const { return mesh.frameCount; }
};

#endif
