
///////////////////////////////////////////

/*
 * 2D, 3D and 4D Perlin noise, classic and simplex, in a GLSL fragment shader.
 *
 * Classic noise is implemented by the functions:
 * float noise(vec2 P)
 * float noise(vec3 P)
 * float noise(vec4 P)
 *
 * Simplex noise is implemented by the functions:
 * float snoise(vec2 P)
 * float snoise(vec3 P)
 * float snoise(vec4 P)
 *
 * Author: Stefan Gustavson ITN-LiTH (stegu@itn.liu.se) 2004-12-05
 * You may use, modify and redistribute this code free of charge,
 * provided that my name and this notice appears intact.
 */

/*
 * NOTE: there is a formal problem with the dependent texture lookups.
 * A texture coordinate of exactly 1.0 will wrap to 0.0, so strictly speaking,
 * an error occurs every 256 units of the texture domain, and the same gradient
 * is used for two adjacent noise cells. One solution is to set the texture
 * wrap mode to "CLAMP" and do the wrapping explicitly in GLSL with the "mod"
 * operator. This could also give you noise with repetition intervals other
 * than 256 without any extra cost.
 * This error is not even noticeable to the eye even if you isolate the exact
 * position in the domain where it occurs and know exactly what to look for.
 * The noise pattern is still visually correct, so I left the bug in there.
 * 
 * The value of classic 4D noise goes above 1.0 and below -1.0 at some
 * points. Not much and only very sparsely, but it happens.
 */


/*
 * "permTexture" is a 256x256 texture that is used for both the permutations
 * and the 2D and 3D gradient lookup. For details, see the main C program.
 * "simplexTexture" is a small look-up table to determine a simplex traversal
 * order for 3D and 4D simplex noise. Details are in the C program.
 * "gradTexture" is a 256x256 texture with 4D gradients, similar to
 * "permTexture" but with the permutation index in the alpha component
 * replaced by the w component of the 4D gradient.
 * 2D classic noise uses only permTexture.
 * 2D simplex noise uses permTexture and simplexTexture.
 * 3D classic noise uses only permTexture.
 * 3D simplex noise uses permTexture and simplexTexture.
 * 4D classic noise uses permTexture and gradTexture.
 * 4D simplex noise uses all three textures.
 */
uniform sampler2D uniform_permTexture;
uniform sampler1D uniform_simplexTexture;
uniform sampler2D uniform_gradTexture;

/*
 * To create offsets of one texel and one half texel in the
 * texture lookup, we need to know the texture image size.
 */
#define ONE 0.00390625
#define ONEHALF 0.001953125
// The numbers above are 1/256 and 0.5/256, change accordingly
// if you change the code to use another texture size.

/*
 * 2D simplex noise. Somewhat slower but much better looking than classic noise.
 */
