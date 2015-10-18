/**
 * Precomputed Atmospheric Scattering
 * Copyright (c) 2008 INRIA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Author: Eric Bruneton
 */

#define FIX

layout (shared, row_major) uniform DynamicDrawUniformBlock
{
	mat4 projInverse;
	mat4 viewInverse;
	vec3 cameraPos;
	vec3 sunDirection;
	float exposure;
	float iSun;
};

#ifdef _VERTEX_

out vec2 coords;
out vec3 ray;

const vec2 data[4] = vec2[]
(
	vec2(-1.0, 1.0),
	vec2(-1.0, -1.0),
	vec2(1.0, 1.0),
	vec2(1.0, -1.0)
);

void main()
{
    coords = data[gl_VertexID] * 0.5 + 0.5;
	vec4 v4_vertex = vec4(data[gl_VertexID], 0.0, 1.0);
    ray = (viewInverse * vec4((projInverse * v4_vertex).xyz, 0.0)).xyz;
    gl_Position = v4_vertex;
}

#else // Fragment

const float M_PI = 3.141592657;

layout (shared) uniform PlanetUniformBlock
{
	float Rg;
	float Rt;
	float RL;
};

layout (shared) uniform AtmosphereUniformBlock
{
	vec3 betaR;
	float HR;
	float HM;
	vec3 betaMSca;
	vec3 betaMEx;
	float mieG;
};

layout (shared) uniform ResUniformBlock
{
	int RES_R;
	int RES_MU;
	int RES_MU_S;
	int RES_NU;
	int TRANSMITTANCE_W;
	int TRANSMITTANCE_H;
	int TRANSMITTANCE_INTEGRAL_SAMPLES;
	int IRRADIANCE_INTEGRAL_SAMPLES;
	int INSCATTER_INTEGRAL_SAMPLES;
	int INSCATTER_SPHERICAL_INTEGRAL_SAMPLES;
	int SKY_W;
	int SKY_H;
};

uniform sampler2D transmittanceSampler;
uniform sampler2D reflectanceSampler;//ground reflectance texture
uniform sampler2D irradianceSampler;//precomputed skylight irradiance (E table)
uniform sampler3D inscatterSampler;//precomputed inscattered light (S table)

in vec2 coords;
in vec3 ray;
out vec4 fragColour;

vec2 getTransmittanceUV(float r, float mu) 
{
    return vec2(
		atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5, 
		sqrt((r - Rg) / (Rt - Rg))
	);
}

// Rayleigh phase function
float phaseFunctionR(float mu) 
{
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu);
}

// Mie phase function
float phaseFunctionM(float mu) 
{
	return 1.5 * 1.0 / (4.0 * M_PI) * (1.0 - mieG*mieG) * pow(1.0 + (mieG*mieG) - 2.0*mieG*mu, -3.0/2.0) * (1.0 + mu*mu) / (2.0 + mieG*mieG);
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu)
// (mu=cos(view zenith angle)), intersections with ground ignored
vec3 transmittance(float r, float mu) 
{
    return texture2D(transmittanceSampler, getTransmittanceUV(r, mu)).rgb;
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu)
// (mu=cos(view zenith angle)), or zero if ray intersects ground
vec3 transmittanceWithShadow(float r, float mu) 
{
    return mu < -sqrt(1.0 - (Rg / r) * (Rg / r)) ? vec3(0.0) : transmittance(r, mu);
}

vec3 irradiance(sampler2D sampler, float r, float muS) 
{
	return texture(sampler, vec2((muS + 0.2)/(1.0 + 0.2), (r - Rg)/(Rt - Rg))).rgb;
}

