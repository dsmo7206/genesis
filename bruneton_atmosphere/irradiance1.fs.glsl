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

// computes ground irradiance due to direct sunlight E[L0] (line 2 in algorithm 4.1)

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

uniform sampler2D transmittanceSampler;

void getIrradianceRMuS(out float r, out float muS) 
{
    r = Rg + (gl_FragCoord.y - 0.5) / (float(SKY_H) - 1.0) * (Rt - Rg);
    muS = -0.2 + (gl_FragCoord.x - 0.5) / (float(SKY_W) - 1.0) * (1.0 + 0.2);
}

vec2 getTransmittanceUV(float r, float mu) 
{
    return vec2(
		atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5, 
		sqrt((r - Rg) / (Rt - Rg))
	);
}

vec3 transmittance(float r, float mu) 
{
    return texture(transmittanceSampler, getTransmittanceUV(r, mu)).rgb;
}

void main() 
{
    float r, muS;
    getIrradianceRMuS(r, muS);
    gl_FragColor = vec4(transmittance(r, muS) * max(muS, 0.0), 0.0);
}