float snoise(vec2 P) 
{
	// Skew and unskew factors are a bit hairy for 2D, so define them as constants
	// This is (sqrt(3.0)-1.0)/2.0
	#define F2 0.366025403784
	// This is (3.0-sqrt(3.0))/6.0
	#define G2 0.211324865405

	// Skew the (x,y) space to determine which cell of 2 simplices we're in
	float s = (P.x + P.y) * F2;   // Hairy factor for 2D skewing
	vec2 Pi = floor(P + s);
	float t = (Pi.x + Pi.y) * G2; // Hairy factor for unskewing
	vec2 P0 = Pi - t; // Unskew the cell origin back to (x,y) space
	Pi = Pi * ONE + ONEHALF; // Integer part, scaled and offset for texture lookup

	vec2 Pf0 = P - P0;  // The x,y distances from the cell origin

	// For the 2D case, the simplex shape is an equilateral triangle.
	// Find out whether we are above or below the x=y diagonal to
	// determine which of the two triangles we're in.
	vec2 o1;
	if (Pf0.x > Pf0.y) 
		o1 = vec2(1.0, 0.0); // +x, +y traversal order
	else 
		o1 = vec2(0.0, 1.0); // +y, +x traversal order

	// Noise contribution from simplex origin
	vec2 grad0 = texture2D(uniform_permTexture, Pi).rg * 4.0 - 1.0;
	float t0 = 0.5 - dot(Pf0, Pf0);
	float n0;
	if (t0 < 0.0) 
	{
		n0 = 0.0;
	}
	else 
	{
		t0 *= t0;
		n0 = t0 * t0 * dot(grad0, Pf0);
	}

	// Noise contribution from middle corner
	vec2 Pf1 = Pf0 - o1 + G2;
	vec2 grad1 = texture2D(uniform_permTexture, Pi + o1*ONE).rg * 4.0 - 1.0;
	float t1 = 0.5 - dot(Pf1, Pf1);
	float n1;
	if (t1 < 0.0) 
	{
		n1 = 0.0;
	}
	else 
	{
		t1 *= t1;
		n1 = t1 * t1 * dot(grad1, Pf1);
	}
  
	// Noise contribution from last corner
	vec2 Pf2 = Pf0 - vec2(1.0-2.0*G2);
	vec2 grad2 = texture2D(uniform_permTexture, Pi + vec2(ONE, ONE)).rg * 4.0 - 1.0;
	float t2 = 0.5 - dot(Pf2, Pf2);
	float n2;
	if (t2 < 0.0) 
		n2 = 0.0;
	else 
	{
		t2 *= t2;
		n2 = t2 * t2 * dot(grad2, Pf2);
	}

	// Sum up and scale the result to cover the range [-1,1]
	return 70.0 * (n0 + n1 + n2);
}


/*
 * 3D simplex noise. Comparable in speed to classic noise, better looking.
 */

float snoise(vec3 P) 
{
	// The skewing and unskewing factors are much simpler for the 3D case
	#define F3 0.333333333333
	#define G3 0.166666666667

	// Skew the (x,y,z) space to determine which cell of 6 simplices we're in
	float s = (P.x + P.y + P.z) * F3; // Factor for 3D skewing
	vec3 Pi = floor(P + s);
	float t = (Pi.x + Pi.y + Pi.z) * G3;
	vec3 P0 = Pi - t; // Unskew the cell origin back to (x,y,z) space
	Pi = Pi * ONE + ONEHALF; // Integer part, scaled and offset for texture lookup

	vec3 Pf0 = P - P0;  // The x,y distances from the cell origin

	// For the 3D case, the simplex shape is a slightly irregular tetrahedron.
	// To find out which of the six possible tetrahedra we're in, we need to
	// determine the magnitude ordering of x, y and z components of Pf0.
	// The method below is explained briefly in the C code. It uses a small
	// 1D texture as a lookup table. The table is designed to work for both
	// 3D and 4D noise, so only 8 (only 6, actually) of the 64 indices are
	// used here.
	float c1 = (Pf0.x > Pf0.y) ? 0.5078125 : 0.0078125; // 1/2 + 1/128
	float c2 = (Pf0.x > Pf0.z) ? 0.25 : 0.0;
	float c3 = (Pf0.y > Pf0.z) ? 0.125 : 0.0;
	float sindex = c1 + c2 + c3;
	vec3 offsets = texture(uniform_simplexTexture, sindex).rgb;
	vec3 o1 = step(0.375, offsets);
	vec3 o2 = step(0.125, offsets);

	// Noise contribution from simplex origin
	float perm0 = texture2D(uniform_permTexture, Pi.xy).a;
	vec3  grad0 = texture2D(uniform_permTexture, vec2(perm0, Pi.z)).rgb * 4.0 - 1.0;
	float t0 = 0.6 - dot(Pf0, Pf0);
	float n0;
	if (t0 < 0.0) 
		n0 = 0.0;
	else 
	{
		t0 *= t0;
		n0 = t0 * t0 * dot(grad0, Pf0);
	}

	// Noise contribution from second corner
	vec3 Pf1 = Pf0 - o1 + G3;
	float perm1 = texture2D(uniform_permTexture, Pi.xy + o1.xy*ONE).a;
	vec3  grad1 = texture2D(uniform_permTexture, vec2(perm1, Pi.z + o1.z*ONE)).rgb * 4.0 - 1.0;
	float t1 = 0.6 - dot(Pf1, Pf1);
	float n1;
	if (t1 < 0.0) 
		n1 = 0.0;
	else 
	{
		t1 *= t1;
		n1 = t1 * t1 * dot(grad1, Pf1);
	}
  
	// Noise contribution from third corner
	vec3 Pf2 = Pf0 - o2 + 2.0 * G3;
	float perm2 = texture2D(uniform_permTexture, Pi.xy + o2.xy*ONE).a;
	vec3  grad2 = texture2D(uniform_permTexture, vec2(perm2, Pi.z + o2.z*ONE)).rgb * 4.0 - 1.0;
	float t2 = 0.6 - dot(Pf2, Pf2);
	float n2;
	if (t2 < 0.0) 
		n2 = 0.0;
	else 
	{
		t2 *= t2;
		n2 = t2 * t2 * dot(grad2, Pf2);
	}
  
	// Noise contribution from last corner
	vec3 Pf3 = Pf0 - vec3(1.0-3.0*G3);
	float perm3 = texture2D(uniform_permTexture, Pi.xy + vec2(ONE, ONE)).a;
	vec3  grad3 = texture2D(uniform_permTexture, vec2(perm3, Pi.z + ONE)).rgb * 4.0 - 1.0;
	float t3 = 0.6 - dot(Pf3, Pf3);
	float n3;
	if(t3 < 0.0) 
		n3 = 0.0;
	else 
	{
		t3 *= t3;
		n3 = t3 * t3 * dot(grad3, Pf3);
	}

	// Sum up and scale the result to cover the range [-1,1]
	return 32.0 * (n0 + n1 + n2 + n3);
}

