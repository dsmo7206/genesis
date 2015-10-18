
uniform vec4 g_randomVectors[256];

#define CONTINENT_FREQUENCY 1.0
#define CONTINENT_LACUNARITY 2.208984375
#define MOUNTAIN_LACUNARITY 2.142578125
#define HILLS_LACUNARITY 2.162109375
#define PLAINS_LACUNARITY 2.314453125
#define BADLANDS_LACUNARITY 2.212890625
#define MOUNTAINS_TWIST 1.0
#define HILLS_TWIST 1.0
#define BADLANDS_TWIST 1.0
#define SEA_LEVEL 0.0
#define SHELF_LEVEL -0.375
#define MOUNTAINS_AMOUNT 0.5
#define BADLANDS_AMOUNT 0.03125
#define TERRAIN_OFFSET 1.0
#define MOUNTAIN_GLACIATION 1.375
#define RIVER_DEPTH 0.0234375
const float HILLS_AMOUNT = (1.0 + MOUNTAINS_AMOUNT) / 2.0;
const float CONTINENT_HEIGHT_SCALE = (1.0 - SEA_LEVEL) / 4.0;

#define SQRT_3 1.7320508075688772935
#define DEFAULT_PERLIN_LACUNARITY 2.0
#define DEFAULT_PERLIN_PERSISTENCE 0.5

int X_NOISE_GEN = 1619;
int Y_NOISE_GEN = 31337;
int Z_NOISE_GEN = 6971;
int SEED_NOISE_GEN = 1013;
int SHIFT_NOISE_GEN = 8;
ivec4 GEN_VEC = ivec4(X_NOISE_GEN, Y_NOISE_GEN, Z_NOISE_GEN, SEED_NOISE_GEN);