vec4 texture4D(sampler3D table, float r, float mu, float muS, float nu)
{
    float H = sqrt(Rt * Rt - Rg * Rg);
    float rho = sqrt(r * r - Rg * Rg);
    float rmu = r * mu;
    float delta = rmu * rmu - r * r + Rg * Rg;
    vec4 cst = rmu < 0.0 && delta > 0.0 ? vec4(1.0, 0.0, 0.0, 0.5 - 0.5 / float(RES_MU)) : vec4(-1.0, H * H, H, 0.5 + 0.5 / float(RES_MU));
	float uR = 0.5 / float(RES_R) + rho / H * (1.0 - 1.0 / float(RES_R));
    float uMu = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5 - 1.0 / float(RES_MU));
    // paper formula
    //float uMuS = 0.5 / float(RES_MU_S) + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / float(RES_MU_S));
    // better formula
    float uMuS = 0.5 / float(RES_MU_S) + (atan(max(muS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / float(RES_MU_S));
    float lerp = (nu + 1.0) / 2.0 * (float(RES_NU) - 1.0);
    float uNu = floor(lerp);
    lerp = lerp - uNu;

	return mix(
		texture(table, vec3((uNu + uMuS) / float(RES_NU), uMu, uR)),
		texture(table, vec3((uNu + uMuS + 1.0) / float(RES_NU), uMu, uR)),
		lerp
	);
}

// optical depth for ray (r,mu) of length d, using analytic formula
// (mu=cos(view zenith angle)), intersections with ground ignored
// H=height scale of exponential density function
float opticalDepth(float H, float r, float mu, float d) 
{
    float a = sqrt((0.5/H)*r);
    vec2 a01 = a*vec2(mu, mu + d / r);
    vec2 a01s = sign(a01);
    vec2 a01sq = a01*a01;
    float x = a01s.y > a01s.x ? exp(a01sq.x) : 0.0;
    vec2 y = a01s / (2.3193*abs(a01) + sqrt(1.52*a01sq + 4.0)) * vec2(1.0, exp(-d/H*(d/(2.0*r)+mu)));
    return sqrt((6.2831*H)*r) * exp((Rg-r)/H) * (x + dot(y, vec2(1.0, -1.0)));
}

// approximated single Mie scattering (cf. approximate Cm in paragraph "Angular precision")
vec3 getMie(vec4 rayMie) 
{ // rayMie.rgb=C*, rayMie.w=Cm,r
	return rayMie.rgb * rayMie.w / max(rayMie.r, 1e-4) * (betaR.r / betaR);
}

// transmittance(=transparency) of atmosphere for ray (r,mu) of length d
// (mu=cos(view zenith angle)), intersections with ground ignored
// uses analytic formula instead of transmittance texture
vec3 analyticTransmittance(float r, float mu, float d) 
{
    return exp(-betaR * opticalDepth(HR, r, mu, d) - betaMEx * opticalDepth(HM, r, mu, d));
}

//inscattered light along ray x+tv, when sun in direction s (=S[L]-T(x,x0)S[L]|x0)
vec3 inscatter(inout vec3 x, inout float t, vec3 v, vec3 sunDirection, out float r, out float mu, out vec3 attenuation) 
{
    vec3 result;
    r = length(x);
    mu = dot(x, v) / r;
    float d = -r * mu - sqrt(r * r * (mu * mu - 1.0) + Rt * Rt);
    if (d > 0.0) // if x in space and ray intersects atmosphere
	{ 
        // move x to nearest intersection of ray with top atmosphere boundary
        x += d * v;
        t -= d;
        mu = (r * mu + d) / Rt;
        r = Rt;
    }
    if (r <= Rt) // if ray intersects atmosphere
	{ 
        float nu = dot(v, sunDirection);
        float muS = dot(x, sunDirection) / r;
        float phaseR = phaseFunctionR(nu);
        float phaseM = phaseFunctionM(nu);
        vec4 inscatter = max(texture4D(inscatterSampler, r, mu, muS, nu), 0.0);
        if (t > 0.0) 
		{
            vec3 x0 = x + t * v;
            float r0 = length(x0);
            float rMu0 = dot(x0, v);
            float mu0 = rMu0 / r0;
            float muS0 = dot(x0, sunDirection) / r0;
#ifdef FIX
            // avoids imprecision problems in transmittance computations based on textures
            attenuation = analyticTransmittance(r, mu, t);
#else
            attenuation = transmittance(r, mu, v, x0);
#endif
            if (r0 > Rg + 0.01) 
			{
                // computes S[L]-T(x,x0)S[L]|x0
                inscatter = max(inscatter - attenuation.rgbr * texture4D(inscatterSampler, r0, mu0, muS0, nu), 0.0);
#ifdef FIX
                // avoids imprecision problems near horizon by interpolating between two points above and below horizon
                const float EPS = 0.004;
                float muHoriz = -sqrt(1.0 - (Rg / r) * (Rg / r));
                if (abs(mu - muHoriz) < EPS) {
                    float a = ((mu - muHoriz) + EPS) / (2.0 * EPS);

                    mu = muHoriz - EPS;
                    r0 = sqrt(r * r + t * t + 2.0 * r * t * mu);
                    mu0 = (r * mu + t) / r0;
                    vec4 inScatter0 = texture4D(inscatterSampler, r, mu, muS, nu);
                    vec4 inScatter1 = texture4D(inscatterSampler, r0, mu0, muS0, nu);
                    vec4 inScatterA = max(inScatter0 - attenuation.rgbr * inScatter1, 0.0);

                    mu = muHoriz + EPS;
                    r0 = sqrt(r * r + t * t + 2.0 * r * t * mu);
                    mu0 = (r * mu + t) / r0;
                    inScatter0 = texture4D(inscatterSampler, r, mu, muS, nu);
                    inScatter1 = texture4D(inscatterSampler, r0, mu0, muS0, nu);
                    vec4 inScatterB = max(inScatter0 - attenuation.rgbr * inScatter1, 0.0);

                    inscatter = mix(inScatterA, inScatterB, a);
                }
#endif
            }
        }
#ifdef FIX
        // avoids imprecision problems in Mie scattering when sun is below horizon
        inscatter.w *= smoothstep(0.00, 0.02, muS);
#endif
        result = max(inscatter.rgb * phaseR + getMie(inscatter) * phaseM, 0.0);
    } 
	else 
	{ // x in space and ray looking in space
        result = vec3(0.0);
    }
    return result * iSun;
}

//ground radiance at end of ray x+tv, when sun in direction sunDirection
//attenuated bewteen ground and viewer (=R[L0]+R[L*])
vec3 groundColor(vec3 x, float t, vec3 v, vec3 sunDirection, float r, float mu, vec3 attenuation)
{
    vec3 result;
    if (t > 0.0) // if ray hits ground surface
	{ 
        // ground reflectance at end of ray, x0
        vec3 x0 = x + t * v;
        float r0 = length(x0);
        vec3 n = x0 / r0;
        vec2 coords = vec2(atan(n.y, n.x), acos(n.z)) * vec2(0.5, 1.0) / M_PI + vec2(0.5, 0.0);
		//vec2 coords = vec2(n.x, n.y);
        vec4 reflectance = texture(reflectanceSampler, coords) * vec4(0.2, 0.2, 0.2, 1.0);
        if (r0 > Rg + 0.01) 
		{
            reflectance = vec4(0.4, 0.4, 0.4, 0.0);
        }

        // direct sun light (radiance) reaching x0
        float muS = dot(n, sunDirection);
        vec3 sunLight = transmittanceWithShadow(r0, muS);

        // precomputed sky light (irradiance) (=E[L*]) at x0
        vec3 groundSkyLight = irradiance(irradianceSampler, r0, muS);

        // light reflected at x0 (=(R[L0]+R[L*])/T(x,x0))
        vec3 groundColor = reflectance.rgb * (max(muS, 0.0) * sunLight + groundSkyLight) * iSun / M_PI;

        // water specular color due to sunLight
        if (reflectance.w > 0.0) 
		{
            vec3 h = normalize(sunDirection - v);
            float fresnel = 0.02 + 0.98 * pow(1.0 - dot(-v, h), 5.0);
            float waterBrdf = fresnel * pow(max(dot(h, n), 0.0), 150.0);
            groundColor += reflectance.w * max(waterBrdf, 0.0) * sunLight * iSun;
        }

        result = attenuation * groundColor; //=R[L0]+R[L*]
    } 
	else // ray looking at the sky
	{ 
        result = vec3(0.0);
    }
    return result;
}

// direct sun light for ray x+tv, when sun in direction sunDirection (=L0)
vec3 sunColor(vec3 x, float t, vec3 v, vec3 sunDirection, float r, float mu) 
{
    if (t > 0.0) 
	{
        return vec3(0.0);
    } 
	else 
	{
        vec3 transmittance = r <= Rt ? transmittanceWithShadow(r, mu) : vec3(1.0); // T(x,xo)
        float isun = step(cos(M_PI / 180.0), dot(v, sunDirection)) * iSun; // Lsun
        return transmittance * isun; // Eq (9)
    }
}

vec3 HDR(vec3 L) 
{
    L = L * exposure;
    L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r);
    L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g);
    L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b);
    return L;
}

