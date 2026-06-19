#include <iostream>
#include <fstream>
#include "ani.h"
#include "../core/timer.h"
#include <GL/glu.h>
#include <cmath>
#include <stdio.h>

using namespace std;
//============================================================
AnimatedModel::AnimatedModel() {
	VCount = 0;
	frame = 0;
	speed = 1;
	scale = 0.0f;
	frameC = 0;
	compiled = 0;
	texture = 0;
	bounds = 0;
	loop = 1;
	frameChange = std::make_unique<timer>(100);
}
////============================================================
AnimatedModel::~AnimatedModel() {}
//============================================================
int AnimatedModel::Load(const char fileName[]) {
	ifstream r(fileName);
	r >> VCount >> frameC;

	if (!VCount)
		return 0;

	float tmp;
	Ver.resize(frameC + 1);
	Ver[0].v.resize(VCount * 3);
	Normals.resize(VCount * 3);
	TexCords.resize(VCount * 2);

	for (int i = 0; i < VCount * 3; i++)
		r >> Ver[0].v[i];

	for (int i = 0; i < VCount * 3; i++)
		r >> Normals[i];

	for (int i = 0; i < VCount * 2; i++)
		r >> TexCords[i];

	for (int j = 1; j < frameC + 1; j++) {
		Ver[j].v.resize(VCount * 3);
		for (int i = 0; i < VCount * 3; i++)
			r >> Ver[j].v[i];
	}

	return 1;
}
//============================================================
void AnimatedModel::Show() {

	if (bounds) // debug:: Bounding cube
	{
		glBegin(GL_LINE_STRIP);
		glVertex3f(-0.5, 1, -0.5);
		glVertex3f(0.5, 1, -0.5);
		glVertex3f(0.5, 0, -0.5);
		glVertex3f(-0.5, 0, -0.5);

		glVertex3f(-0.5, 0, 0.5);
		glVertex3f(-0.5, 1, 0.5);
		glVertex3f(0.5, 1, 0.5);
		glVertex3f(0.5, 0, 0.5);
		glVertex3f(-0.5, 0, 0.5);

		glEnd();

		glBegin(GL_LINE_STRIP);
		glVertex3f(-0.5, 1, 0.5);
		glVertex3f(-0.5, 1, -0.5);
		glVertex3f(0.5, 1, -0.5);
		glVertex3f(0.5, 1, 0.5);

		glEnd();

		glBegin(GL_LINE);
		glVertex3f(-0.5, 0, -0.5);
		glVertex3f(-0.5, 1, -0.5);

		glEnd();
	}

	glBindTexture(GL_TEXTURE_2D, texture);

	if (!compiled) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, Ver[static_cast<int>(frame)].v.data());
		glNormalPointer(GL_FLOAT, 0, Normals.data());
		glTexCoordPointer(2, GL_FLOAT, 0, TexCords.data());
		glDrawArrays(GL_TRIANGLES, 0, VCount);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
	} else
		glCallList(List[static_cast<int>(frame)]);
}
//============================================================
float AnimatedModel::getScale() {
	if (fabs(scale) > 0.00000000001)
		return scale;

	float minX = 1000.0, maxX = -1000.0;
	float minY = 1000.0, maxY = -1000.0;
	float minZ = 1000.0, maxZ = -1000.0;

	// find maximum dimensions of the model
	for (int i = 0; i < VCount * 3; i += 3) {
		if (Ver[static_cast<int>(frame)].v[i] > maxX)
			maxX = Ver[static_cast<int>(frame)].v[i];
		if (Ver[static_cast<int>(frame)].v[i] < minX)
			minX = Ver[static_cast<int>(frame)].v[i];

		if (Ver[static_cast<int>(frame)].v[i + 1] > maxY)
			maxY = Ver[static_cast<int>(frame)].v[i + 1];
		if (Ver[static_cast<int>(frame)].v[i + 1] < minY)
			minY = Ver[static_cast<int>(frame)].v[i + 1];

		if (Ver[static_cast<int>(frame)].v[i + 2] > maxZ)
			maxZ = Ver[static_cast<int>(frame)].v[i + 2];
		if (Ver[static_cast<int>(frame)].v[i + 2] < minZ)
			minZ = Ver[static_cast<int>(frame)].v[i + 2];
	}

	// find the largest scale

	float scX = maxX - minX;
	float scY = maxY - minY;
	float scZ = maxZ - minZ;

	if (scX > scY && scX > scZ)
		scale = scX;

	else if (scY > scX && scY > scZ)
		scale = scY;
	else
		scale = scZ;

	return scale;
}
//============================================================
void AnimatedModel::Advance_Animation() {
	if (frameC == 1)
		return;

	if (!frameChange->TimePassed())
		return;

	if (!loop && frame < frameC)
		frame += 0.04 * speed;

	if (!loop && frame >= frameC - 1)
		frame = frameC - 1;

	if (loop)
		frame += 0.04 * speed;

	if (loop && frame >= (frameC - 0.2))
		frame = 0.0;
}
//============================================================
void AnimatedModel::setSpeed(int nSpeed) {
	if (nSpeed < 1)
		speed = 1;
	else
		speed = nSpeed;
}
//============================================================
void AnimatedModel::Compile() {
	if (compiled)
		return; // avoid too many compilations

	List.resize(frameC);

	for (int i = 0; i < frameC; i++) {
		List[i] = glGenLists(1);

		glBindTexture(GL_TEXTURE_2D, texture);

		glNewList(List[i], GL_COMPILE);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, Ver[i].v.data());
		glNormalPointer(GL_FLOAT, 0, Normals.data());
		glTexCoordPointer(2, GL_FLOAT, 0, TexCords.data());
		glDrawArrays(GL_TRIANGLES, 0, VCount /*/divisor*/);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);

		glEndList();
	}

	compiled = 1;
}
//============================================================
void AnimatedModel::BindTexture(int t) { texture = t; }
//============================================================
void AnimatedModel::Centrify() {
	float scale = 1 / getScale();
	Scale(scale);

	float minX = 1000.0, maxX = -1000.0;
	float minY = 1000.0, maxY = -1000.0;
	float minZ = 1000.0, maxZ = -1000.0;

	// find maximum dimensions of the model
	for (int i = 0; i < VCount * 3; i += 3) {
		if (Ver[0].v[i] > maxX)
			maxX = Ver[0].v[i];
		if (Ver[0].v[i] < minX)
			minX = Ver[0].v[i];

		if (Ver[0].v[i + 1] > maxY)
			maxY = Ver[0].v[i + 1];
		if (Ver[0].v[i + 1] < minY)
			minY = Ver[0].v[i + 1];

		if (Ver[0].v[i + 2] > maxZ)
			maxZ = Ver[0].v[i + 2];
		if (Ver[0].v[i + 2] < minZ)
			minZ = Ver[0].v[i + 2];
	}

	Translate(-(minX + maxX) / 2, -minY, -(maxZ + minZ) / 2); // apacia bus 0, x centruojam, z 0

}
//============================================================
void AnimatedModel::Translate(float x, float y, float z) {
	for (int j = 0; j < frameC; j++) {
		for (int i = 0; i < VCount * 3; i += 3) {
			Ver[j].v[i] += x;
			Ver[j].v[i + 1] += y;
			Ver[j].v[i + 2] += z;
		}
	}
}
//============================================================
void AnimatedModel::Scale(float sc) {
	for (int j = 0; j < frameC; j++)
		for (int i = 0; i < VCount * 3; i++)
			Ver[j].v[i] *= sc;
}
//============================================================
void AnimatedModel::Reset() { frame = 0.0; }
//============================================================
