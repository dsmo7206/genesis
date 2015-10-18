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

// adds deltaS into S (line 11 in algorithm 4.1)

layout (shared) uniform LayerUniformBlock
{
	float r;
	vec4 dhdH;
	int layer;
};

const float M_PI = 3.141592657;

layout (shared) uniform PlanetUniformBlock
{
	float Rg;
	float Rt;
	float RL;
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

uniform sampler3D deltaSSampler;

out vec4 fragColour;

void getMuMuSNu(float r, vec4 dhdH, out float mu, out float muS, out float nu) 
{
    float x = gl_FragCoord.x - 0.5;
    float y = gl_FragCoord.y - 0.5;
    if (y < float(RES_MU) / 2.0) 
	{
        float d = 1.0 - y / (float(RES_MU) / 2.0 - 1.0);
        d = min(max(dhdH.z, d * dhdH.w), dhdH.w * 0.999);
        mu = (Rg * Rg - r * r - d * d) / (2.0 * r * d);
        mu = min(mu, -sqrt(1.0 - (Rg / r) * (Rg / r)) - 0.001);
    } else 
	{
        float d = (y - float(RES_MU) / 2.0) / (float(RES_MU) / 2.0 - 1.0);
        d = min(max(dhdH.x, d * dhdH.y), dhdH.y * 0.999);
        mu = (Rt * Rt - r * r - d * d) / (2.0 * r * d);
    }
    muS = mod(x, float(RES_MU_S)) / (float(RES_MU_S) - 1.0);
    // paper formula
    //muS = -(0.6 + log(1.0 - muS * (1.0 -  exp(-3.6)))) / 3.0;
    // better formula
    muS = tan((2.0 * muS - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1);
    nu = -1.0 + floor(x / float(RES_MU_S)) / (float(RES_NU) - 1.0) * 2.0;
}

float phaseFunctionR(float mu) 
{
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu);
}

void main() 
{
    float mu, muS, nu;
    getMuMuSNu(r, dhdH, mu, muS, nu);
    vec3 uvw = vec3(gl_FragCoord.xy, float(layer) + 0.5) / vec3(ivec3(RES_MU_S * RES_NU, RES_MU, RES_R));
    fragColour = vec4(texture(deltaSSampler, uvw).rgb / phaseFunctionR(nu), 0.0);
}
