
void TW_CALL antSetWaveLength(const void* value, void* clientData) 
{ 
	Planet* const planet = (Planet*)clientData;
	const glm::vec3 waveLength = *(glm::vec3*)value;
	planet->m_atmosphereConstants->m_waveLength = waveLength;

	planet->m_uniforms.v3_invWavelength = glm::vec3(powf(waveLength.x, -4.0), powf(waveLength.y, -4.0), powf(waveLength.z, -4.0));
}

void TW_CALL antSetInnerRadius(const void* value, void* clientData) 
{ 
	Planet* const planet = (Planet*)clientData;
	const float innerRadius = *(float*)value;
	planet->m_atmosphereConstants->m_innerRadius = innerRadius;

	planet->m_uniforms.f_innerRadius = innerRadius;
	planet->m_uniforms.f_innerRadius2 = innerRadius * innerRadius;
	planet->m_uniforms.f_scale = 1.0f / (planet->m_atmosphereConstants->m_outerRadius - innerRadius);
	planet->m_uniforms.f_scaleOverScaleDepth = planet->m_uniforms.f_scale / planet->m_atmosphereConstants->m_scaleDepth;
}

void TW_CALL antSetOuterRadius(const void* value, void* clientData) 
{ 
	Planet* const planet = (Planet*)clientData;
	const float outerRadius = *(float*)value;
	planet->m_atmosphereConstants->m_outerRadius = outerRadius;
	
	planet->m_uniforms.f_outerRadius = outerRadius;
	planet->m_uniforms.f_outerRadius2 = outerRadius * outerRadius;
	planet->m_uniforms.f_scale = 1.0f / (outerRadius - planet->m_atmosphereConstants->m_innerRadius);
	planet->m_uniforms.f_scaleOverScaleDepth = planet->m_uniforms.f_scale / planet->m_atmosphereConstants->m_scaleDepth;
}

void TW_CALL antSetKr(const void* value, void* clientData) 
{ 
	Planet* const planet = (Planet*)clientData;
	const float kr = *(float*)value;
	planet->m_atmosphereConstants->m_kr = kr;

	planet->m_uniforms.f_krESun = kr * planet->m_atmosphereConstants->m_eSun;
	planet->m_uniforms.f_kr4PI = kr * 4.0f * PI;
}

void TW_CALL antSetKm(const void* value, void* clientData) 
{ 
	Planet* const planet = (Planet*)clientData;
	const float km = *(float*)value;
	planet->m_atmosphereConstants->m_km = km;

	planet->m_uniforms.f_kmESun = km * planet->m_atmosphereConstants->m_eSun;
	planet->m_uniforms.f_km4PI = km * 4.0f * PI;
}

void TW_CALL antSetESun(const void* value, void* clientData) 
{ 
	Planet* const planet = (Planet*)clientData;
	const float eSun = *(float*)value;
	planet->m_atmosphereConstants->m_eSun = eSun;

	planet->m_uniforms.f_krESun = planet->m_atmosphereConstants->m_kr * eSun;
	planet->m_uniforms.f_kmESun = planet->m_atmosphereConstants->m_km * eSun;
}

void TW_CALL antSetG(const void* value, void* clientData) 
{ 
	Planet* const planet = (Planet*)clientData;
	const float f_g = *(float*)value;
	planet->m_atmosphereConstants->m_g = f_g;

	planet->m_uniforms.f_g = f_g;
	planet->m_uniforms.f_g2 = f_g * f_g;
}

void TW_CALL antSetScaleDepth(const void* value, void* clientData) 
{ 
	Planet* const planet = (Planet*)clientData;
	const float scaleDepth = *(float*)value;
	planet->m_atmosphereConstants->m_scaleDepth = scaleDepth;

	const float scale = 1.0f / (
		planet->m_atmosphereConstants->m_outerRadius - 
		planet->m_atmosphereConstants->m_innerRadius
	);

	planet->m_uniforms.f_scaleDepth = scaleDepth;
	planet->m_uniforms.f_scaleOverScaleDepth = scale / scaleDepth;
}

void TW_CALL antSetSamples(const void* value, void* clientData) 
{ 
	Planet* const planet = (Planet*)clientData;
	const int samples = *(int*)value;
	planet->m_atmosphereConstants->m_samples = samples;

	planet->m_uniforms.i_samples = samples;
}

void TW_CALL antGetWaveLength(void* value, void* clientData) 
{ 
	*(glm::vec3*)value = ((Planet*)clientData)->m_atmosphereConstants->m_waveLength; 
}
void TW_CALL antGetInnerRadius(void* value, void* clientData) 
{ 
	*(float*)value = ((Planet*)clientData)->m_atmosphereConstants->m_innerRadius; 
}
void TW_CALL antGetOuterRadius(void* value, void* clientData) 
{ 
	*(float*)value = ((Planet*)clientData)->m_atmosphereConstants->m_outerRadius; 
}
void TW_CALL antGetKr(void* value, void* clientData) 
{ 
	*(float*)value = ((Planet*)clientData)->m_atmosphereConstants->m_kr; 
}
void TW_CALL antGetKm(void* value, void* clientData) 
{ 
	*(float*)value = ((Planet*)clientData)->m_atmosphereConstants->m_km; 
}
void TW_CALL antGetESun(void* value, void* clientData) 
{ 
	*(float*)value = ((Planet*)clientData)->m_atmosphereConstants->m_eSun; 
}
void TW_CALL antGetG(void* value, void* clientData) 
{ 
	*(float*)value = ((Planet*)clientData)->m_atmosphereConstants->m_g; 
}
void TW_CALL antGetScaleDepth(void* value, void* clientData) 
{ 
	*(float*)value = ((Planet*)clientData)->m_atmosphereConstants->m_scaleDepth; 
}
void TW_CALL antGetSamples(void* value, void* clientData) 
{ 
	*(int*)value = ((Planet*)clientData)->m_atmosphereConstants->m_samples; 
}