// All the following have been padded to the longest array's length
const float CONTINENTS_WITH_MOUNTAINS_CURVE_POINTS_IN[10]  = float[](-1.0, 0.0, 1.0 - MOUNTAINS_AMOUNT, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
const float CONTINENTS_WITH_MOUNTAINS_CURVE_POINTS_OUT[10] = float[](-0.0625, 0.0, 0.0625, 0.25, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
const int NUM_CONTINENTS_WITH_MOUNTAINS_CURVE_POINTS = 4;

const float RIVER_POSITIONS_CURVE0_POINTS_IN[10]  = float[](-2.0, -1.0, -0.125, 0.0, 1.0, 2.0, 0.0, 0.0, 0.0, 0.0);
const float RIVER_POSITIONS_CURVE0_POINTS_OUT[10] = float[](2.0, 1.0, 0.875, -1.0, -1.5, -2.0, 0.0, 0.0, 0.0, 0.0);
const int NUM_RIVER_POSITIONS_CURVE0_POINTS = 6;

const float RIVER_POSITIONS_CURVE1_POINTS_IN[10]  = float[](-2.0, -1.0, -0.125, 0.0, 1.0, 2.0, 0.0, 0.0, 0.0, 0.0);
const float RIVER_POSITIONS_CURVE1_POINTS_OUT[10] = float[](2.0, 1.5, 1.4375, 0.5, 0.25, 0.0, 0.0, 0.0, 0.0, 0.0);
const int NUM_RIVER_POSITIONS_CURVE1_POINTS = 6;

const float BADLANDS_CLIFF_CURVE_POINTS_IN[10]  = float[](-2.0, -1.0, 0.0, 0.5, 0.625, 0.75, 2.0, 0.0, 0.0, 0.0);
const float BADLANDS_CLIFF_CURVE_POINTS_OUT[10] = float[](-2.0, -1.25, -0.75, -0.25, 0.875, 1.0, 1.25, 0.0, 0.0, 0.0); 
const int NUM_BADLANDS_CLIFF_CURVE_POINTS = 7;

const float BASE_CONTINENT_CURVE_POINTS_IN[10]  = float[](-2.0000 + SEA_LEVEL, -1.0000 + SEA_LEVEL, SEA_LEVEL,          0.0625 + SEA_LEVEL, 0.1250 + SEA_LEVEL, 0.2500 + SEA_LEVEL, 0.5000 + SEA_LEVEL, 0.7500 + SEA_LEVEL, 1.0000 + SEA_LEVEL, 2.0000 + SEA_LEVEL);
const float BASE_CONTINENT_CURVE_POINTS_OUT[10] = float[](-1.625 + SEA_LEVEL,   -1.375 + SEA_LEVEL, -0.375 + SEA_LEVEL, 0.125 + SEA_LEVEL,  0.250 + SEA_LEVEL,  1.000 + SEA_LEVEL,  0.250 + SEA_LEVEL,  0.250 + SEA_LEVEL,  0.500 + SEA_LEVEL,  0.500 + SEA_LEVEL);
const int NUM_BASE_CONTINENT_CURVE_POINTS = 10;

// All the following have been padded to the longest array's length
const float CONTINENTAL_SHELF_TERRACE_POINTS[6] = float[](-1.0, -0.75, SHELF_LEVEL, 1.0, 0.0, 0.0);
const int NUM_CONTINENTAL_SHELF_TERRACE_POINTS = 4;

const float TERRAIN_TYPE_TERRACE_POINTS[6] = float[](-1.0, SHELF_LEVEL + SEA_LEVEL / 2.0, 1.0, 0.0, 0.0, 0.0);
const int NUM_TERRAIN_TYPE_TERRACE_POINTS = 3;

const float BADLANDS_CLIFF_TERRACE_POINTS[6] = float[](-1.0, -0.875, -0.75, -0.5, 0.0, 1.0);
const int NUM_BADLANDS_CLIFF_TERRACE_POINTS = 6;

float sCurve3(float x)
{
	return smoothstep(0.0, 1.0, x);
}

float sCurve5(float a)
{
	float a3 = a * a * a;
	float a4 = a3 * a;
	float a5 = a4 * a;
	return (6.0 * a5) - (15.0 * a4) + (10.0 * a3);
}

float cubicInterp(float n0, float n1, float n2, float n3, float a)
{
	float p = (n3 - n2) - (n0 - n1);
	float q = (n0 - n1) - p;
	float r = n2 - n0;
	float s = n1;
	return p * a * a * a + q * a * a + r * a + s;
}

float select(float source1, float source2, float control, float falloff, float lowerBound, float upperBound)
{
	float halfWay = 0.5 * (lowerBound + upperBound);
	float dist = abs(halfWay - control);
	return mix(source2, source1, smoothstep(upperBound - falloff - halfWay, upperBound + falloff - halfWay, dist));
}

int intValueNoise3D(ivec3 pos, int seed)
{
	int n = int(dot(GEN_VEC, ivec4(pos, seed))) & 0x7fffffff;
	n = (n >> 13) ^ n;
	return (n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff;
}

float valueNoise3D(ivec3 pos, int seed)
{
	return 1.0 - float(intValueNoise3D(pos, seed)) / 1073741824.0;
}
/*
float gradientNoise3D(vec3 pos, int ix, int iy, int iz, int seed)
{
	int vectorIndex = int(dot(GEN_VEC, ivec4(ix, iy, iz, seed)));// & 0xffffffff;

	vectorIndex ^= (vectorIndex >> SHIFT_NOISE_GEN);
	vectorIndex &= 0xff;
	
	vec3 gradVec = g_randomVectors[vectorIndex].xyz;
	vec3 pointVec = pos - vec3(ix, iy, iz);
	return dot(gradVec, pointVec) * 2.12;
}

float gradientCoherentNoise3D(vec3 pos, int seed)
{
	ivec3 pos0 = ivec3(floor(pos));
	ivec3 pos1 = pos0 + ivec3(1, 1, 1);

	float xs = sCurve3(pos.x - float(pos0.x)); // sCurve5 for best
	float ys = sCurve3(pos.y - float(pos0.y)); // sCurve5 for best
	float zs = sCurve3(pos.z - float(pos0.z)); // sCurve5 for best
	float iy0 = mix(
		mix(
			gradientNoise3D(pos, pos0.x, pos0.y, pos0.z, seed), 
			gradientNoise3D(pos, pos1.x, pos0.y, pos0.z, seed), 
			xs
		), 
		mix(
			gradientNoise3D(pos, pos0.x, pos1.y, pos0.z, seed), 
			gradientNoise3D(pos, pos1.x, pos1.y, pos0.z, seed), 
			xs
		), 
		ys
	);
	float iy1 = mix(
		mix(
			gradientNoise3D(pos, pos0.x, pos0.y, pos1.z, seed), 
			gradientNoise3D(pos, pos1.x, pos0.y, pos1.z, seed), 
			xs
		), 
		mix(
			gradientNoise3D(pos, pos0.x, pos1.y, pos1.z, seed), 
			gradientNoise3D(pos, pos1.x, pos1.y, pos1.z, seed), 
			xs
		), 
		ys
	);
	return mix(iy0, iy1, zs);
}

float gradientCoherentNoise3DBest(vec3 pos, int seed)
{
	ivec3 pos0 = ivec3(floor(pos));
	ivec3 pos1 = pos0 + ivec3(1, 1, 1);

	float xs = sCurve5(pos.x - float(pos0.x)); // sCurve5 for best
	float ys = sCurve5(pos.y - float(pos0.y)); // sCurve5 for best
	float zs = sCurve5(pos.z - float(pos0.z)); // sCurve5 for best
	float iy0 = mix(
		mix(
			gradientNoise3D(pos, pos0.x, pos0.y, pos0.z, seed), 
			gradientNoise3D(pos, pos1.x, pos0.y, pos0.z, seed), 
			xs
		), 
		mix(
			gradientNoise3D(pos, pos0.x, pos1.y, pos0.z, seed), 
			gradientNoise3D(pos, pos1.x, pos1.y, pos0.z, seed), 
			xs
		), 
		ys
	);
	float iy1 = mix(
		mix(
			gradientNoise3D(pos, pos0.x, pos0.y, pos1.z, seed), 
			gradientNoise3D(pos, pos1.x, pos0.y, pos1.z, seed), 
			xs
		), 
		mix(
			gradientNoise3D(pos, pos0.x, pos1.y, pos1.z, seed), 
			gradientNoise3D(pos, pos1.x, pos1.y, pos1.z, seed), 
			xs
		), 
		ys
	);
	return mix(iy0, iy1, zs);
}
*/

/*
vec3 mod289(vec3 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x) {
     return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

float gradientCoherentNoise3D(vec3 v, int seed)
  { 
  v += vec3(seed);

  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

// First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

// Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  //   x0 = x0 - 0.0 + 0.0 * C.xxx;
  //   x1 = x0 - i1  + 1.0 * C.xxx;
  //   x2 = x0 - i2  + 2.0 * C.xxx;
  //   x3 = x0 - 1.0 + 3.0 * C.xxx;
  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
  vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

// Permutations
  i = mod289(i); 
  vec4 p = permute( permute( permute( 
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

// Gradients: 7x7 points over a square, mapped onto an octahedron.
// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
  float n_ = 0.142857142857; // 1.0/7.0
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
  //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

//Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

// Mix final noise value
  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), 
                                dot(p2,x2), dot(p3,x3) ) );
  }

*/


#define  NORMALIZE_GRADIENTS
#undef  USE_CIRCLE
#define COLLAPSE_SORTNET

float permute(float x0,vec3 p) { 
  float x1 = mod(x0 * p.y, p.x);
  return floor(  mod( (x1 + p.z) *x0, p.x ));
}
vec2 permute(vec2 x0,vec3 p) { 
  vec2 x1 = mod(x0 * p.y, p.x);
  return floor(  mod( (x1 + p.z) *x0, p.x ));
}
vec3 permute(vec3 x0,vec3 p) { 
  vec3 x1 = mod(x0 * p.y, p.x);
  return floor(  mod( (x1 + p.z) *x0, p.x ));
}
vec4 permute(vec4 x0,vec3 p) { 
  vec4 x1 = mod(x0 * p.y, p.x);
  return floor(  mod( (x1 + p.z) *x0, p.x ));
}

//uniform vec4 pParam; 
// Example constant with a 289 element permutation
const vec4 pParam = vec4( 17.0*17.0, 34.0, 1.0, 7.0);

float taylorInvSqrt(float r)
{ 
  return ( 0.83666002653408 + 0.7*0.85373472095314 - 0.85373472095314 * r );
}

float gradientCoherentNoise3D(vec3 v, int seed)
{ 
	v += vec3(seed);
	

  const vec2  C = vec2(1./6. , 1./3. ) ;
  const vec4  D = vec4(0., 0.5, 1.0, 2.0);

// First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;
  
// Other corners
#ifdef COLLAPSE_SORTNET
  vec3 g = vec3( greaterThan(   x0.xyz, x0.yzx) );
  vec3 l = vec3( lessThanEqual( x0.xyz, x0.yzx) );

  vec3 i1 = g.xyz  * l.zxy;
  vec3 i2 = max( g.xyz, l.zxy);
#else
// Keeping this clean - let the compiler optimize.
  vec3 q1;
  q1.x = max(x0.x, x0.y);
  q1.y = min(x0.x, x0.y);
  q1.z = x0.z;

  vec3 q2;
  q2.x = max(q1.x,q1.z);
  q2.z = min(q1.x,q1.z);
  q2.y = q1.y;

  vec3 q3;
  q3.y = max(q2.y, q2.z);
  q3.z = min(q2.y, q2.z);
  q3.x = q2.x;

  vec3 i1 = vec3(equal(q3.xxx, x0));
  vec3 i2 = i1 + vec3(equal(q3.yyy, x0));
#endif

   //  x0 = x0 - 0. + 0. * C 
  vec3 x1 = x0 - i1 + 1. * C.xxx;
  vec3 x2 = x0 - i2 + 2. * C.xxx;
  vec3 x3 = x0 - 1. + 3. * C.xxx;

// Permutations
  i = mod(i, pParam.x ); 
  vec4 p = permute( permute( permute( 
             i.z + vec4(0., i1.z, i2.z, 1. ), pParam.xyz)
           + i.y + vec4(0., i1.y, i2.y, 1. ), pParam.xyz) 
           + i.x + vec4(0., i1.x, i2.x, 1. ), pParam.xyz);

// Gradients
// ( N*N points uniformly over a square, mapped onto a octohedron.)
  float n_ = 1.0/pParam.w ;
  vec3  ns = n_ * D.wyz - D.xzx ;

  vec4 j = p - pParam.w*pParam.w*floor(p * ns.z *ns.z);  //  mod(p,N*N)

  vec4 x_ = floor(j * ns.z)  ;
  vec4 y_ = floor(j - pParam.w * x_ ) ;    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1. - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  vec4 s0 = vec4(lessThan(b0,D.xxxx)) *2. -1.;
  vec4 s1 = vec4(lessThan(b1,D.xxxx)) *2. -1.;
  vec4 sh = vec4(lessThan(h, D.xxxx));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

#ifdef NORMALISE_GRADIENTS
  p0 *= taylorInvSqrt(dot(p0,p0));
  p1 *= taylorInvSqrt(dot(p1,p1));
  p2 *= taylorInvSqrt(dot(p2,p2));
  p3 *= taylorInvSqrt(dot(p3,p3));
#endif

// Mix
  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.);
  m = m * m;
//used to be 64.
  return 48.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), 
                                dot(p2,x2), dot(p3,x3) ) );
}

