
in vec2 v2_texCoord;
out vec3 colourOut;

uniform float exposure;
uniform sampler2D tex;

#define A 0.15
#define B 0.50
#define C 0.10
#define D 0.20
#define E 0.02
#define F 0.30

vec3 Uncharted2Tonemap(vec3 x)
{
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 hdr(vec3 x)
{
	return vec3(1.0) - exp(-exposure * x);
}

const float DX = 2.0/1920.0;
const float DY = 2.0/1080.0;

vec3 getAverage()
{
	return (
		vec3(texture(tex, v2_texCoord)) * 2.0 + 
		vec3(texture(tex, v2_texCoord + vec2(-DX, -DY))) +
		vec3(texture(tex, v2_texCoord + vec2(-DX, +DY))) + 
		vec3(texture(tex, v2_texCoord + vec2(+DX, -DY))) + 
		vec3(texture(tex, v2_texCoord + vec2(+DX, +DY)))
	) / 6.0;
}

void main()
{
	//colourOut = getAverage();
	colourOut = vec3(texture(tex, v2_texCoord));
	//float lightness = (colourOut.r + colourOut.g + colourOut.b) / 3.0;

	//colourOut = vec3(2.0*lightness, 2.0*lightness, lightness);
	//colourOut = Uncharted2Tonemap(colourOut * exposure);
	//colourOut = hdr(colourOut);
}
