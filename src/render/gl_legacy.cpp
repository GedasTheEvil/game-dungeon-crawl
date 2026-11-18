#include "gl_legacy.h"
#include "gl_includes.h"
#include "matrix_stack.h"

void GlLegacy::syncMatrices() {
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(gProj.top().data());
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(gMv.top().data());
}

void GlLegacy::adoptMatrices() {
	Mat4 m;
	glGetFloatv(GL_PROJECTION_MATRIX, m.m.data());
	gProj.load(m);
	glGetFloatv(GL_MODELVIEW_MATRIX, m.m.data());
	gMv.load(m);
}

void GlLegacy::resetColour() { glColor4f(1.f, 1.f, 1.f, 1.f); }

void GlLegacy::colour(float r, float g, float b, float a) { glColor4f(r, g, b, a); }

void GlLegacy::texturing(bool on) {
	if (on)
		glEnable(GL_TEXTURE_2D);
	else
		glDisable(GL_TEXTURE_2D);
}
