// Atmospheric scattering fragment shader
// Author: Sean O'Neil
// Copyright (c) 2004 Sean O'Neil

layout (std140) uniform PlanetUniforms
{
	mat4 m4_terrainMV;
	mat4 m4_terrainMVP;
	mat4 m4_skyMV;
	mat4 m4_skyMVP;
	
	vec3 v3_cameraFromCenter_VS; // The camera's current position
	vec3 v3_invWavelength;		// 1 / pow(wavelength, 4) for the red, green, and blue channels
	vec3 v3_lightCol;
	vec3 v3_lightDir_VS;		// The direction vector to the light source
	vec3 v3_lightPos_VS;
	vec3 v3_planetPos_VS;
	
	float f_cameraHeight;		// The camera's current height
	float f_cameraHeight2;		// f_cameraHeight^2
	float f_depthCoef;
	float g;
	float g2;
	float f_innerRadius;		// The inner (planetary) radius
	float f_innerRadius2;		// f_innerRadius^2
	float f_krESun;				// Kr * ESun
	float f_kmESun;				// Km * ESun
	float f_kr4PI;				// Kr * 4 * PI
	float f_km4PI;				// Km * 4 * PI
	float f_outerRadius;		// The outer (atmosphere) radius
	float f_outerRadius2;		// f_outerRadius^2
	float f_scale;				// 1 / (f_outerRadius - f_innerRadius)
	float f_scaleDepth;			// The scale depth (i.e. the altitude at which the atmosphere's average density is found)
	float f_scaleOverScaleDepth;// f_scale / f_scaleDepth
	
	int i_samples;
};

in vec3 v3_cameraFromVertPos_VS;
in vec3 v3_rayleighCol;
in vec3 v3_mieCol;
in float flogz; // Depth hack

void main()
{
	float fCos = dot(v3_lightDir_VS, v3_cameraFromVertPos_VS) / length(v3_cameraFromVertPos_VS);
	float fRayleighPhase = 1.0 + fCos * fCos;
	float fMiePhase = (1.0 - g2) / (2.0 + g2) * fRayleighPhase / pow(1.0 + g2 - 2.0 * g * fCos, 1.5);
	
	gl_FragColor = vec4(1.0 - exp(-1.5 * (fRayleighPhase * v3_rayleighCol + fMiePhase * v3_mieCol)), 1.0);
	gl_FragDepth = log2(flogz) * 0.5 * f_depthCoef;
}
