#pragma once

#include "glstuff.h"

#define ARRAY_SIZE_IN_ELEMENTS(arr) (sizeof(arr)/sizeof(arr[0]))
#define ZERO_MEM(arr) memset(arr, 0, sizeof(arr))

#define GBUFFER_POSITION_TEXTURE_UNIT 0
#define GBUFFER_DIFFUSE_TEXTURE_UNIT  1
#define GBUFFER_NORMAL_TEXTURE_UNIT   2
#define GBUFFER_TEXCOORD_TEXTURE_UNIT 3

class GBuffer
{
public:

	enum GBUFFER_TEXTURE_TYPE {
		GBUFFER_TEXTURE_TYPE_POSITION,
		GBUFFER_TEXTURE_TYPE_DIFFUSE,
		GBUFFER_TEXTURE_TYPE_NORMAL,
		GBUFFER_TEXTURE_TYPE_TEXCOORD,
		GBUFFER_NUM_TEXTURES
	};

	GBuffer();
	~GBuffer();

	bool init(unsigned int width, unsigned int height);

	void bindForWriting();
	void bindForReading();
	void setReadBuffer(GBUFFER_TEXTURE_TYPE TextureType);

	FrameBuffer m_fbo;
	Texture m_texturePosition;
	Texture m_textureDiffuse;
	Texture m_textureNormal;
	Texture m_textureTexcoord;
	Texture m_textureDepth;
};