float gradientCoherentNoise3DBest(vec3 v, int seed)
{
	return gradientCoherentNoise3D(v, seed);
}


float perlin(int m_seed, float freq, float persistence, float lacunarity, int octaves, vec3 pos)
{
	float value = 0.0;
	float signal = 0.0;
	float curPersistence = 1.0;
	pos *= freq;

	for (int curOctave = 0; curOctave < octaves; ++curOctave) 
	{
		int seed = (m_seed + curOctave);// & int(0xffffffff);
		signal = gradientCoherentNoise3D(pos, seed);
		value += signal * curPersistence;
		pos *= lacunarity;
		curPersistence *= persistence;
	}

	return value;
}

float ridgedMulti(int m_seed, float freq, float lacunarity, int octaves, vec3 pos)
{
	pos *= freq;
	float value = 0.0;
	float weight = 1.0;
	float offset = 1.0;
	float gain = 2.0;
	float spectralFreq = 1.0;

	for (int curOctave = 0; curOctave < octaves; ++curOctave) 
	{
		int seed = (m_seed + curOctave) & 0x7fffffff;
		float signal = gradientCoherentNoise3D(pos, seed);
		signal = abs(signal);
		signal = offset - signal;
		signal *= signal;
		signal *= weight;
		weight = clamp(signal * gain, 0.0, 1.0);
		value += (signal * 1.0 / spectralFreq);
		spectralFreq *= lacunarity;
		pos *= lacunarity;
	}

	return (value * 1.25) - 1.0;
}

