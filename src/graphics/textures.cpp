#include "textures.h"
#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include <cstddef>
#include <GL/gl.h>
#include "../core/logger.h"

Textura::Textura() {
	loaded = false;
	texture = new TextureImage[1]();
}

Textura::~Textura() {
	if (texture != nullptr && texture[0].data != nullptr) {
		free(texture[0].data);
		texture[0].data = nullptr;
	}

	delete[] texture;
	loaded = false;
}

int Textura::LoadTGA(const char* filename) // Loads A TGA File Into Memory
{
	try {
		char tgAheader[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Uncompressed TGA Header
		char tgAcompare[12];									   // Used To Compare TGA Header
		char header[6];											   // First 6 Useful Bytes From The Header
		int bytesPerPixel;	// Holds Number Of Bytes Per Pixel Used In The TGA File
		size_t imageSize;	// Used To Store The Image Size When Setting Aside Ram
		int temp;			// Temporary Variable
		int type = GL_RGBA; // Set The Default GL Mode To RBGA (32 BPP)

		FILE* file = fopen(filename, "rb"); // Open The TGA File

		if (file == nullptr ||														// Does File Even Exist?
			fread(tgAcompare, 1, sizeof(tgAcompare), file) != sizeof(tgAcompare) || // Are There 12 Bytes To Read?
			memcmp(tgAheader, tgAcompare, sizeof(tgAheader)) != 0 ||  // Does The Header Match What We Want?
			fread(header, 1, sizeof(header), file) != sizeof(header)) // If So Read Next 6 Header Bytes
		{
			if (file != nullptr) // Did The File Even Exist? *Added Jim Strong*
				fclose(file);	 // If Anything Failed, Close The File

			return 0; // Return False
		}

		texture->width = header[1] * 256 + header[0];  // Determine The TGA Width	(highbyte*256+lowbyte)
		texture->height = header[3] * 256 + header[2]; // Determine The TGA Height	(highbyte*256+lowbyte)

		if (texture->width <= 0 ||				  // Is The Width Less Than Or Equal To Zero
			texture->height <= 0 ||				  // Is The Height Less Than Or Equal To Zero
			(header[4] != 24 && header[4] != 32)) // Is The TGA 24 or 32 Bit?
		{
			fclose(file); // If Anything Failed, Close The File
			return 0;	  // Return False
		}
		LOG_INFOF("texture", "%s:: \n\tHeight:%d\n\tWidth:%d", filename, texture->height, texture->width);

		texture->bpp = static_cast<unsigned char>(header[4]); // Grab The TGA's Bits Per Pixel (24 or 32)
		bytesPerPixel = texture->bpp / 8;					  // Divide By 8 To Get The Bytes Per Pixel
		imageSize = static_cast<size_t>(texture->width) * texture->height *
					bytesPerPixel; // Calculate The Memory Required For The TGA Data

		texture->data = static_cast<char*>(malloc(imageSize)); // Reserve Memory To Hold The TGA Data

		if (texture->data == nullptr) { // Does The Storage Memory Exist?
			fclose(file);				// Close The File
			throw std::runtime_error("Failed to allocate memory for texture data");
		}

		if (fread(texture->data, 1, imageSize, file) !=
			(size_t)imageSize) { // Does The Image Size Match The Memory Reserved?
			free(texture->data); // If So, Release The Image Data
			texture->data = nullptr;
			fclose(file); // Close The File
			throw std::runtime_error("Failed to read texture data from file");
		}

		LOG_INFOF("texture", "Size:%zu", imageSize);
		size_t i;

		for (i = 0; i < imageSize; i += bytesPerPixel)			 // Loop Through The Image Data
		{														 // Swaps The 1st And 3rd Bytes ('R'ed and 'B'lue)
			temp = static_cast<unsigned char>(texture->data[i]); // Temporarily Store The Value At Image Data 'i'
			texture->data[i] = texture->data[i + 2];			 // Set The 1st Byte To The Value Of The 3rd Byte
			texture->data[i + 2] = static_cast<char>(temp); // Set The 3rd Byte To The Value In 'temp' (1st Byte Value)
		}

		fclose(file); // Close The File

		// Build A Texture From The Data
		glGenTextures(1, (GLuint*)&texture[0].texID); // Generate OpenGL texture IDs

		glBindTexture(GL_TEXTURE_2D, texture[0].texID);					  // Bind Our Texture
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Linear Filtered
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Linear Filtered

		if (texture[0].bpp == 24) // Was The TGA 24 Bits
		{
			type = GL_RGB; // If So Set The 'type' To GL_RGB
		}

		glTexImage2D(GL_TEXTURE_2D, 0, type, texture[0].width, texture[0].height, 0, type, GL_UNSIGNED_BYTE,
					 texture[0].data);

		loaded = true;

		return 1; // Texture Building Went Ok, Return True
	} catch (const std::exception& e) {
		LOG_ERRORF("texture", "Error loading TGA %s: %s", filename, e.what());
		return 0;
	}
}
//================================================================================================================================
int Textura::LoadBMP(const char* filename) {
	try {
		FILE* file;
		unsigned long size;		   // size of the image in bytes.
		unsigned long i;		   // standard counter.
		unsigned short int planes; // number of planes in image (must be 1)
		unsigned short int bpp;	   // number of bits per pixel (must be 24)

		// Make sure the file exists
		file = fopen(filename, "rb");
		if (file == nullptr) {
			LOG_WARNINGF("texture", "File Not Found : %s", filename);
			return 0;
		}

		// Skip to bmp header
		fseek(file, 18, SEEK_CUR);

		// read width
		i = fread(&texture[0].width, 4, 1, file);
		if (i != 1) {
			LOG_ERRORF("texture", "Error reading width from %s.", filename);
			fclose(file);
			return 0;
		}
		LOG_INFOF("texture", "Width of %s: %d", filename, texture[0].width);

		// read the height
		i = fread(&texture[0].height, 4, 1, file);
		if (i != 1) {
			LOG_ERRORF("texture", "Error reading height from %s.", filename);
			fclose(file);
			return 0;
		}
		LOG_INFOF("texture", "Height of %s: %d", filename, texture[0].height);

		// calculate the size (assuming 24 bpp)
		size = static_cast<unsigned long>(texture[0].width) * texture[0].height * 3;

		// read the planes
		if ((fread(&planes, 2, 1, file)) != 1) {
			LOG_ERRORF("texture", "Error reading planes from %s.", filename);
			fclose(file);
			return 0;
		}

		if (planes != 1) {
			LOG_WARNINGF("texture", "Planes from %s is not 1: %u", filename, planes);
			fclose(file);
			return 0;
		}

		// read the bpp
		i = fread(&bpp, 2, 1, file);
		if (i != 1) {
			LOG_ERRORF("texture", "Error reading bpp from %s.", filename);
			fclose(file);
			return 0;
		}

		if (bpp != 24) {
			LOG_WARNINGF("texture", "Bpp from %s is not 24: %u", filename, bpp);
			fclose(file);
			return 0;
		}

		// seek past the rest of the bitmap header
		fseek(file, 24, SEEK_CUR);

		// Read the data
		LOG_INFOF("texture", "creating data array of size %lu", size);

		texture[0].data = static_cast<char*>(malloc(size));

		if (texture[0].data == nullptr) {
			LOG_ERROR("texture", "Error allocating memory for texture data");
			fclose(file);
			throw std::runtime_error("Failed to allocate memory for texture data");
		}

		if (fread(&texture[0].data[0], size, 1, file) != 1) {
			LOG_ERRORF("texture", "Error reading texture data from %s.", filename);
			free(texture[0].data);
			texture[0].data = nullptr;
			fclose(file);
			throw std::runtime_error("Failed to read texture data from file");
		}

		// windows neturi GL_BGR, darom savo
		char tmpC;
		for (unsigned long p = 0; p < size; p += 3) {
			tmpC = texture[0].data[p];
			texture[0].data[p] = texture[0].data[p + 2];
			texture[0].data[p + 2] = tmpC;
		}

		glGenTextures(1, /*(GLuint *)*/ &texture[0].texID);

		LOG_INFOF("texture", "Texture id=[%d]", texture[0].texID);

		glBindTexture(GL_TEXTURE_2D, texture[0].texID); // 2d texture (x and y size)

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
						GL_LINEAR); // scale linearly when image bigger than texture

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
						GL_LINEAR); // scale linearly when image smalled than texture

		glTexImage2D(GL_TEXTURE_2D, 0, 3, texture[0].width, texture[0].height, 0, GL_RGB, GL_UNSIGNED_BYTE,
					 texture[0].data);

		loaded = true;
		fclose(file);

		return 1;
	} catch (const std::exception& e) {
		LOG_ERRORF("texture", "Error loading BMP %s: %s", filename, e.what());
		return 0;
	}
}
//----------------------------------------------------------------------------------
void Textura::Bind() { glBindTexture(GL_TEXTURE_2D, texture[0].texID); }
//----------------------------------------------------------------------------------
int Textura::ID() { return static_cast<int>(texture[0].texID); }
