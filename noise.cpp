#include "noise.h"

/* 
 * Testbed for GLSL fragment noise() replacement.
 *
 * Shaders are loaded from two external files:
 * "GLSLnoisetest.vert" and "GLSLnoisetest.frag".
 * The program itself draws a spinning sphere
 * with a noise-generated fragment color.
 *
 * This program uses GLFW for convenience, to handle the OS-specific
 * window management stuff. Some Windows-specific stuff is still there,
 * though, so this code is not portable to other platforms.
 * The non-portable parts of the code is the MessageBox() function
 * call, which is easily replaced by a fprintf(stderr) call,
 * and probably most of the stupid loading of function pointers that
 * Windows requires of you to access anything above OpenGL 1.1.
 *
 * Author: Stefan Gustavson (stegu@itn.liu.se) 2004
 */

int perm[256]= {
	151,160,137,91,90,15,
	131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
	190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
	88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
	77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
	102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
	135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
	5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
	223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
	129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
	251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
	49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
	138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

/* These are Ken Perlin's proposed gradients for 3D noise. I kept them for
   better consistency with the reference implementation, but there is really
   no need to pad this to 16 gradients for this particular implementation.
   If only the "proper" first 12 gradients are used, they can be extracted
   from the grad4[][] array: grad3[i][j] == grad4[i*2][j], 0<=i<=11, j=0,1,2
*/
int grad3[16][3] = {
	{0,1,1},{0,1,-1},{0,-1,1},{0,-1,-1},
    {1,0,1},{1,0,-1},{-1,0,1},{-1,0,-1},
    {1,1,0},{1,-1,0},{-1,1,0},{-1,-1,0}, // 12 cube edges
    {1,0,-1},{-1,0,-1},{0,-1,1},{0,1,1} // 4 more to make 16
}; 

/* These are my own proposed gradients for 4D noise. They are the coordinates
   of the midpoints of each of the 32 edges of a tesseract, just like the 3D
   noise gradients are the midpoints of the 12 edges of a cube.
*/
int grad4[32][4]= {
	{0,1,1,1}, {0,1,1,-1}, {0,1,-1,1}, {0,1,-1,-1}, // 32 tesseract edges
	{0,-1,1,1}, {0,-1,1,-1}, {0,-1,-1,1}, {0,-1,-1,-1},
	{1,0,1,1}, {1,0,1,-1}, {1,0,-1,1}, {1,0,-1,-1},
	{-1,0,1,1}, {-1,0,1,-1}, {-1,0,-1,1}, {-1,0,-1,-1},
	{1,1,0,1}, {1,1,0,-1}, {1,-1,0,1}, {1,-1,0,-1},
	{-1,1,0,1}, {-1,1,0,-1}, {-1,-1,0,1}, {-1,-1,0,-1},
	{1,1,1,0}, {1,1,-1,0}, {1,-1,1,0}, {1,-1,-1,0},
	{-1,1,1,0}, {-1,1,-1,0}, {-1,-1,1,0}, {-1,-1,-1,0}
};

/* This is a look-up table to speed up the decision on which simplex we
   are in inside a cube or hypercube "cell" for 3D and 4D simplex noise.
   It is used to avoid complicated nested conditionals in the GLSL code.
   The table is indexed in GLSL with the results of six pair-wise
   comparisons beween the components of the P=(x,y,z,w) coordinates
   within a hypercube cell.
   c1 = x>=y ? 32 : 0;
   c2 = x>=z ? 16 : 0;
   c3 = y>=z ? 8 : 0;
   c4 = x>=w ? 4 : 0;
   c5 = y>=w ? 2 : 0;
   c6 = z>=w ? 1 : 0;
   offsets = simplex[c1+c2+c3+c4+c5+c6];
   o1 = step(160,offsets);
   o2 = step(96,offsets);
   o3 = step(32,offsets);
   (For the 3D case, c4, c5, c6 and o3 are not needed.)
*/
unsigned char simplex4[][4] = {
	{0,64,128,192},{0,64,192,128},{0,0,0,0},
	{0,128,192,64},{0,0,0,0},{0,0,0,0},{0,0,0,0},{64,128,192,0},
	{0,128,64,192},{0,0,0,0},{0,192,64,128},{0,192,128,64},
	{0,0,0,0},{0,0,0,0},{0,0,0,0},{64,192,128,0},
	{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
	{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
	{64,128,0,192},{0,0,0,0},{64,192,0,128},{0,0,0,0},
	{0,0,0,0},{0,0,0,0},{128,192,0,64},{128,192,64,0},
	{64,0,128,192},{64,0,192,128},{0,0,0,0},{0,0,0,0},
	{0,0,0,0},{128,0,192,64},{0,0,0,0},{128,64,192,0},
	{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
	{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
	{128,0,64,192},{0,0,0,0},{0,0,0,0},{0,0,0,0},
	{192,0,64,128},{192,0,128,64},{0,0,0,0},{192,64,128,0},
	{128,64,0,192},{0,0,0,0},{0,0,0,0},{0,0,0,0},
	{192,64,0,128},{0,0,0,0},{192,128,0,64},{192,128,64,0}
};




/*
 * initPermTexture(GLuint *texID) - create and load a 2D texture for
 * a combined index permutation and gradient lookup table.
 * This texture is used for 2D and 3D noise, both classic and simplex.
 */
GLuint initPermTexture()
{
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	GLuint texId;

	char *pixels;
	int i,j;
  
	glGenTextures(1, &texId); // Generate a unique texture ID
	glBindTexture(GL_TEXTURE_2D, texId); // Bind the texture to texture unit 0

	pixels = (char*)malloc( 256*256*4 );
	for(i = 0; i<256; i++)
	for(j = 0; j<256; j++) {
		int offset = (i*256+j)*4;
		char value = perm[(j+perm[i]) & 0xFF];
		pixels[offset] = grad3[value & 0x0F][0] * 64 + 64;   // Gradient x
		pixels[offset+1] = grad3[value & 0x0F][1] * 64 + 64; // Gradient y
		pixels[offset+2] = grad3[value & 0x0F][2] * 64 + 64; // Gradient z
		pixels[offset+3] = value;                     // Permuted index
	}
  
	// GLFW texture loading functions won't work here - we need GL_NEAREST lookup.
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	return texId;
}

/*
 * initSimplexTexture(GLuint *texID) - create and load a 1D texture for a
 * simplex traversal order lookup table. This is used for simplex noise only,
 * and only for 3D and 4D noise where there are more than 2 simplices.
 * (3D simplex noise has 6 cases to sort out, 4D simplex noise has 24 cases.)
 */
GLuint initSimplexTexture()
{
	glEnable(GL_TEXTURE_1D);
	glActiveTexture(GL_TEXTURE1); // Activate a different texture unit (unit 1)

	GLuint texId;
	glGenTextures(1, &texId); // Generate a unique texture ID
	glBindTexture(GL_TEXTURE_1D, texId); // Bind the texture to texture unit 1

	// GLFW texture loading functions won't work here - we need GL_NEAREST lookup.
	glTexImage1D( GL_TEXTURE_1D, 0, GL_RGBA, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, simplex4 );
	glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	glActiveTexture(GL_TEXTURE0); // Switch active texture unit back to 0 again

	return texId;
}

/*
 * initGradTexture(GLuint *texID) - create and load a 2D texture
 * for a 4D gradient lookup table. This is used for 4D noise only.
 */
GLuint initGradTexture()
{
	glEnable(GL_TEXTURE_2D);
	GLuint texId;

	char *pixels;
	int i,j;
  
	glActiveTexture(GL_TEXTURE2); // Activate a different texture unit (unit 2)

	glGenTextures(1, &texId); // Generate a unique texture ID
	glBindTexture(GL_TEXTURE_2D, texId); // Bind the texture to texture unit 2

	pixels = (char*)malloc( 256*256*4 );
	for(i = 0; i<256; i++)
	for(j = 0; j<256; j++) {
		int offset = (i*256+j)*4;
		char value = perm[(j+perm[i]) & 0xFF];
		pixels[offset] = grad4[value & 0x1F][0] * 64 + 64;   // Gradient x
		pixels[offset+1] = grad4[value & 0x1F][1] * 64 + 64; // Gradient y
		pixels[offset+2] = grad4[value & 0x1F][2] * 64 + 64; // Gradient z
		pixels[offset+3] = grad4[value & 0x1F][3] * 64 + 64; // Gradient z
	}
  
	// GLFW texture loading functions won't work here - we need GL_NEAREST lookup.
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	glActiveTexture(GL_TEXTURE0); // Switch active texture unit back to 0 again

	return texId;
}