float ridgedMultiBest(int m_seed, float freq, float lacunarity, int octaves, vec3 pos)
{
	pos *= freq;
	float value = 0.0;
	float weight = 1.0;
	float offset = 1.0;
	float gain = 2.0;
	float spectralFreq = 1.0;

	for (int curOctave = 0; curOctave < octaves; ++curOctave) 
	{
		int seed = (m_seed + curOctave) & 0x7fffffff;
		float signal = gradientCoherentNoise3DBest(pos, seed);
		signal = abs(signal);
		signal = offset - signal;
		signal *= signal;
		signal *= weight;
		weight = clamp(signal * gain, 0.0, 1.0);
		value += (signal * 1.0 / spectralFreq);
		spectralFreq *= lacunarity;
		pos *= lacunarity;
	}

	return (value * 1.25) - 1.0;
}

vec3 turbulence(vec3 pos, int seed, float freq, float power, int roughness)
{
	vec3 pos0 = pos + vec3(12414.0 / 65536.0, 65124.0 / 65536.0, 31337.0 / 65536.0);
	vec3 pos1 = pos + vec3(26519.0 / 65536.0, 18128.0 / 65536.0, 60493.0 / 65536.0);
	vec3 pos2 = pos + vec3(53820.0 / 65536.0, 11213.0 / 65536.0, 44845.0 / 65536.0);

	return pos + power * vec3(
		perlin(seed    , freq, DEFAULT_PERLIN_PERSISTENCE, DEFAULT_PERLIN_LACUNARITY, roughness, pos0), // std
		perlin(seed + 1, freq, DEFAULT_PERLIN_PERSISTENCE, DEFAULT_PERLIN_LACUNARITY, roughness, pos1), // std
		perlin(seed + 2, freq, DEFAULT_PERLIN_PERSISTENCE, DEFAULT_PERLIN_LACUNARITY, roughness, pos2) // std
	);
}

float curve(float source, float[10] inPoints, float[10] outPoints, int numPoints)
{
	const int numPointsMinusOne = numPoints - 1;

	int indexPos;

	for (indexPos = 0; indexPos <= numPointsMinusOne; ++indexPos)
	{
		if (source < inPoints[indexPos])
			break;
	}

	indexPos = clamp(indexPos, 0, numPointsMinusOne);
	
	int index0 = clamp(indexPos - 2, 0, numPointsMinusOne);
	int index1 = clamp(indexPos - 1, 0, numPointsMinusOne);
	int index2 = clamp(indexPos    , 0, numPointsMinusOne);
	int index3 = clamp(indexPos + 1, 0, numPointsMinusOne);
	
	if (index1 == index2)
		return outPoints[index1];
	
	float input0 = inPoints[index1];
	float input1 = inPoints[index2];
	float alpha = (source - input0) / (input1 - input0);
	
	return cubicInterp(outPoints[index0], outPoints[index1], outPoints[index2], outPoints[index3], alpha);
}

