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
#include "cashe.h"

extern Cashe c;

// Math Functions
inline float DotProduct (VECTOR &V1, VECTOR &V2)
{
	return V1.X * V2.X + V1.Y * V2.Y + V1.Z * V2.Z;
}

inline float Magnitude (VECTOR &V)	
{
	return sqrtf (V.X * V.X + V.Y * V.Y + V.Z * V.Z);
}

void Normalize (VECTOR &V)
{
	float M = Magnitude (V);

	if (M != 0.0f)
	{
		V.X /= M;
		V.Y /= M;
		V.Z /= M;
	}
}

void RotateVector (MATRIX &M, VECTOR &V, VECTOR &D)
{
	D.X = (M.Data[0] * V.X) + (M.Data[4] * V.Y) + (M.Data[8]  * V.Z);	
	D.Y = (M.Data[1] * V.X) + (M.Data[5] * V.Y) + (M.Data[9]  * V.Z);	
	D.Z = (M.Data[2] * V.X) + (M.Data[6] * V.Y) + (M.Data[10] * V.Z);	
}


CartoonANI::CartoonANI()
{
     char Line[255];
     float shaderData[32][3];
     
     FILE *In	= NULL;
     In = fopen ("Textures/Shader.bmp", "r");

     if (In)	
     {
	   for (int i = 0; i < 32; i++)	
	   {
		 if (feof (In))
		 break;

		 fgets (Line, 255, In);

		 shaderData[i][0] = shaderData[i][1] = shaderData[i][2] = float(atof(Line)); 
	   }

	   fclose (In);
     }
     
     glGenTextures (1, (GLuint *)&shaderTexture[0]);	

     glBindTexture (GL_TEXTURE_1D, shaderTexture[0]);

     glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);	
     glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

     glTexImage1D (GL_TEXTURE_1D, 0, GL_RGB, 32, 0, GL_RGB , GL_FLOAT, shaderData);	

     lightAngle.X = 0.0f;		
     lightAngle.Y = 0.0f;		
     lightAngle.Z = 1.0f;		

     Normalize (lightAngle);
     
     outlineWidth = 2;          
}

void CartoonANI::ShowC()
{
     glEnable (GL_CULL_FACE);
     float TmpShade;
     MATRIX TmpMatrix;
     VECTOR TmpVector, TmpNormal;
     
     glGetFloatv (GL_MODELVIEW_MATRIX, TmpMatrix.Data);
     
     glEnable (GL_TEXTURE_1D);
     glBindTexture (GL_TEXTURE_1D, shaderTexture[0]);
     
    
     glBegin (GL_TRIANGLES);
     
     for(int i = 0; i < VCount * 3; i+= 3)
     {
	   TmpNormal.X = Normals[i];
	   TmpNormal.Y = Normals[i+1];
	   TmpNormal.Z = Normals[i+2];

	   RotateVector (TmpMatrix, TmpNormal, TmpVector);	

	   Normalize (TmpVector);			

	   TmpShade = DotProduct (TmpVector, lightAngle);	

	   if (TmpShade < 0.0f)
		 TmpShade = 0.0f;

	   glTexCoord1f (TmpShade);
	   glVertex3f(Ver[(int) frame].v[i],Ver[(int) frame].v[i+1],Ver[(int) frame].v[i+2]);   
	   
     }
     
     glEnd ();
     
     glDisable (GL_TEXTURE_1D);
     
     if(outline)
     {
	   glEnable (GL_BLEND);
	   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	   glPolygonMode (GL_BACK, GL_LINE);
	   glLineWidth (outlineWidth);

	   glCullFace (GL_FRONT);

	   glDepthFunc (GL_LEQUAL);	

	   glColor3f (0,0,0);	

	   glBegin (GL_TRIANGLES);	

	   for(int i = 0; i < VCount * 3; i+= 3)
		 glVertex3f(Ver[(int) frame].v[i],Ver[(int) frame].v[i+1],Ver[(int) frame].v[i+2]);   
	   glEnd ();	

	   glDepthFunc (GL_LEQUAL);

	   glCullFace (GL_BACK);

	   glPolygonMode (GL_BACK, GL_FILL);

	   glDisable (GL_BLEND);
     }
     
     glColor3f (1.0f, 1.0f, 1.0f);
        
     
     if(c.Orig_model)
     {
	   glEnable (GL_BLEND);
	   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	   glColor4f (1.0f, 1.0f, 1.0f,0.4);
	   Show();
	   glDisable (GL_BLEND);
     }
     
     glDisable (GL_CULL_FACE);
}
