#ifndef GL_STATE_H
#define GL_STATE_H

#include "gl_includes.h"

// The few fixed states a draw changes (plan gl-core-foundation): blend, depth test and write, culling. Replaces
// glPushAttrib / glPopAttrib (gone in a core context): read() what is set, change it for the draw, apply() the saved
// copy back. Reading asks GL, as the not yet ported code changes the states behind its back.
struct GlState {
	bool blend = false;
	GLenum blendSrc = GL_ONE, blendDst = GL_ZERO;
	bool depthTest = false;
	bool depthWrite = true;
	bool cull = false;

	[[nodiscard]] static GlState read();
	void apply() const;
};

#endif
