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

void GlLegacy::currentColour(float rgba[4]) { glGetFloatv(GL_CURRENT_COLOR, rgba); }

void GlLegacy::setLight(const DefaultShader::Light& light) {
	const float direction[4] = {light.direction.x, light.direction.y, light.direction.z, 0.f};
	const float ambient[4] = {light.ambient.x, light.ambient.y, light.ambient.z, 1.f};
	const float diffuse[4] = {light.diffuse.x, light.diffuse.y, light.diffuse.z, 1.f};
	const float none[4] = {0.f, 0.f, 0.f, 1.f};
	glLightfv(GL_LIGHT0, GL_POSITION, direction);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, none);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, none);
}

void GlLegacy::lighting(bool on) {
	if (on) {
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_NORMALIZE);
		glEnable(GL_COLOR_MATERIAL);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	} else {
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
		glDisable(GL_NORMALIZE);
		glDisable(GL_COLOR_MATERIAL);
	}
}

void GlLegacy::texCoord(float u, float v) { glTexCoord2f(u, v); }
