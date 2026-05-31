#include "shader.h"

#ifndef WIN32
#include <GL/glut.h>
#endif
#ifdef WIN32
#include <GL/freeglut.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../state/cashe.h"

extern Cashe c;

// Math Functions
inline float dotProduct(VECTOR& v1, VECTOR& v2) { return v1.X * v2.X + v1.Y * v2.Y + v1.Z * v2.Z; }

inline float magnitude(VECTOR& v) { return sqrtf(v.X * v.X + v.Y * v.Y + v.Z * v.Z); }

void normalize(VECTOR& v) {
	float m = magnitude(v);

	if (m != 0.0f) {
		v.X /= m;
		v.Y /= m;
		v.Z /= m;
	}
}

void rotateVector(MATRIX& m, VECTOR& v, VECTOR& d) {
	d.X = (m.Data[0] * v.X) + (m.Data[4] * v.Y) + (m.Data[8] * v.Z);
	d.Y = (m.Data[1] * v.X) + (m.Data[5] * v.Y) + (m.Data[9] * v.Z);
	d.Z = (m.Data[2] * v.X) + (m.Data[6] * v.Y) + (m.Data[10] * v.Z);
}

AnimatedCartoonModel::AnimatedCartoonModel() {
	char line[255];
	float shaderData[32][3];

	FILE* in = nullptr;
	in = fopen("Textures/Shader.bmp", "r");

	if (in) {
		for (int i = 0; i < 32; i++) {
			if (feof(in))
				break;

			if (fgets(line, 255, in) == nullptr)
				break;

			shaderData[i][0] = shaderData[i][1] = shaderData[i][2] = float(atof(line));
		}

		fclose(in);
	}

	glGenTextures(1, (GLuint*)&shaderTexture[0]);

	glBindTexture(GL_TEXTURE_1D, shaderTexture[0]);

	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 32, 0, GL_RGB, GL_FLOAT, shaderData);

	lightAngle.X = 0.0f;
	lightAngle.Y = 0.0f;
	lightAngle.Z = 1.0f;

	normalize(lightAngle);

	outlineWidth = 2;
}

void AnimatedCartoonModel::ShowC() {
	glEnable(GL_CULL_FACE);
	float tmpShade;
	MATRIX tmpMatrix;
	VECTOR tmpVector, tmpNormal;

	glGetFloatv(GL_MODELVIEW_MATRIX, tmpMatrix.Data);

	glEnable(GL_TEXTURE_1D);
	glBindTexture(GL_TEXTURE_1D, shaderTexture[0]);

	glBegin(GL_TRIANGLES);

	for (int i = 0; i < VCount * 3; i += 3) {
		tmpNormal.X = Normals[i];
		tmpNormal.Y = Normals[i + 1];
		tmpNormal.Z = Normals[i + 2];

		rotateVector(tmpMatrix, tmpNormal, tmpVector);

		normalize(tmpVector);

		tmpShade = dotProduct(tmpVector, lightAngle);

		if (tmpShade < 0.0f)
			tmpShade = 0.0f;

		glTexCoord1f(tmpShade);
		glVertex3f(Ver[(int)frame].v[i], Ver[(int)frame].v[i + 1], Ver[(int)frame].v[i + 2]);
	}

	glEnd();

	glDisable(GL_TEXTURE_1D);

	if (outline) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glPolygonMode(GL_BACK, GL_LINE);
		glLineWidth(outlineWidth);

		glCullFace(GL_FRONT);

		glDepthFunc(GL_LEQUAL);

		glColor3f(0, 0, 0);

		glBegin(GL_TRIANGLES);

		for (int i = 0; i < VCount * 3; i += 3)
			glVertex3f(Ver[(int)frame].v[i], Ver[(int)frame].v[i + 1], Ver[(int)frame].v[i + 2]);
		glEnd();

		glDepthFunc(GL_LEQUAL);

		glCullFace(GL_BACK);

		glPolygonMode(GL_BACK, GL_FILL);

		glDisable(GL_BLEND);
	}

	glColor3f(1.0f, 1.0f, 1.0f);

	if (c.Orig_model) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1.0f, 1.0f, 1.0f, 0.4);
		Show();
		glDisable(GL_BLEND);
	}

	glDisable(GL_CULL_FACE);
}
