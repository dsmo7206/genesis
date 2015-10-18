
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

in vec3 v3_vertNorm_VS;
in vec3 v3_eyeDir_VS;
in vec3 v3_lightDir_VS_in;
in vec3 v3_vertCol;
in float flogz; // Depth hack

#if ATMOSPHERE
in vec3 v3_atmCol;
in vec3 v3_atmAtt;
#endif

layout (location = 0) out vec3 ColourOut; 
layout (location = 1) out vec3 DepthOut; 
layout (location = 2) out vec3 NormOut;

vec3 getTerrainColour()
{
	const vec3 L = normalize(v3_lightDir_VS_in);
	const float cosTheta = clamp(dot(v3_vertNorm_VS, L), 0.0, 1.0);
	const vec3 E = normalize(v3_eyeDir_VS); // Eye vector (towards the camera)
	const vec3 R = reflect(-L, v3_vertNorm_VS);
	const float cosAlpha = clamp(dot(E, R), 0.0, 1.0);
	
	return
		  v3_vertCol * 0.01 // Ambient
		+ v3_vertCol * v3_lightCol * cosTheta // Diffuse
		//+ v3_vertCol * v3_lightCol * pow(cosAlpha, 30) // Specular
	;
}

void main()
{
	vec3 terrainColour = getTerrainColour();

#if ATMOSPHERE
	//gl_FragColor = vec4(v3_atmCol + terrainColour*v3_atmAtt, 1.0);
	ColourOut = v3_atmCol + terrainColour*v3_atmAtt;
#else
	//gl_FragColor = vec4(terrainColour, 1.0);
	ColourOut = terrainColour;
#endif

	//gl_FragDepth = log2(flogz) * 0.5 * f_depthCoef; // DEPTH HACK
	gl_FragDepth = log2(flogz) * 0.5 * f_depthCoef; // DEPTH HACK
	DepthOut = vec3(flogz);

	NormOut = vec3(m4_terrainMV * vec4(v3_lightDir_VS_in, 0.0));
}
