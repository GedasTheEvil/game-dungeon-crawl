#ifndef MATRIX_STACK_H
#define MATRIX_STACK_H

#include <vector>
#include "mat4.h"

// The CPU twin of the GL matrix stack (plan gl-core-foundation), with the same verbs: a port turns glPushMatrix()
// into gMv.push(), glTranslatef into gMv.translate, glGetFloatv(GL_MODELVIEW_MATRIX) into gMv.top(). Batch and the
// default shaders take their matrices from gMv and gProj. No GL here (the unit tests check it).
class MatrixStack {
  public:
	MatrixStack() : stack(1) {}

	void push() { stack.push_back(stack.back()); }
	// Popping the last matrix is a bug in the caller; it keeps the stack as is (glPopMatrix's stack underflow).
	void pop() {
		if (stack.size() > 1)
			stack.pop_back();
	}
	// Each multiplies the top on the right, as glTranslatef and the rest do.
	void translate(float x, float y, float z) { mult(Mat4::translation(x, y, z)); }
	void rotate(float degrees, float x, float y, float z) { mult(Mat4::rotation(degrees, x, y, z)); }
	void scale(float x, float y, float z) { mult(Mat4::scaling(x, y, z)); }
	void mult(const Mat4& m) { stack.back() = stack.back() * m; }
	void load(const Mat4& m) { stack.back() = m; }
	void loadIdentity() { stack.back() = Mat4::identity(); }

	[[nodiscard]] const Mat4& top() const { return stack.back(); }
	[[nodiscard]] size_t depth() const { return stack.size(); }

  private:
	std::vector<Mat4> stack;
};

// The model view and the projection, as GL_MODELVIEW and GL_PROJECTION.
extern MatrixStack gMv;
extern MatrixStack gProj;

#endif
