#include "textures.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/gl.h>

Textura::Textura()
{
     loaded = false;
     texture = new TextureImage [1];
     
}

Textura::~Textura()
{
     if(loaded)delete texture;
     loaded = false;
     printf("Deleting Texture %x \n",this);

}

int Textura::LoadTGA(const char *filename)			// Loads A TGA File Into Memory
{
	char		TGAheader[12]={0,0,2,0,0,0,0,0,0,0,0,0};	// Uncompressed TGA Header
	char		TGAcompare[12];					// Used To Compare TGA Header
	char		header[6];					// First 6 Useful Bytes From The Header
	int		bytesPerPixel;					// Holds Number Of Bytes Per Pixel Used In The TGA File
	int		imageSize;					// Used To Store The Image Size When Setting Aside Ram
	int		temp;						// Temporary Variable
	int		type=GL_RGBA;					// Set The Default GL Mode To RBGA (32 BPP)

	FILE *file = fopen(filename, "rb");				// Open The TGA File

	if(	file==NULL ||						// Does File Even Exist?
		fread(TGAcompare,1,sizeof(TGAcompare),file)!=sizeof(TGAcompare) ||	// Are There 12 Bytes To Read?
		memcmp(TGAheader,TGAcompare,sizeof(TGAheader))!=0		||	// Does The Header Match What We Want?
		fread(header,1,sizeof(header),file)!=sizeof(header))			// If So Read Next 6 Header Bytes
	{
		if (file != NULL)							// Did The File Even Exist? *Added Jim Strong*
			fclose(file);							// If Anything Failed, Close The File
		
		return false;								// Return False
	}

	texture->width  = header[1] * 256 + header[0];					// Determine The TGA Width	(highbyte*256+lowbyte)
	texture->height = header[3] * 256 + header[2];					// Determine The TGA Height	(highbyte*256+lowbyte)
                                                	
 	if(	texture->width	<=0	||						// Is The Width Less Than Or Equal To Zero
		texture->height	<=0	||						// Is The Height Less Than Or Equal To Zero
		(header[4]!=24 && header[4]!=32))					// Is The TGA 24 or 32 Bit?
	{
		fclose(file);								// If Anything Failed, Close The File
		return false;								// Return False
	}
	printf("%s:: \n\tHeight:%d\n\tWidth:%d\n",filename,texture->height,texture->width);

	texture->bpp	= header[4];					// Grab The TGA's Bits Per Pixel (24 or 32)
	bytesPerPixel	= texture->bpp/8;				// Divide By 8 To Get The Bytes Per Pixel
	imageSize		= texture->width*texture->height*bytesPerPixel;	// Calculate The Memory Required For The TGA Data

	texture->data = (char *)malloc(imageSize);				// Reserve Memory To Hold The TGA Data

	if(	texture->data==NULL ||									// Does The Storage Memory Exist?
		fread(texture->data, 1, imageSize, file)!=imageSize)	// Does The Image Size Match The Memory Reserved?
	{
		if(texture->data!=NULL)								// Was Image Data Loaded
		{
			free(texture->data);								// If So, Release The Image Data
			texture->data = NULL;
		}		

		fclose(file);												// Close The File
		return false;												// Return False
	}

     printf("\tSize:%d\n",imageSize);
	int i;

	for(i=0; i<(unsigned int)imageSize; i+=bytesPerPixel)	// Loop Through The Image Data
	{																// Swaps The 1st And 3rd Bytes ('R'ed and 'B'lue)
		temp=texture->data[i];									// Temporarily Store The Value At Image Data 'i'
		texture->data[i] = texture->data[i + 2];			// Set The 1st Byte To The Value Of The 3rd Byte
		texture->data[i + 2] = temp;							// Set The 3rd Byte To The Value In 'temp' (1st Byte Value)
	}

	fclose (file);													// Close The File

	// Build A Texture From The Data
	glGenTextures(1, (GLuint *)&texture[0].texID);							// Generate OpenGL texture IDs

	glBindTexture(GL_TEXTURE_2D, texture[0].texID);				// Bind Our Texture
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	// Linear Filtered
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// Linear Filtered
	
	if (texture[0].bpp==24)					// Was The TGA 24 Bits
	{
		type=GL_RGB;						// If So Set The 'type' To GL_RGB
	}

	glTexImage2D(GL_TEXTURE_2D, 0, type, texture[0].width, texture[0].height,
		0, type, GL_UNSIGNED_BYTE, texture[0].data);
	
	loaded = true;

	return true;													// Texture Building Went Ok, Return True
}
//================================================================================================================================
int Textura::LoadBMP(const char *filename)
{

     FILE *file;
     unsigned long size;                 // size of the image in bytes.
     unsigned long i;                    // standard counter.
     unsigned short int planes;          // number of planes in image (must be 1) 
     unsigned short int bpp;             // number of bits per pixel (must be 24)
     //     char temp;                          // used to convert bgr to rgb color.

     // Make sure the file exists
     if ((file = fopen(filename, "rb"))==NULL)
     {
	   printf("File Not Found : %s\n",filename);
	   return 0;
     }

     // Skip to bmp header
     fseek(file,18, SEEK_CUR);

     // read width
     if ((i = fread(&texture[0].width, 4, 1, file)) != 1) {
	   printf("Error reading width from %s.\n", filename);
	   return 0;
     }
     printf("Width of %s: %lu\n",filename, texture[0].width);

     //read the height
     if ((i = fread(&texture[0].height,4,1,file)) != 1) {
	   printf("Error reading height from %s.\n", filename);
	   return 0;
     }
     printf("Height of %s: %lu\n", filename, texture[0].height);

     // calculate the size (assuming 24 bpp)
     size = texture[0].width * texture[0].height * 3;

     // read the planes
     if ((fread(&planes, 2, 1, file)) != 1) {
	   printf("Error reading planes from %s. \n", filename);
	   return 0;
     }

     if (planes != 1) {
	   printf("Planes from %s is not 1: %u\n", filename, planes);
	   return 0;
     }

     // read the bpp
     if ((i = fread(&bpp, 2, 1, file)) != 1) {
	   printf("Error reading bpp from %s. \n", filename);
	   return 0;
     }

     if (bpp != 24) {
	   printf("Bpp from %s is not 24: %u\n", filename, bpp);
	   return 0;
     }
     

     // seek past the rest of the bitmap header
     fseek(file, 24, SEEK_CUR);

     // Read the data
     printf("creating data array of size %d\n",size);
     
     texture[0].data = NULL;
//      texture[0].data = new char[size];
     texture[0].data = (char *) malloc(size);
     
     if (texture[0].data == NULL) 
     {
	   printf("Error allocating memory for texture data\n");
	   return 0;
     }


     if ((i = fread(&texture[0].data[0],size,1,file)) != 1) 
     {
	   printf("Error reading texture data from %s.\n", filename);
	   return 0;
     }
	 
	 //windows neturi GL_BGR, darom savo
	 char tmp_c;
	 for(int i = 0; i< size; i+=3)
	 {
		tmp_c = texture[0].data[i];
		texture[0].data[i] = texture[0].data[i+2];
		texture[0].data[i+2] = tmp_c;
	 }


     glGenTextures(1, /*(GLuint *)*/ &texture[0].texID);
     
     printf("Texture id=[%d]\n",texture[0].texID);

     glBindTexture(GL_TEXTURE_2D, texture[0].texID);   // 2d texture (x and y size)

     glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // scale linearly when image bigger than texture

     glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // scale linearly when image smalled than texture

     glTexImage2D(GL_TEXTURE_2D, 0, 3, texture[0].width, texture[0].height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture[0].data);

     loaded = true;
     
     return 1;
}
//----------------------------------------------------------------------------------
void Textura::Bind()
{ 
     glBindTexture(GL_TEXTURE_2D, texture[0].texID);
}
//----------------------------------------------------------------------------------
int Textura::ID()
{
     return texture[0].texID;
}


