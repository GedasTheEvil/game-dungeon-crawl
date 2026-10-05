#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

#include <GL/gl.h>

// An offscreen colour + depth buffer to draw into, then use as a mipmapped texture (the journal's pages, which the
// page turn bends). Owns its GL objects: freed with it, not copied.
class RenderTarget {
  public:
	RenderTarget() = default;
	~RenderTarget();
	RenderTarget(const RenderTarget&) = delete;
	RenderTarget& operator=(const RenderTarget&) = delete;

	// Draws go to the target, its whole area is the viewport. Builds it on first use and on a size change; false if
	// the driver has no framebuffers (then nothing is bound and the caller draws without it).
	bool Begin(int width, int height);
	// Back to the window and its viewport; the texture's mipmaps are rebuilt.
	void End();
	[[nodiscard]] int TextureID() const { return static_cast<int>(colour); }

  private:
	GLuint fbo = 0, colour = 0, depth = 0;
	int w = 0, h = 0;
	GLint viewport[4] = {};
	void Release();
};

#endif
