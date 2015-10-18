
#extension GL_EXT_texture_array : enable

layout(location = 0) in vec3 v3_vertPos_MS;

uniform mat4 MVP;
uniform vec2 gridSize;
uniform float choppy;
uniform sampler2DArray fftWavesSampler;
uniform vec4 GRID_SIZES;
uniform float f_depthCoef;
uniform float stretch;
uniform float scale;

out vec2 u; // coordinates in world space used to compute P(u)
out vec3 P; // wave point P(u) in world space
out float flogz;

void main() 
{
	vec3 normPosModelSpace3 = normalize(v3_vertPos_MS);    

    u = vec2(stretch*normPosModelSpace3.x, stretch*normPosModelSpace3.y); // HACK

    vec2 ux = vec2(gridSize.x, 0.0);
    vec2 uy = vec2(0.0, gridSize.y);
    vec2 dux = abs(ux - u) * 2.0;
    vec2 duy = abs(uy - u) * 2.0;

    vec3 dP = vec3(0.0);
    dP.z += texture2DArrayGrad(fftWavesSampler, vec3(u / GRID_SIZES.x, 0.0), dux / GRID_SIZES.x, duy / GRID_SIZES.x).x;
    dP.z += texture2DArrayGrad(fftWavesSampler, vec3(u / GRID_SIZES.y, 0.0), dux / GRID_SIZES.y, duy / GRID_SIZES.y).y;
    dP.z += texture2DArrayGrad(fftWavesSampler, vec3(u / GRID_SIZES.z, 0.0), dux / GRID_SIZES.z, duy / GRID_SIZES.z).z;
    dP.z += texture2DArrayGrad(fftWavesSampler, vec3(u / GRID_SIZES.w, 0.0), dux / GRID_SIZES.w, duy / GRID_SIZES.w).w;
	/*
    if (choppy > 0.0) {
        dP.xy += texture2DArrayGrad(fftWavesSampler, vec3(u / GRID_SIZES.x, 3.0), dux / GRID_SIZES.x, duy / GRID_SIZES.x).xy;
        dP.xy += texture2DArrayGrad(fftWavesSampler, vec3(u / GRID_SIZES.y, 3.0), dux / GRID_SIZES.y, duy / GRID_SIZES.y).zw;
        dP.xy += texture2DArrayGrad(fftWavesSampler, vec3(u / GRID_SIZES.z, 4.0), dux / GRID_SIZES.z, duy / GRID_SIZES.z).xy;
        dP.xy += texture2DArrayGrad(fftWavesSampler, vec3(u / GRID_SIZES.w, 4.0), dux / GRID_SIZES.w, duy / GRID_SIZES.w).zw;
    }
	*/

    //P = vec3(u + dP.xy, dP.z);

	normPosModelSpace3 *= (1.0 + scale*dP.z);

	vec4 screenPosition = MVP * vec4(normPosModelSpace3.xyz, 1.0);
	flogz = 1.0 + screenPosition.w;
	screenPosition.z = log2(max(1e-6, flogz)) * f_depthCoef - 1.0;
	gl_Position = screenPosition;

	P = normPosModelSpace3; // PROBABLY NOT CORRECT
}