float voronoiWithDistance(int seed, float freq, float disp, vec3 pos)
{
	//return 0.0;

	pos *= freq;
	ivec3 posInt = ivec3(pos) - ivec3(step(vec3(0, 0, 0), -pos));
	float minDist = 2147483647.0;
	vec3 candPos = vec3(0, 0, 0);
	
	for (int x = -2; x <= 2; ++x)
	{
		for (int y = -2; y <= 2; ++y)
		{
			for (int z = -2; z <= 2; ++z)
			{
				ivec3 currVec = ivec3(x, y, z) + posInt;
				vec3 tempPos = currVec + vec3(
					valueNoise3D(currVec, seed), 
					valueNoise3D(currVec, seed + 1), 
					valueNoise3D(currVec, seed + 2)
				);
				vec3 distVec = tempPos - pos;
				float dist = dot(distVec, distVec);

				if (dist < minDist) 
				{
					minDist = dist;
					candPos = tempPos;
				}
			}
		}
	}

	vec3 distVec = candPos - pos;
	float value = length(distVec) * SQRT_3 - 1.0;
	return value + disp * valueNoise3D(ivec3(floor(candPos)), seed);
}

float billow(int m_seed, float freq, float persistence, float lacunarity, int octaves, vec3 pos)
{
	float value = 0.0;
	float curPersistence = 1.0;
	
	pos *= freq;

	for (int curOctave = 0; curOctave < octaves; ++curOctave) 
	{
		int seed = (m_seed + curOctave);// & int(0xffffffff);
		float signal = gradientCoherentNoise3D(pos, seed);
		signal = 2.0 * abs(signal) - 1.0;
		value += signal * curPersistence;
		pos *= lacunarity;
		curPersistence *= persistence;
	}
	
	return value + 0.5;
}

float billowBest(int m_seed, float freq, float persistence, float lacunarity, int octaves, vec3 pos)
{
	float value = 0.0;
	float curPersistence = 1.0;
	
	pos *= freq;

	for (int curOctave = 0; curOctave < octaves; ++curOctave) 
	{
		int seed = (m_seed + curOctave);// & int(0xffffffff);
		float signal = gradientCoherentNoise3DBest(pos, seed);
		signal = 2.0 * abs(signal) - 1.0;
		value += signal * curPersistence;
		pos *= lacunarity;
		curPersistence *= persistence;
	}
	
	return value + 0.5;
}

float terrace(float value, float[6] points, int numPoints)
{
	int maxInd = numPoints - 1;
	
	int indexPos;
	for (indexPos = 0; indexPos <= maxInd; ++indexPos) 
	{
		if (value < points[indexPos]) 
			break;
	}
	
	int index0 = clamp(indexPos - 1, 0, maxInd);
	int index1 = clamp(indexPos    , 0, maxInd);

	if (index0 == index1)
		return points[index1];

	float value0 = points[index0];
	float value1 = points[index1];
	float alpha = (value - value0) / (value1 - value0);
	return mix(value0, value1, alpha * alpha);
}

float expoFunc(float mant, float expo)
{
	return pow(abs(0.5*mant + 0.5), expo) * 2.0 - 1.0;
}

float getBaseContinentDef(vec3 pos, int m_seed)
{	
	float baseContinentDef_pe0 = perlin(m_seed, CONTINENT_FREQUENCY, 0.5, CONTINENT_LACUNARITY, 14, pos);
	float baseContinentDef_cu = curve(baseContinentDef_pe0, BASE_CONTINENT_CURVE_POINTS_IN, BASE_CONTINENT_CURVE_POINTS_OUT, NUM_BASE_CONTINENT_CURVE_POINTS);
	float baseContinentDef_pe1 = perlin(m_seed + 1, CONTINENT_FREQUENCY * 4.34375, 0.5, CONTINENT_LACUNARITY, 11, pos);
	float baseContinentDef_sb = baseContinentDef_pe1 * 0.375 + 0.625;
	float baseContinentDef_mi = min(baseContinentDef_sb, baseContinentDef_cu);
	float baseContinentDef = clamp(baseContinentDef_mi, -1.0, 1.0);
	return baseContinentDef;
}

float getContinentDef(vec3 pos, int m_seed)
{
	float baseContinentDef = getBaseContinentDef(pos, m_seed);
	vec3 pos_continentDef_tu0 = turbulence(pos, m_seed + 10, CONTINENT_FREQUENCY * 15.25, CONTINENT_FREQUENCY / 113.75, 13);
	vec3 pos_continentDef_tu1 = turbulence(pos_continentDef_tu0, m_seed + 11, CONTINENT_FREQUENCY * 47.25, CONTINENT_FREQUENCY / 433.75, 12);
	vec3 pos_continentDef_tu2 = turbulence(pos_continentDef_tu1, m_seed + 12, CONTINENT_FREQUENCY * 95.25, CONTINENT_FREQUENCY / 1019.75, 11);
	float continentDef_tu2 = getBaseContinentDef(pos_continentDef_tu2, m_seed);
	float continentDef = select(baseContinentDef, continentDef_tu2, baseContinentDef, 0.0625, SEA_LEVEL - 0.0375, SEA_LEVEL + 1000.0375);
	return continentDef;
}

