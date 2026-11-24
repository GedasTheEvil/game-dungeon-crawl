#include "gl_state.h"

namespace {
void enable(GLenum cap, bool on) {
	if (on)
		glEnable(cap);
	else
		glDisable(cap);
}
} // namespace

GlState GlState::read() {
	GlState s;
	s.blend = glIsEnabled(GL_BLEND) == GL_TRUE;
	GLint src = GL_ONE;
	GLint dst = GL_ZERO;
	glGetIntegerv(GL_BLEND_SRC_RGB, &src);
	glGetIntegerv(GL_BLEND_DST_RGB, &dst);
	s.blendSrc = static_cast<GLenum>(src);
	s.blendDst = static_cast<GLenum>(dst);
	s.depthTest = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
	GLboolean write = GL_TRUE;
	glGetBooleanv(GL_DEPTH_WRITEMASK, &write);
	s.depthWrite = write == GL_TRUE;
	s.cull = glIsEnabled(GL_CULL_FACE) == GL_TRUE;
	return s;
}

void GlState::apply() const {
	enable(GL_BLEND, blend);
	glBlendFunc(blendSrc, blendDst);
	enable(GL_DEPTH_TEST, depthTest);
	glDepthMask(depthWrite ? GL_TRUE : GL_FALSE);
	enable(GL_CULL_FACE, cull);
}
