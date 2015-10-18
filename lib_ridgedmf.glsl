
uniform float uniform_lacunarity;
uniform float uniform_gain;
uniform float uniform_offset;
uniform int   uniform_octaves;
uniform float uniform_scale;
uniform float uniform_bias;

float snoise(vec3 p);

// Noise functions
float ridge(float h, float offset)
{
	return 1.0 - abs(h);

    h = offset - abs(h);
    return h * h;
}

float bias(float a, float b)
{
	return pow(a, log(b) / log(0.5));
}

float ridgedmf(vec3 p, float lacunarity, float gain, float offset, int octaves, int seed)
{
	float sum = 0.0;
	float freq = 1.0;
	float amp = 0.5;
	float prev = 1.0;
	for (int i = 0; i < octaves; ++i) 
	{
		float noise = snoise(p*freq+seed);
		noise = ridge(noise, offset);
		sum += noise*amp*prev;
		prev = noise;
		freq *= lacunarity;
		amp *= gain;
	}
	return sum;
}

const vec4 CH[10] = {
	vec4(0.0, 0.0, 0.0,                                                 -2.0),
	vec4(0.023529411764705882, 0.22745098039215686, 0.4980392156862745, -0.03125),
	vec4(0.054901960784313725, 0.4392156862745098, 0.7529411764705882,  -0.0001220703125),
	vec4(234.0/255.0, 206.0/255.0, 106.0/255.0,  0.0),
	vec4(0.27450980392156865, 0.47058823529411764, 0.23529411764705882,  0.01),
	vec4(0.43137254901960786, 0.5490196078431373, 0.29411764705882354,   0.125),
	vec4(0.6274509803921569, 0.5490196078431373, 0.43529411764705883,    0.25),
	vec4(0.7215686274509804, 0.6392156862745098, 0.5529411764705883,     0.375),
	vec4(1.0, 1.0, 1.0,                                                  0.75),
	vec4(0.5019607843137255, 1.0, 1.0,                                   2.0)
};

vec3 getColour(float altitude)
{
	if (altitude < CH[0].w)
		return CH[0].xyz;
		
	for (int i = 1; i < 10; ++i)
		if (altitude < CH[i].w)
			return mix(CH[i-1].xyz, CH[i].xyz, (altitude-CH[i-1].w)/(CH[i].w-CH[i-1].w));
			
	return vec3(0.0, 0.0, 1.0);
}

vec4 getColourAndAltitude(vec3 pos, int seed)
{
	float altitude = ridgedmf(
		10.0*(pos + vec3(seed, 0, 0)), 
		uniform_lacunarity, uniform_gain, uniform_offset, uniform_octaves, seed
	);
	altitude = uniform_scale*(altitude + uniform_bias);

	return vec4(getColour(altitude), 1.0 + altitude);
}