float getMountainBaseDef_b1(vec3 pos, int m_seed)
{
	float mountainBaseDef_rm0 = ridgedMulti(m_seed + 30, 1723.0, MOUNTAIN_LACUNARITY, 4, pos); // std
	float mountainBaseDef_sb0 = mountainBaseDef_rm0 * 0.5 + 0.375;
	float mountainBaseDef_rm1 = ridgedMultiBest(m_seed + 31, 367.0, MOUNTAIN_LACUNARITY, 1, pos); // best
	float mountainBaseDef_sb1 = mountainBaseDef_rm1 * -2.0 - 0.5;
	float mountainBaseDef_co = -1.0;
	float mountainBaseDef_bl = mix(mountainBaseDef_co, mountainBaseDef_sb0, 0.5*mountainBaseDef_sb1 + 0.5);
	return mountainBaseDef_bl;
}

float getMountainousHigh_ma(vec3 pos, int m_seed)
{
	float mountainousHigh_rm0 = ridgedMultiBest(m_seed + 40, 2371.0, MOUNTAIN_LACUNARITY, 3, pos); // best
	float mountainousHigh_rm1 = ridgedMultiBest(m_seed + 41, 2341.0, MOUNTAIN_LACUNARITY, 3, pos); // best
	float mountainousHigh_ma = max(mountainousHigh_rm0, mountainousHigh_rm1);
	return mountainousHigh_ma;
}

float getHillyTerrain_ex(vec3 pos, int m_seed)
{	
	float hillyTerrain_bi = billow(m_seed + 60, 1663.0, 0.5, HILLS_LACUNARITY, 6, pos); // best
	float hillyTerrain_sb0 = hillyTerrain_bi * 0.5 + 0.5;
	float hillyTerrain_rm = ridgedMultiBest(m_seed + 61, 367.5, HILLS_LACUNARITY, 1, pos); // best
	float hillyTerrain_sb1 = hillyTerrain_rm * -2.0 - 0.5;
	float hillyTerrain_co = 1.0;
	float hillyTerrain_bl = mix(hillyTerrain_co, hillyTerrain_sb1, (hillyTerrain_sb0 + 1.0) / 2.0);
	float hillyTerrain_sb2 = hillyTerrain_bl * 0.75 - 0.25;
	float hillyTerrain_ex = expoFunc(hillyTerrain_sb2, 1.375);
	return hillyTerrain_ex;
}

float getScaledPlainsTerrain(vec3 pos, int m_seed)
{
	float plainsTerrain_bi0 = billowBest(m_seed + 70, 1097.5, 0.5, PLAINS_LACUNARITY, 8, pos); // best
	float plainsTerrain_sb0 = plainsTerrain_bi0 * 0.5 + 0.5;
	float plainsTerrain_bi1 = billowBest(m_seed + 71, 1319.5, 0.5, PLAINS_LACUNARITY, 8, pos); // best
	float plainsTerrain_sb1 = plainsTerrain_bi1 * 0.5 + 0.5;
	float plainsTerrain_mu = plainsTerrain_sb0 * plainsTerrain_sb1;
	return (plainsTerrain_mu * 2.0 - 1.0) * 0.00390625 + 0.0078125;
}

float getBadlandsCliffs_te(vec3 pos, int m_seed)
{
	float badlandsCliffs_pe = perlin(m_seed + 90, CONTINENT_FREQUENCY * 839.0, 0.5, BADLANDS_LACUNARITY, 6, pos);
	float badlandsCliffs_cu = curve(badlandsCliffs_pe, BADLANDS_CLIFF_CURVE_POINTS_IN, BADLANDS_CLIFF_CURVE_POINTS_OUT, NUM_BADLANDS_CLIFF_CURVE_POINTS);
	float badlandsCliffs_cl = clamp(badlandsCliffs_cu, -999.125, 0.875);
	float badlandsCliffs_te = terrace(badlandsCliffs_cl, BADLANDS_CLIFF_TERRACE_POINTS, NUM_BADLANDS_CLIFF_TERRACE_POINTS);
	return badlandsCliffs_te;
}

float getScaledBadlandsTerrain(vec3 pos, int m_seed)
{	
	float badlandsSand_rm = ridgedMultiBest(m_seed + 80, 6163.5, BADLANDS_LACUNARITY, 1, pos); // best
	float badlandsSand_sb0 = badlandsSand_rm * 0.875;
	float badlandsSand_vo = voronoiWithDistance(m_seed + 81, 16183.25, 0.0, pos); 
	//float badlandsSand_vo = 0.0; // HACK
	float badlandsSand_sb1 = badlandsSand_vo * 0.25 + 0.25;
	float badlandsSand = badlandsSand_sb0 + badlandsSand_sb1;
	
	vec3 pos_badlandsCliffs_tu0 = turbulence(pos, m_seed + 91, 16111.0, 1.0 / 141539.0 * BADLANDS_TWIST, 3);
	vec3 pos_badlandsCliffs_tu1 =  turbulence(pos_badlandsCliffs_tu0, m_seed + 92, 36107.0, 1.0 / 211543.0 * BADLANDS_TWIST, 3);
	float badlandsCliffs = getBadlandsCliffs_te(pos_badlandsCliffs_tu1, m_seed);

	float badlandsTerrain_sb = badlandsSand * 0.25 - 0.75;
	return max(badlandsCliffs, badlandsTerrain_sb) * 0.0625 + 0.0625;
}