/*
* 4D simplex noise. A lot faster than classic 4D noise, and better looking.
*/

float snoise(vec4 P) 
{
	// The skewing and unskewing factors are hairy again for the 4D case
	// This is (sqrt(5.0)-1.0)/4.0
	#define F4 0.309016994375
	// This is (5.0-sqrt(5.0))/20.0
	#define G4 0.138196601125

	// Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
	float s = (P.x + P.y + P.z + P.w) * F4; // Factor for 4D skewing
	vec4 Pi = floor(P + s);
	float t = (Pi.x + Pi.y + Pi.z + Pi.w) * G4;
	vec4 P0 = Pi - t; // Unskew the cell origin back to (x,y,z,w) space
	Pi = Pi * ONE + ONEHALF; // Integer part, scaled and offset for texture lookup

	vec4 Pf0 = P - P0;  // The x,y distances from the cell origin

	// For the 4D case, the simplex is a 4D shape I won't even try to describe.
	// To find out which of the 24 possible simplices we're in, we need to
	// determine the magnitude ordering of x, y, z and w components of Pf0.
	// The method below is presented without explanation. It uses a small 1D
	// texture as a lookup table. The table is designed to work for both
	// 3D and 4D noise and contains 64 indices, of which only 24 are actually
	// used. An extension to 5D would require a larger texture here.
	float c1 = (Pf0.x > Pf0.y) ? 0.5078125 : 0.0078125; // 1/2 + 1/128
	float c2 = (Pf0.x > Pf0.z) ? 0.25 : 0.0;
	float c3 = (Pf0.y > Pf0.z) ? 0.125 : 0.0;
	float c4 = (Pf0.x > Pf0.w) ? 0.0625 : 0.0;
	float c5 = (Pf0.y > Pf0.w) ? 0.03125 : 0.0;
	float c6 = (Pf0.z > Pf0.w) ? 0.015625 : 0.0;
	float sindex = c1 + c2 + c3 + c4 + c5 + c6;
	vec4 offsets = texture(uniform_simplexTexture, sindex).rgba;
	vec4 o1 = step(0.625, offsets);
	vec4 o2 = step(0.375, offsets);
	vec4 o3 = step(0.125, offsets);

	// Noise contribution from simplex origin
	float perm0xy = texture2D(uniform_permTexture, Pi.xy).a;
	float perm0zw = texture2D(uniform_permTexture, Pi.zw).a;
	vec4  grad0 = texture2D(uniform_gradTexture, vec2(perm0xy, perm0zw)).rgba * 4.0 - 1.0;
	float t0 = 0.6 - dot(Pf0, Pf0);
	float n0;
	if (t0 < 0.0) n0 = 0.0;
	else {
	t0 *= t0;
	n0 = t0 * t0 * dot(grad0, Pf0);
	}

	// Noise contribution from second corner
	vec4 Pf1 = Pf0 - o1 + G4;
	o1 = o1 * ONE;
	float perm1xy = texture2D(uniform_permTexture, Pi.xy + o1.xy).a;
	float perm1zw = texture2D(uniform_permTexture, Pi.zw + o1.zw).a;
	vec4  grad1 = texture2D(uniform_gradTexture, vec2(perm1xy, perm1zw)).rgba * 4.0 - 1.0;
	float t1 = 0.6 - dot(Pf1, Pf1);
	float n1;
	if (t1 < 0.0) n1 = 0.0;
	else {
	t1 *= t1;
	n1 = t1 * t1 * dot(grad1, Pf1);
	}
  
	// Noise contribution from third corner
	vec4 Pf2 = Pf0 - o2 + 2.0 * G4;
	o2 = o2 * ONE;
	float perm2xy = texture2D(uniform_permTexture, Pi.xy + o2.xy).a;
	float perm2zw = texture2D(uniform_permTexture, Pi.zw + o2.zw).a;
	vec4  grad2 = texture2D(uniform_gradTexture, vec2(perm2xy, perm2zw)).rgba * 4.0 - 1.0;
	float t2 = 0.6 - dot(Pf2, Pf2);
	float n2;
	if (t2 < 0.0) n2 = 0.0;
	else {
	t2 *= t2;
	n2 = t2 * t2 * dot(grad2, Pf2);
	}
  
	// Noise contribution from fourth corner
	vec4 Pf3 = Pf0 - o3 + 3.0 * G4;
	o3 = o3 * ONE;
	float perm3xy = texture2D(uniform_permTexture, Pi.xy + o3.xy).a;
	float perm3zw = texture2D(uniform_permTexture, Pi.zw + o3.zw).a;
	vec4  grad3 = texture2D(uniform_gradTexture, vec2(perm3xy, perm3zw)).rgba * 4.0 - 1.0;
	float t3 = 0.6 - dot(Pf3, Pf3);
	float n3;
	if (t3 < 0.0) n3 = 0.0;
	else {
	t3 *= t3;
	n3 = t3 * t3 * dot(grad3, Pf3);
	}
  
	// Noise contribution from last corner
	vec4 Pf4 = Pf0 - vec4(1.0-4.0*G4);
	float perm4xy = texture2D(uniform_permTexture, Pi.xy + vec2(ONE, ONE)).a;
	float perm4zw = texture2D(uniform_permTexture, Pi.zw + vec2(ONE, ONE)).a;
	vec4  grad4 = texture2D(uniform_gradTexture, vec2(perm4xy, perm4zw)).rgba * 4.0 - 1.0;
	float t4 = 0.6 - dot(Pf4, Pf4);
	float n4;
	if(t4 < 0.0) n4 = 0.0;
	else {
	t4 *= t4;
	n4 = t4 * t4 * dot(grad4, Pf4);
	}

	// Sum up and scale the result to cover the range [-1,1]
	return 27.0 * (n0 + n1 + n2 + n3 + n4);
}
