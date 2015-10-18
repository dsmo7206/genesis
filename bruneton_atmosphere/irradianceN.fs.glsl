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

// computes ground irradiance due to skylight E[deltaS] (line 8 in algorithm 4.1)

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

const float M_PI = 3.141592657;

const float dphi = M_PI / float(IRRADIANCE_INTEGRAL_SAMPLES);
const float dtheta = M_PI / float(IRRADIANCE_INTEGRAL_SAMPLES);

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

uniform sampler3D deltaSRSampler;
uniform sampler3D deltaSMSampler;
uniform float first;

void getIrradianceRMuS(out float r, out float muS) 
{
    r = Rg + (gl_FragCoord.y - 0.5) / (float(SKY_H) - 1.0) * (Rt - Rg);
    muS = -0.2 + (gl_FragCoord.x - 0.5) / (float(SKY_W) - 1.0) * (1.0 + 0.2);
}

// Rayleigh phase function
float phaseFunctionR(float mu) 
{
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu);
}

// Mie phase function
float phaseFunctionM(float mu) 
{
	return 1.5 * 1.0 / (4.0 * M_PI) * (1.0 - mieG*mieG) * pow(1.0 + (mieG*mieG) - 2.0*mieG*mu, -3.0/2.0) * (1.0 + mu * mu) / (2.0 + mieG*mieG);
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

void main() 
{
    float r, muS;
    getIrradianceRMuS(r, muS);
    vec3 s = vec3(max(sqrt(1.0 - muS * muS), 0.0), 0.0, muS);

    vec3 result = vec3(0.0);
    // integral over 2.PI around x with two nested loops over w directions (theta,phi) -- Eq (15)
    for (int iphi = 0; iphi < 2 * IRRADIANCE_INTEGRAL_SAMPLES; ++iphi) 
	{
        float phi = (float(iphi) + 0.5) * dphi;
        for (int itheta = 0; itheta < IRRADIANCE_INTEGRAL_SAMPLES / 2; ++itheta) 
		{
            float theta = (float(itheta) + 0.5) * dtheta;
            float dw = dtheta * dphi * sin(theta);
            vec3 w = vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
            float nu = dot(s, w);
            if (first == 1.0) 
			{
                // first iteration is special because Rayleigh and Mie were stored separately,
                // without the phase functions factors; they must be reintroduced here
                float pr1 = phaseFunctionR(nu);
                float pm1 = phaseFunctionM(nu);
                vec3 ray1 = texture4D(deltaSRSampler, r, w.z, muS, nu).rgb;
                vec3 mie1 = texture4D(deltaSMSampler, r, w.z, muS, nu).rgb;
                result += (ray1 * pr1 + mie1 * pm1) * w.z * dw;
            } 
			else 
			{
                result += texture4D(deltaSRSampler, r, w.z, muS, nu).rgb * w.z * dw;
            }
        }
    }

    gl_FragColor = vec4(result, 0.0);
}