float getScaledRiverPositions(vec3 pos, int m_seed)
{
	vec3 pos_riverPositions_tu = turbulence(pos, m_seed + 102, 9.25, 1.0 / 57.75, 6);

	float riverPositions_rm0 = ridgedMultiBest(m_seed + 100, 18.75, CONTINENT_LACUNARITY, 1, pos_riverPositions_tu); // best
	float riverPositions_cu0 = curve(riverPositions_rm0, RIVER_POSITIONS_CURVE0_POINTS_IN, RIVER_POSITIONS_CURVE0_POINTS_OUT, NUM_RIVER_POSITIONS_CURVE0_POINTS);
	float riverPositions_rm1 = ridgedMultiBest(m_seed + 101, 43.25, CONTINENT_LACUNARITY, 1, pos_riverPositions_tu); // best
	float riverPositions_cu1 = curve(riverPositions_rm1, RIVER_POSITIONS_CURVE1_POINTS_IN, RIVER_POSITIONS_CURVE1_POINTS_OUT, NUM_RIVER_POSITIONS_CURVE1_POINTS);
	float riverPositions_mi = min(riverPositions_cu0, riverPositions_cu1);

	return riverPositions_mi * (RIVER_DEPTH / 2.0) - RIVER_DEPTH / 2.0;
}

float getScaledMountainousTerrain(vec3 pos, int m_seed)
{
	vec3 pos_mountainBaseDef_tu0 = turbulence(pos, m_seed + 32, 1337.0, 1.0 / 6730.0 * MOUNTAINS_TWIST, 4);
	vec3 pos_mountainBaseDef_tu1 = turbulence(pos_mountainBaseDef_tu0, m_seed + 33, 21221.0, 1.0 / 120157.0 * MOUNTAINS_TWIST, 6);
	float mountainBaseDef = getMountainBaseDef_b1(pos_mountainBaseDef_tu1, m_seed);

	vec3 pos_mountainousHigh_tu = turbulence(pos, m_seed + 42, 31511.0, 1.0 / 180371.0 * MOUNTAINS_TWIST, 4);
	float mountainousHigh = getMountainousHigh_ma(pos_mountainousHigh_tu, m_seed);

	float mountainousLow_rm0 = ridgedMultiBest(m_seed + 50, 1381.0, MOUNTAIN_LACUNARITY, 8, pos); // best
	float mountainousLow_rm1 = ridgedMultiBest(m_seed + 51, 1427.0, MOUNTAIN_LACUNARITY, 8, pos); // best
	float mountainousLow = mountainousLow_rm0 * mountainousLow_rm1;
	
	float mountainousTerrain_sb0 = mountainousLow * 0.03125 - 0.96875;
	float mountainousTerrain_sb1 = mountainousHigh * 0.25 + 0.25;
	float mountainousTerrain_ad = mountainousTerrain_sb1 + mountainBaseDef;
	float mountainousTerrain_se = select(mountainousTerrain_sb0, mountainousTerrain_ad, mountainBaseDef, 0.5, -0.5, 999.5);
	float mountainousTerrain_sb2 = mountainousTerrain_se * 0.8;
	float mountainousTerrain = expoFunc(mountainousTerrain_sb2, MOUNTAIN_GLACIATION);

	float scaledMountainousTerrain_sb0 = mountainousTerrain * 0.125 + 0.125;
	float scaledMountainousTerrain_pe = perlin(m_seed + 110, 14.5, 0.5, MOUNTAIN_LACUNARITY, 6, pos);
	float scaledMountainousTerrain_ex = expoFunc(scaledMountainousTerrain_pe, 1.25);
	float scaledMountainousTerrain_sb1 = scaledMountainousTerrain_ex * 0.25 + 1.0;
	return scaledMountainousTerrain_sb0 * scaledMountainousTerrain_sb1;
}

float getScaledHillyTerrain(vec3 pos, int m_seed)
{
	vec3 pos_hillyTerrain_tu0 = turbulence(pos, m_seed + 62, 1531.0, 1.0 / 16921.0 * HILLS_TWIST, 4);
	vec3 pos_hillyTerrain_tu1 = turbulence(pos_hillyTerrain_tu0, m_seed + 63, 21617.0, 1.0 / 117529.0 * HILLS_TWIST, 6);
	float hillyTerrain = getHillyTerrain_ex(pos_hillyTerrain_tu1, m_seed);

	float scaledHillyTerrain_sb0 = hillyTerrain * 0.0625 + 0.0625;
	float scaledHillyTerrain_pe = perlin(m_seed + 120, 13.5, 0.5, HILLS_LACUNARITY, 6, pos);
	float scaledHillyTerrain_ex = expoFunc(scaledHillyTerrain_pe, 1.25);
	float scaledHillyTerrain_sb1 = scaledHillyTerrain_ex * 0.5 + 1.5;
	return scaledHillyTerrain_sb0 * scaledHillyTerrain_sb1;
}

