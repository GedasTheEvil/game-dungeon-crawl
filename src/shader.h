#ifndef ShaderH
#define ShaderH

#include "ANI.h"
#include "shader.h"

// User Defined Structures
typedef struct tagMATRIX	
{
	float Data[16];	
}
MATRIX;

typedef struct tagVECTOR	
{
	float X, Y, Z;	
}
VECTOR;

typedef struct tagVERTEX		
{
	VECTOR Nor;	
	VECTOR Pos;
}
VERTEX;

typedef struct tagPOLYGON	
{
	VERTEX Verts[3];	
}
POLYGON;

class CartoonANI : public ANI
{
     private:
	   VECTOR lightAngle;
     
     public:

	   int	shaderTexture[1];
	   int outlineWidth;
	   bool outline;
	   
	   CartoonANI();
// 	   ~CartoonANI();
	   void ShowC();
	   
};

#endif
