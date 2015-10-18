// Atmospheric scattering vertex shader
// Author: Sean O'Neil
// Copyright (c) 2004 Sean O'Neil

layout(location = 0) in vec4 v4_vertPos_MS;

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

out float flogz; // Depth hack
out vec3 v3_cameraFromVertPos_VS;
out vec3 v3_rayleighCol;
out vec3 v3_mieCol;

float scale(float fCos)
{
	const float x = 1.0 - fCos;
	return f_scaleDepth * exp(-0.00287 + x * (0.459 + x * (3.83 + x * (-6.80 + x * 5.25))));
}

void setAtmosphereOutputs(void)
{
	// Get the ray from the camera to the vertex and its length (which is the far point of the ray passing through the atmosphere)
	const vec3 v3_vertPosFromCenter_VS = (m4_skyMV * v4_vertPos_MS).xyz - v3_planetPos_VS;
	
	const vec3 v3_vertPosFromCamera_VS = v3_vertPosFromCenter_VS - v3_cameraFromCenter_VS;
	float f_vertDistFromCamera_VS = length(v3_vertPosFromCamera_VS);
	const vec3 v3_vertDirFromCamera_VS = v3_vertPosFromCamera_VS / f_vertDistFromCamera_VS;

#if OUTSIDE_ATMOSPHERE
	// Calculate the closest intersection of the ray with the outer atmosphere (which is the near point of the ray passing through the atmosphere)
	float B = 2.0 * dot(v3_cameraFromCenter_VS, v3_vertDirFromCamera_VS);
	float C = f_cameraHeight2 - f_outerRadius2;
	float fDet = max(0.0, B * B - 4.0 * C);
	float fNear = 0.5 * (-B - sqrt(fDet));

	// Calculate the ray's starting position, then calculate its scattering offset
	vec3 v3Start = v3_cameraFromCenter_VS + v3_vertDirFromCamera_VS * fNear;
	f_vertDistFromCamera_VS -= fNear;
	float fStartAngle = dot(v3_vertDirFromCamera_VS, v3Start) / f_outerRadius;
	float fStartDepth = exp(-1.0 / f_scaleDepth);
	float fStartOffset = fStartDepth * scale(fStartAngle);
#else
	vec3 v3Start = v3_cameraFromCenter_VS;
	float fHeight = length(v3Start); 
	float fDepth = exp(f_scaleOverScaleDepth * (f_innerRadius - f_cameraHeight));
	float fStartAngle = dot(v3_vertDirFromCamera_VS, v3Start) / fHeight;
	float fStartOffset = fDepth * scale(fStartAngle);
#endif

	// Initialize the scattering loop variables
	float fSampleLength = f_vertDistFromCamera_VS / i_samples;
	float f_scaledLength = fSampleLength * f_scale;
	vec3 v3SampleRay = v3_vertDirFromCamera_VS * fSampleLength;
	vec3 v3SamplePoint = v3Start + v3SampleRay * 0.5;

	// Now loop through the sample rays
	vec3 v3FrontColor = vec3(0.0);
	for (int i = 0; i < i_samples; i++)
	{
		float fHeight = length(v3SamplePoint); // HACK
		float fDepth = exp(f_scaleOverScaleDepth * (f_innerRadius - fHeight));
		float fLightAngle = dot(v3_lightDir_VS, v3SamplePoint) / fHeight;
		float fCameraAngle = dot(v3_vertDirFromCamera_VS, v3SamplePoint) / fHeight;
		float fScatter = (fStartOffset + fDepth * (scale(fLightAngle) - scale(fCameraAngle)));
		vec3 v3Attenuate = exp(-fScatter * (v3_invWavelength * f_kr4PI + f_km4PI));
		v3FrontColor += v3Attenuate * (fDepth * f_scaledLength);
		v3SamplePoint += v3SampleRay;
	}

	// Finally, scale the Mie and Rayleigh colors and set up the varying variables for the pixel shader
	v3_cameraFromVertPos_VS = v3_cameraFromCenter_VS - v3_vertPosFromCenter_VS;
	v3_rayleighCol = v3FrontColor * (v3_invWavelength * f_krESun);
	v3_mieCol = v3FrontColor * f_kmESun;
}

void setSkyOutputs()
{
	// Output position of the vertex, in clip space : MVP * position
	vec4 position = m4_skyMVP * v4_vertPos_MS;

	// Depth hack
	flogz = 1.0 + position.w;
	position.z = log2(max(1e-6, flogz)) * f_depthCoef - 1.0;

	gl_Position = position;
}

void main()
{
	setAtmosphereOutputs();
	setSkyOutputs();
}
