#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "animated_model.h"
#include "textures.h"
#include "../core/logger.h"
#include "../core/timer.h"
#include <GL/glu.h>

using namespace std;

//============================================================
AnimatedModel::AnimatedModel() {
	speed = 1;
	compiled = false;
	texture = 0;
	loop = true;
	playback.stepStart = GameClock::now();
}
////============================================================
AnimatedModel::~AnimatedModel() {
	if (glContextCurrent())
		for (int list : List)
			glDeleteLists(static_cast<GLuint>(list), 1);
}
//============================================================
int AnimatedModel::Load(const char fileName[]) { return mesh.Load(fileName) ? 1 : 0; }
//============================================================
void AnimatedModel::Show() const { Show(playback); }
//============================================================
void AnimatedModel::Show(const AnimPlayback& p) const { Show(p, texture); }
//============================================================
void AnimatedModel::Show(const AnimPlayback& p, int textureId) const {
	const auto frame = static_cast<int>(p.frame);
	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textureId));

	if (!compiled) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, mesh.frames[frame].v.data());
		glNormalPointer(GL_FLOAT, 0, mesh.frames[frame].n.data());
		glTexCoordPointer(2, GL_FLOAT, 0, mesh.texCoords.data());
		glDrawArrays(GL_TRIANGLES, 0, mesh.cornerCount);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
	} else
		glCallList(List[frame]);
}
//============================================================
//============================================================
void AnimatedModel::Advance() { Advance(playback); }
//============================================================
void AnimatedModel::Advance(AnimPlayback& p) const { AdvancePlayback(p, mesh.frameCount, loop, speed); }
//============================================================
void AnimatedModel::setSpeed(int nSpeed) {
	if (nSpeed < 1)
		speed = 1;
	else
		speed = nSpeed;
}
//============================================================
void AnimatedModel::Compile(bool keepFrames) {
	if (compiled)
		return; // avoid too many compilations

	List.resize(mesh.frameCount);

	for (int i = 0; i < mesh.frameCount; i++) {
		List[i] = static_cast<int>(glGenLists(1));

		glBindTexture(GL_TEXTURE_2D, texture);

		glNewList(List[i], GL_COMPILE);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, mesh.frames[i].v.data());
		glNormalPointer(GL_FLOAT, 0, mesh.frames[i].n.data());
		glTexCoordPointer(2, GL_FLOAT, 0, mesh.texCoords.data());
		glDrawArrays(GL_TRIANGLES, 0, mesh.cornerCount /*/divisor*/);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);

		glEndList();
	}

	compiled = true;
	// The lists hold their own copy. Frame 0 stays for the measures (YRange, HalfXZ, Vertex).
	if (!keepFrames) {
		mesh.frames.resize(1);
		mesh.frames.shrink_to_fit();
	}
}
//============================================================
void AnimatedModel::BindTexture(int t) { texture = t; }
//============================================================
void AnimatedModel::Reset() { playback.frame = 0.0; }
//============================================================