float getTerrainTypeDef(vec3 pos, int m_seed)
{
	vec3 pos_terrainTypeDef_tu = turbulence(pos, m_seed + 20, CONTINENT_FREQUENCY * 18.125, CONTINENT_FREQUENCY / 20.59375 * TERRAIN_OFFSET, 3);
	float terrainTypeDef_tu = getContinentDef(pos_terrainTypeDef_tu, m_seed);
	return terrace(terrainTypeDef_tu, TERRAIN_TYPE_TERRACE_POINTS, NUM_TERRAIN_TYPE_TERRACE_POINTS);
}

float getBaseContinentElev(vec3 pos, int m_seed, float continentDef)
{
	float continentalShelf_te = terrace(continentDef, CONTINENTAL_SHELF_TERRACE_POINTS, NUM_CONTINENTAL_SHELF_TERRACE_POINTS);
	float continentalShelf_rm = ridgedMultiBest(m_seed + 130, CONTINENT_FREQUENCY * 4.375, CONTINENT_LACUNARITY, 16, pos); // best
	float continentalShelf_sb = continentalShelf_rm * -0.125 - 0.125;
	float continentalShelf_cl = clamp(continentalShelf_te, -0.75, SEA_LEVEL);
	float continentalShelf_ad = continentalShelf_sb + continentalShelf_cl;
	float continentalShelf = continentalShelf_ad;

	float baseContinentElev_sb = continentDef * CONTINENT_HEIGHT_SCALE;

	float baseContinentElev_se = select(
		baseContinentElev_sb, 
		continentalShelf, 
		continentDef, 
		0.03125, SHELF_LEVEL - 1000.0, SHELF_LEVEL
	);

	return baseContinentElev_se;
}

float getContinentsWithMountains(vec3 pos, int m_seed, float baseContinentElev, float continentDef)
{
	float scaledPlainsTerrain = getScaledPlainsTerrain(pos, m_seed);
	float scaledHillyTerrain = getScaledHillyTerrain(pos, m_seed);
	float continentsWithPlains = baseContinentElev + scaledPlainsTerrain;
	float terrainTypeDef = getTerrainTypeDef(pos, m_seed);
	float scaledMountainousTerrain = getScaledMountainousTerrain(pos, m_seed);

	float continentsWithHills_ad = baseContinentElev + scaledHillyTerrain;
	float continentsWithHills = select(continentsWithPlains, continentsWithHills_ad, terrainTypeDef, 0.25, 1.0 - HILLS_AMOUNT, 1001.0 - HILLS_AMOUNT);

	float continentsWithMountains_ad0 = baseContinentElev + scaledMountainousTerrain;
	float continentsWithMountains_cu = curve(continentDef, CONTINENTS_WITH_MOUNTAINS_CURVE_POINTS_IN, CONTINENTS_WITH_MOUNTAINS_CURVE_POINTS_OUT, NUM_CONTINENTS_WITH_MOUNTAINS_CURVE_POINTS);
	float continentsWithMountains_ad1 = continentsWithMountains_ad0 + continentsWithMountains_cu;
	return select(continentsWithHills, continentsWithMountains_ad1, terrainTypeDef, 0.25, 1.0 - MOUNTAINS_AMOUNT, 1001.0 - MOUNTAINS_AMOUNT);
}

float getContinentsWithBadlands(vec3 pos, int m_seed)
{
	float continentDef = getContinentDef(pos, m_seed);
	float baseContinentElev = getBaseContinentElev(pos, m_seed, continentDef);
	float scaledBadlandsTerrain = getScaledBadlandsTerrain(pos, m_seed);
	float continentsWithMountains = getContinentsWithMountains(pos, m_seed, baseContinentElev, continentDef);
	
	float continentsWithBadlands_se = select(
		continentsWithMountains, 
		baseContinentElev + scaledBadlandsTerrain, 
		perlin(m_seed + 140, 16.5, 0.5, CONTINENT_LACUNARITY, 2, pos), 
		0.25, 1.0 - BADLANDS_AMOUNT, 1001.0 - BADLANDS_AMOUNT
	);
	return max(continentsWithMountains, continentsWithBadlands_se);
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

vec4 getColourAndAltitude(vec3 pos, int m_seed)
{
	float continentsWithBadlands = getContinentsWithBadlands(pos, m_seed);
	float scaledRiverPositions = getScaledRiverPositions(pos, m_seed);

	float altitude = select(
		continentsWithBadlands, 
		continentsWithBadlands + scaledRiverPositions, 
		continentsWithBadlands, 
		CONTINENT_HEIGHT_SCALE - SEA_LEVEL, SEA_LEVEL, CONTINENT_HEIGHT_SCALE + SEA_LEVEL
	);

	return vec4(getColour(altitude), 1.0 + 0.0005*altitude);
}