void main() 
{
	vec3 mutableCameraPos = cameraPos;
    vec3 v = normalize(ray);

    float r = length(mutableCameraPos);
    float mu = dot(mutableCameraPos, v) / r;
    float t = -r * mu - sqrt(r * r * (mu * mu - 1.0) + Rg * Rg);

    vec3 g = mutableCameraPos - vec3(0.0, 0.0, Rg + 10.0);
    float a = v.x * v.x + v.y * v.y - v.z * v.z;
    float b = 2.0 * (g.x * v.x + g.y * v.y - g.z * v.z);
    float c = g.x * g.x + g.y * g.y - g.z * g.z;
    float d = -(b + sqrt(b * b - 4.0 * a * c)) / (2.0 * a);
    bool cone = d > 0.0 && abs(mutableCameraPos.z + d * v.z - Rg) <= 10.0;

    if (t > 0.0) 
	{
        if (cone && d < t) 
		{
            t = d;
        }
    } 
	else if (cone) 
	{
        t = d;
    }

    vec3 attenuation;
	vec3 inscatterColor = inscatter(mutableCameraPos, t, v, sunDirection, r, mu, attenuation); //S[L]-T(x,xs)S[l]|xs
    vec3 groundColor = groundColor(mutableCameraPos, t, v, sunDirection, r, mu, attenuation); //R[L0]+R[L*]
    vec3 sunColor = sunColor(mutableCameraPos, t, v, sunDirection, r, mu); //L0
    fragColour = vec4(HDR(sunColor + groundColor + inscatterColor), 1.0); // Eq (16)

    //fragColour = texture(inscatterSampler, vec3(coords, (sunDirection.x+1.0)*0.5));
    //fragColour = vec4(texture(irradianceSampler,coords).rgb*5.0, 1.0);
    //fragColour = texture(transmittanceSampler,coords);
	//fragColour = vec4(HDR(groundColor), 1.0);
}

#endif
