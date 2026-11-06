#define GL_GLEXT_PROTOTYPES // GL 3.0 framebuffer entry points, exported by libGL on Linux
#include "render_target.h"
#include "textures.h"

#include <GL/glext.h>
#include "../core/logger.h"

RenderTarget::~RenderTarget() {
	if (glContextCurrent())
		Release();
}

void RenderTarget::Release() {
	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &depth);
	glDeleteTextures(1, &colour);
	fbo = depth = colour = 0;
	w = h = 0;
}

bool RenderTarget::Begin(int width, int height) {
	if (width <= 0 || height <= 0)
		return false;
	if (fbo == 0 || width != w || height != h) {
		Release();
		glGenTextures(1, &colour);
		glBindTexture(GL_TEXTURE_2D, colour);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		glGenRenderbuffers(1, &depth);
		glBindRenderbuffer(GL_RENDERBUFFER, depth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colour, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
		bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		if (!ok) {
			LOG_ERRORF("graphics", "Render target %dx%d incomplete", width, height);
			Release();
			return false;
		}
		w = width;
		h = height;
	}
	glGetIntegerv(GL_VIEWPORT, viewport);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, w, h);
	return true;
}

void RenderTarget::End() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	glBindTexture(GL_TEXTURE_2D, colour);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}
