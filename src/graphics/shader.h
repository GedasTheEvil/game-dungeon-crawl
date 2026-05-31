#ifndef ShaderH
#define ShaderH

#include "ani.h"
#include "shader.h"

// User Defined Structures
typedef struct tagMATRIX {
	float Data[16];
} MATRIX;

typedef struct tagVECTOR {
	float X, Y, Z;
} VECTOR;

typedef struct tagVERTEX {
	VECTOR Nor;
	VECTOR Pos;
} VERTEX;

typedef struct tagPOLYGON {
	VERTEX Verts[3];
} POLYGON;

class AnimatedCartoonModel : public AnimatedModel {
  private:
	VECTOR lightAngle;

  public:
	int shaderTexture[1];
	int outlineWidth;
	bool outline;

	AnimatedCartoonModel();
	// 	   ~AnimatedCartoonModel();
	void ShowC();
};

#endif
