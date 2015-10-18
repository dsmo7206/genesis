#include "bruneton_water.h"
#include "utils.h"
#include "globals.h"
#include "world_clock.h"

#if 0

#define IRRADIANCE_UNIT 0
#define INSCATTER_UNIT 1
#define TRANSMITTANCE_UNIT 2
#define SKY_UNIT 3
#define NOISE_UNIT 4
#define SPECTRUM_1_2_UNIT 5
#define SPECTRUM_3_4_UNIT 6
#define SLOPE_VARIANCE_UNIT 7
#define FFT_A_UNIT 8
#define FFT_B_UNIT 9
#define BUTTERFLY_UNIT 10

unsigned int skyTexSize = 512;
float *spectrum12 = NULL;
float *spectrum34 = NULL;
const int N_SLOPE_VARIANCE = 10; // size of the 3d texture containing precomputed filtered slope variances
float WIND = 5.0; // wind speed in meters per second (at 10m above surface)
float OMEGA = 0.84f; // sea state (inverse wave age)
const int PASSES = 8; // number of passes needed for the FFT 6 -> 64, 7 -> 128, 8 -> 256, etc
const int FFT_SIZE = 1 << PASSES; // size of the textures storing the waves in frequency and spatial domains
float GRID1_SIZE = 5488.0; // size in meters (i.e. in spatial domain) of the first grid
float GRID2_SIZE = 392.0; // size in meters (i.e. in spatial domain) of the second grid
float GRID3_SIZE = 28.0; // size in meters (i.e. in spatial domain) of the third grid
float GRID4_SIZE = 2.0; // size in meters (i.e. in spatial domain) of the fourth grid
bool choppy = true;
float A = 1.0; // wave amplitude factor (should be one)
const float cm = 0.23f; // Eq 59
const float km = 370.0f; // Eq 59
float hdrExposure = 0.4f;
bool seaContrib = true;
bool sunContrib = true;
bool skyContrib = true;

float sqr(float x)
{
    return x * x;
}

float omega(float k)
{
    return sqrt(9.81f * k * (1.0f + sqr(k / km))); // Eq 24
}

long lrandom(long *seed)
{
    *seed = (*seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return *seed;
}

float frandom(long *seed)
{
    long r = lrandom(seed) >> (31 - 24);
    return r / (float)(1 << 24);
}

inline float grandom(float mean, float stdDeviation, long *seed)
{
    float x1, x2, w, y1;
    static float y2;
    static int use_last = 0;

    if (use_last) {
        y1 = y2;
        use_last = 0;
    } else {
        do {
            x1 = 2.0f * frandom(seed) - 1.0f;
            x2 = 2.0f * frandom(seed) - 1.0f;
            w  = x1 * x1 + x2 * x2;
        } while (w >= 1.0f);
        w  = sqrt((-2.0f * log(w)) / w);
        y1 = x1 * w;
        y2 = x2 * w;
        use_last = 1;
    }
	return mean + y1 * stdDeviation;
}

// 1/kx and 1/ky in meters
float spectrum(float kx, float ky, bool omnispectrum = false)
{
    float U10 = WIND;
    float Omega = OMEGA;

    // phase speed
    float k = sqrt(kx * kx + ky * ky);
    float c = omega(k) / k;

    // spectral peak
    float kp = 9.81f * sqr(Omega / U10); // after Eq 3
    float cp = omega(kp) / kp;

    // friction velocity
    float z0 = (float)3.7e-5 * sqr(U10) / 9.81f * powf(U10 / cp, 0.9f); // Eq 66
    float u_star = 0.41f * U10 / logf(10.0f / z0); // Eq 60

    float Lpm = expf(-5.0f / 4.0f * sqr(kp / k)); // after Eq 3
    float gamma = Omega < 1.0f ? 1.7f : 1.7f + 6.0f * logf(Omega); // after Eq 3 // log10 or log??
    float sigma = 0.08f * (1.0f + 4.0f / powf(Omega, 3.0f)); // after Eq 3
    float Gamma = expf(-1.0f / (2.0f * sqr(sigma)) * sqr(sqrt(k / kp) - 1.0f));
    float Jp = pow(gamma, Gamma); // Eq 3
    float Fp = Lpm * Jp * exp(- Omega / sqrt(10.0f) * (sqrt(k / kp) - 1.0f)); // Eq 32
    float alphap = 0.006f * sqrt(Omega); // Eq 34
    float Bl = 0.5f * alphap * cp / c * Fp; // Eq 31

    float alpham = 0.01f * (u_star < cm ? 1.0f + logf(u_star / cm) : 1.0f + 3.0f * logf(u_star / cm)); // Eq 44
    float Fm = expf(-0.25f * sqr(k / km - 1.0f)); // Eq 41
    float Bh = 0.5f * alpham * cm / c * Fm * Lpm; // Eq 40 (fixed)

    if (omnispectrum) {
        return A * (Bl + Bh) / (k * sqr(k)); // Eq 30
    }

    float a0 = logf(2.0f) / 4.0f; float ap = 4.0f; float am = 0.13f * u_star / cm; // Eq 59
    float Delta = tanh(a0 + ap * powf(c / cp, 2.5f) + am * powf(cm / c, 2.5f)); // Eq 57

    float phi = atan2(ky, kx);

    if (kx < 0.0f) {
        return 0.0f;
    } else {
        Bl *= 2.0f;
        Bh *= 2.0f;
    }

    return A * (Bl + Bh) * (1.0f + Delta * cosf(2.0f * phi)) / (2.0f * PI * sqr(sqr(k))); // Eq 67
}

void getSpectrumSample(int i, int j, float lengthScale, float kMin, float *result)
{
    static long seed = 1234;
    float dk = 2.0f * PI / lengthScale;
    float kx = i * dk;
    float ky = j * dk;
    if (abs(kx) < kMin && abs(ky) < kMin) {
        result[0] = 0.0f;
        result[1] = 0.0f;
    } else {
        float S = spectrum(kx, ky);
        float h = sqrt(S / 2.0f) * dk;
        float phi = frandom(&seed) * 2.0f * PI;
        result[0] = h * cos(phi);
        result[1] = h * sin(phi);
    }
}

// generates the waves spectrum
void generateWavesSpectrum()
{
    if (spectrum12 != NULL) {
        delete[] spectrum12;
        delete[] spectrum34;
    }
    spectrum12 = new float[FFT_SIZE * FFT_SIZE * 4];
    spectrum34 = new float[FFT_SIZE * FFT_SIZE * 4];

    for (int y = 0; y < FFT_SIZE; ++y) 
	{
        for (int x = 0; x < FFT_SIZE; ++x) 
		{
            int offset = 4 * (x + y * FFT_SIZE);
            int i = x >= FFT_SIZE / 2 ? x - FFT_SIZE : x;
            int j = y >= FFT_SIZE / 2 ? y - FFT_SIZE : y;
            getSpectrumSample(i, j, GRID1_SIZE, PI / GRID1_SIZE, spectrum12 + offset);
            getSpectrumSample(i, j, GRID2_SIZE, PI * FFT_SIZE / GRID1_SIZE, spectrum12 + offset + 2);
            getSpectrumSample(i, j, GRID3_SIZE, PI * FFT_SIZE / GRID2_SIZE, spectrum34 + offset);
            getSpectrumSample(i, j, GRID4_SIZE, PI * FFT_SIZE / GRID3_SIZE, spectrum34 + offset + 2);
        }
    }

    glActiveTexture(GL_TEXTURE0 + SPECTRUM_1_2_UNIT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, FFT_SIZE, FFT_SIZE, 0, GL_RGBA, GL_FLOAT, spectrum12);
    glActiveTexture(GL_TEXTURE0 + SPECTRUM_3_4_UNIT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, FFT_SIZE, FFT_SIZE, 0, GL_RGBA, GL_FLOAT, spectrum34);
}

float getSlopeVariance(float kx, float ky, float *spectrumSample)
{
    const float kSquare = kx * kx + ky * ky;
    const float real = spectrumSample[0];
    const float img = spectrumSample[1];
    const float hSquare = real * real + img * img;
    return kSquare * hSquare * 2.0f;
}

// precomputes filtered slope variances in a 3d texture, based on the wave spectrum
void BrunetonWater::computeSlopeVarianceTex()
{
    // slope variance due to all waves, by integrating over the full spectrum
    float theoreticSlopeVariance = 0.0;
    float k = (float)5e-3;
    while (k < 1e3) {
        float nextK = k * 1.001f;
        theoreticSlopeVariance += k * k * spectrum(k, 0, true) * (nextK - k);
        k = nextK;
    }

    // slope variance due to waves, by integrating over the spectrum part
    // that is covered by the four nested grids. This can give a smaller result
    // than the theoretic total slope variance, because the higher frequencies
    // may not be covered by the four nested grid. Hence the difference between
    // the two is added as a "delta" slope variance in the "variances" shader,
    // to be sure not to lose the variance due to missing wave frequencies in
    // the four nested grids
    float totalSlopeVariance = 0.0;
    for (int y = 0; y < FFT_SIZE; ++y) {
        for (int x = 0; x < FFT_SIZE; ++x) {
            int offset = 4 * (x + y * FFT_SIZE);
            float i = 2.0f * PI * (x >= FFT_SIZE / 2 ? x - FFT_SIZE : x);
            float j = 2.0f * PI * (y >= FFT_SIZE / 2 ? y - FFT_SIZE : y);
            totalSlopeVariance += getSlopeVariance(i / GRID1_SIZE, j / GRID1_SIZE, spectrum12 + offset);
            totalSlopeVariance += getSlopeVariance(i / GRID2_SIZE, j / GRID2_SIZE, spectrum12 + offset + 2);
            totalSlopeVariance += getSlopeVariance(i / GRID3_SIZE, j / GRID3_SIZE, spectrum34 + offset);
            totalSlopeVariance += getSlopeVariance(i / GRID4_SIZE, j / GRID4_SIZE, spectrum34 + offset + 2);
        }
    }

	glBindVertexArray(m_variancesProgram.m_vertexArray.m_id);
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_variancesFbo.m_id);
    glViewport(0, 0, N_SLOPE_VARIANCE, N_SLOPE_VARIANCE);

	glUseProgram(m_variancesProgram.m_program->m_id);
	glUniform4f(m_variancesProgram.m_locId_gridSizes, GRID1_SIZE, GRID2_SIZE, GRID3_SIZE, GRID4_SIZE);
    glUniform1f(m_variancesProgram.m_locId_slopeVarianceDelta, 0.5f * (theoreticSlopeVariance - totalSlopeVariance));

    for (int layer = 0; layer < N_SLOPE_VARIANCE; ++layer) 
	{
		glFramebufferTexture3D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_3D, m_slopeVarianceTex.m_id, 0, layer);
		glUniform1f(m_variancesProgram.m_locId_c, (float)layer);
        m_variancesProgram.drawQuad();
    }
	
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ----------------------------------------------------------------------------
// WAVES GENERATION AND ANIMATION (using FFT on GPU)
// ----------------------------------------------------------------------------

int bitReverse(int i, int N)
{
	int j = i;
	int M = N;
	int Sum = 0;
	int W = 1;
	M = M / 2;
	while (M != 0) {
		j = (i & M) > M - 1;
		Sum += j * W;
		W *= 2;
		M = M / 2;
	}
	return Sum;
}

void computeWeight(int N, int k, float &wr, float &wi)
{
	wr = cosf(2.0f * PI * k / float(N));
	wi = sinf(2.0f * PI * k / float(N));
}

float *computeButterflyLookupTexture()
{
    float *data = new float[FFT_SIZE * PASSES * 4];

	for (int i = 0; i < PASSES; i++) {
		int nBlocks  = (int) powf(2.0, float(PASSES - 1 - i));
		int nHInputs = (int) powf(2.0, float(i));
		for (int j = 0; j < nBlocks; j++) {
			for (int k = 0; k < nHInputs; k++) {
			    int i1, i2, j1, j2;
				if (i == 0) {
					i1 = j * nHInputs * 2 + k;
					i2 = j * nHInputs * 2 + nHInputs + k;
					j1 = bitReverse(i1, FFT_SIZE);
					j2 = bitReverse(i2, FFT_SIZE);
				} else {
					i1 = j * nHInputs * 2 + k;
					i2 = j * nHInputs * 2 + nHInputs + k;
					j1 = i1;
					j2 = i2;
				}

				float wr, wi;
				computeWeight(FFT_SIZE, k * nBlocks, wr, wi);

                int offset1 = 4 * (i1 + i * FFT_SIZE);
                data[offset1 + 0] = (j1 + 0.5f) / FFT_SIZE;
                data[offset1 + 1] = (j2 + 0.5f) / FFT_SIZE;
                data[offset1 + 2] = wr;
                data[offset1 + 3] = wi;

                int offset2 = 4 * (i2 + i * FFT_SIZE);
                data[offset2 + 0] = (j1 + 0.5f) / FFT_SIZE;
                data[offset2 + 1] = (j2 + 0.5f) / FFT_SIZE;
                data[offset2 + 2] = -wr;
                data[offset2 + 3] = -wi;
			}
		}
	}

	return data;
}

InitProgram::InitProgram() :
	m_program(new ShaderProgram({ ShaderStages::Vertex::bruneton_init, ShaderStages::Fragment::bruneton_init }))
{
	glBindVertexArray(m_vertexArray.m_id);
	glBindBuffer(GL_ARRAY_BUFFER, m_fullscreenQuadVertexBuffer.m_id);

	glm::vec4 quadVertexes[4] = {
		glm::vec4(-1.0, -1.0, 0.0, 0.0),
		glm::vec4(+1.0, -1.0, 1.0, 0.0),
		glm::vec4(-1.0, +1.0, 0.0, 1.0),
		glm::vec4(+1.0, +1.0, 1.0, 1.0)
	};

	glBufferData(GL_ARRAY_BUFFER, 4*sizeof(glm::vec4), (void*)quadVertexes, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 0);

	glUseProgram(m_program->m_id);

	m_locId_spectrum_1_2_Sampler = m_program->getUniformLocationId("spectrum_1_2_Sampler");
	m_locId_spectrum_3_4_Sampler = m_program->getUniformLocationId("spectrum_3_4_Sampler");
	m_locId_fftSize = m_program->getUniformLocationId("FFT_SIZE");
	m_locId_inverseGridSizes = m_program->getUniformLocationId("INVERSE_GRID_SIZES");
	m_locId_t = m_program->getUniformLocationId("t");

	glUniform1i(m_locId_spectrum_1_2_Sampler, SPECTRUM_1_2_UNIT);
	glUniform1i(m_locId_spectrum_3_4_Sampler, SPECTRUM_3_4_UNIT);

	glBindVertexArray(0);
}

void InitProgram::drawQuad()
{
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

/////////////////////////////////////////////////

FFTProgram::FFTProgram(bool isX) :
	m_program(
		new ShaderProgram({
			ShaderStages::Vertex::bruneton_fft,
			ShaderStages::Geometry::bruneton_fft,
			isX ? ShaderStages::Fragment::bruneton_fftx : ShaderStages::Fragment::bruneton_ffty
		})
	)
{
	glBindVertexArray(m_vertexArray.m_id);
	glBindBuffer(GL_ARRAY_BUFFER, m_fullscreenQuadVertexBuffer.m_id);

	glm::vec4 quadVertexes[4] = {
		glm::vec4(-1.0, -1.0, 0.0, 0.0),
		glm::vec4(+1.0, -1.0, 1.0, 0.0),
		glm::vec4(-1.0, +1.0, 0.0, 1.0),
		glm::vec4(+1.0, +1.0, 1.0, 1.0)
	};

	glBufferData(GL_ARRAY_BUFFER, 4*sizeof(glm::vec4), (void*)quadVertexes, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 0);

	glUseProgram(m_program->m_id);

	m_locId_butterflySampler = m_program->getUniformLocationId("butterflySampler");
	m_locId_nLayers = m_program->getUniformLocationId("nLayers");
	m_locId_pass = m_program->getUniformLocationId("pass");
	m_locId_imgSampler = m_program->getUniformLocationId("imgSampler");

	glUniform1i(m_locId_butterflySampler, BUTTERFLY_UNIT);

	glBindVertexArray(0);
}

void FFTProgram::drawQuad()
{
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

/////////////////////////////////////////////////

VariancesProgram::VariancesProgram() :
	m_program(new ShaderProgram({ ShaderStages::Vertex::bruneton_variances, ShaderStages::Fragment::bruneton_variances }))
{
	glBindVertexArray(m_vertexArray.m_id);
	glBindBuffer(GL_ARRAY_BUFFER, m_fullscreenQuadVertexBuffer.m_id);

	glm::vec4 quadVertexes[4] = {
		glm::vec4(-1.0, -1.0, 0.0, 0.0),
		glm::vec4(+1.0, -1.0, 1.0, 0.0),
		glm::vec4(-1.0, +1.0, 0.0, 1.0),
		glm::vec4(+1.0, +1.0, 1.0, 1.0)
	};

	glBufferData(GL_ARRAY_BUFFER, 4*sizeof(glm::vec4), (void*)quadVertexes, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 0);

	glUseProgram(m_program->m_id);
	
	m_locId_nSlopeVariance = m_program->getUniformLocationId("N_SLOPE_VARIANCE");
	m_locId_spectrum_1_2_Sampler = m_program->getUniformLocationId("spectrum_1_2_Sampler");
	m_locId_spectrum_3_4_Sampler = m_program->getUniformLocationId("spectrum_3_4_Sampler");
	m_locId_fftSize = m_program->getUniformLocationId("FFT_SIZE");
	m_locId_gridSizes = m_program->getUniformLocationId("GRID_SIZES");
	m_locId_slopeVarianceDelta = m_program->getUniformLocationId("slopeVarianceDelta");
	m_locId_c = m_program->getUniformLocationId("c");

	glUniform1f(m_locId_nSlopeVariance, (float)N_SLOPE_VARIANCE);
	glUniform1i(m_locId_spectrum_1_2_Sampler, SPECTRUM_1_2_UNIT);
	glUniform1i(m_locId_spectrum_1_2_Sampler, SPECTRUM_3_4_UNIT);
	glUniform1i(m_locId_fftSize, FFT_SIZE);

	glBindVertexArray(0);
}

void VariancesProgram::drawQuad()
{
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

/////////////////////////////////////////////////

RenderProgram::RenderProgram(
	ShaderProgram* program, 
	const glm::vec4& seaColour,
	float stretch,
	float scale
) : 
	m_program(program), m_seaColour(seaColour), m_stretch(stretch), m_scale(scale)
{
	glUseProgram(m_program->m_id);

	m_locId_M = m_program->getUniformLocationId("M");
	m_locId_MVP = m_program->getUniformLocationId("MVP");
	m_locId_worldCamera = m_program->getUniformLocationId("worldCamera"); // camera position in world space
	m_locId_worldSunDir = m_program->getUniformLocationId("worldSunDir"); // sun direction in world space
	m_locId_gridSize = m_program->getUniformLocationId("gridSize");
	m_locId_choppy = m_program->getUniformLocationId("choppy");
	m_locId_fftWavesSampler = m_program->getUniformLocationId("fftWavesSampler");
	m_locId_GRID_SIZES = m_program->getUniformLocationId("GRID_SIZES");
	m_locId_slopeVarianceSampler = m_program->getUniformLocationId("slopeVarianceSampler");
	m_locId_seaColour = m_program->getUniformLocationId("seaColour");
	m_locId_f_depthCoef = m_program->getUniformLocationId("f_depthCoef");
	m_locId_stretch = m_program->getUniformLocationId("stretch");
	m_locId_scale = m_program->getUniformLocationId("scale");

    glUniform1i(m_locId_fftWavesSampler, FFT_A_UNIT);
    glUniform1i(m_locId_slopeVarianceSampler, SLOPE_VARIANCE_UNIT);
    glUniform4f(m_locId_GRID_SIZES, GRID1_SIZE, GRID2_SIZE, GRID3_SIZE, GRID4_SIZE);
    glUniform2f(m_locId_gridSize, 0.01f, 0.01f);
    glUniform1f(m_locId_choppy, choppy);
	glUniform4f(m_locId_seaColour, m_seaColour.r, m_seaColour.g, m_seaColour.b, m_seaColour.a);
	glUniform1f(m_locId_stretch, m_stretch);
	glUniform1f(m_locId_scale, m_scale);
}

/////////////////////////////////////////////////

BrunetonWater::BrunetonWater(const glm::vec4& seaColour, float stretch, float scale) : 
	Water(new ShaderProgram({ ShaderStages::Vertex::bruneton_render, ShaderStages::Fragment::bruneton_render })),
	m_renderProgram(m_program, seaColour, stretch, scale), 
	m_fftXProgram(true), 
	m_fftYProgram(false)
{
	/*
	m_overlay_bar = TwNewBar("Ocean");
    TwAddVarCB(m_overlay_bar, "Wind speed", TW_TYPE_FLOAT, setFloat, getFloat, &WIND, "min=3.0 max=21.0 step=1.0 group=Spectrum");
    TwAddVarCB(m_overlay_bar, "Inv. wave age", TW_TYPE_FLOAT, setFloat, getFloat, &OMEGA, "min=0.84 max=5.0 step=0.1 group=Spectrum");
    TwAddVarCB(m_overlay_bar, "Amplitude", TW_TYPE_FLOAT, setFloat, getFloat, &A, "min=0.01 max=1000.0 step=0.01 group=Spectrum");
    TwAddButton(m_overlay_bar, "Generate", _computeSlopeVarianceTex, NULL, "group=Spectrum");

    TwAddVarRW(m_overlay_bar, "Sea color", TW_TYPE_COLOR4F, &seaColor, "group=Rendering");
    TwAddVarRW(m_overlay_bar, "Exposure", TW_TYPE_FLOAT, &hdrExposure, "min=0.01 max=4.0 step=0.01 group=Rendering");
    TwAddVarRW(m_overlay_bar, "Choppy", TW_TYPE_BOOL8, &choppy, "group=Rendering");
    TwAddVarCB(m_overlay_bar, "Sea", TW_TYPE_BOOL8, setBool, getBool, &seaContrib, "group=Rendering");
    TwAddVarCB(m_overlay_bar, "Sun", TW_TYPE_BOOL8, setBool, getBool, &sunContrib, "group=Rendering");
    TwAddVarCB(m_overlay_bar, "Sky", TW_TYPE_BOOL8, setBool, getBool, &skyContrib, "group=Rendering");
	*/

    float *data = new float[16*64*3];
	FILE* f;
	fopen_s(&f, "bruneton_irradiance.raw", "rb");
    fread(data, 1, 16*64*3*sizeof(float), f);
    fclose(f);
    glActiveTexture(GL_TEXTURE0 + IRRADIANCE_UNIT);
	glBindTexture(GL_TEXTURE_2D, m_irradianceTex.m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F_ARB, 64, 16, 0, GL_RGB, GL_FLOAT, data);
    delete[] data;

    int res = 64;
    int nr = res / 2;
    int nv = res * 2;
    int nb = res / 2;
    int na = 8;
    fopen_s(&f, "bruneton_inscatter.raw", "rb");
    data = new float[nr*nv*nb*na*4];
    fread(data, 1, nr*nv*nb*na*4*sizeof(float), f);
    fclose(f);
    glActiveTexture(GL_TEXTURE0 + INSCATTER_UNIT);
	glBindTexture(GL_TEXTURE_3D, m_inscatterTex.m_id);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F_ARB, na*nb, nv, nr, 0, GL_RGBA, GL_FLOAT, data);
    delete[] data;

    data = new float[256*64*3];
    fopen_s(&f, "bruneton_transmittance.raw", "rb");
    fread(data, 1, 256*64*3*sizeof(float), f);
    fclose(f);
    glActiveTexture(GL_TEXTURE0 + TRANSMITTANCE_UNIT);
	glBindTexture(GL_TEXTURE_2D, m_transmittanceTex.m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F_ARB, 256, 64, 0, GL_RGB, GL_FLOAT, data);
    delete[] data;

    float maxAnisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

    glActiveTexture(GL_TEXTURE0 + SKY_UNIT);
	glBindTexture(GL_TEXTURE_2D, m_skyTex.m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, skyTexSize, skyTexSize, 0, GL_RGBA, GL_FLOAT, NULL);
    glGenerateMipmap(GL_TEXTURE_2D);

    unsigned char* img = new unsigned char[512 * 512 + 38];
    fopen_s(&f, "bruneton_noise.pgm", "rb");
    fread(img, 1, 512 * 512 + 38, f);
    fclose(f);
    glActiveTexture(GL_TEXTURE0 + NOISE_UNIT);
	glBindTexture(GL_TEXTURE_2D, m_noiseTex.m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 512, 512, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, img + 38);
    glGenerateMipmap(GL_TEXTURE_2D);
    delete[] img;

    glActiveTexture(GL_TEXTURE0 + SPECTRUM_1_2_UNIT);
	glBindTexture(GL_TEXTURE_2D, m_spectrum12Tex.m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, FFT_SIZE, FFT_SIZE, 0, GL_RGB, GL_FLOAT, NULL);

    glActiveTexture(GL_TEXTURE0 + SPECTRUM_3_4_UNIT);
	glBindTexture(GL_TEXTURE_2D, m_spectrum34Tex.m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, FFT_SIZE, FFT_SIZE, 0, GL_RGB, GL_FLOAT, NULL);

    glActiveTexture(GL_TEXTURE0 + SLOPE_VARIANCE_UNIT);
	glBindTexture(GL_TEXTURE_3D, m_slopeVarianceTex.m_id);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE_ALPHA16F_ARB, N_SLOPE_VARIANCE, N_SLOPE_VARIANCE, N_SLOPE_VARIANCE, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, NULL);

    glActiveTexture(GL_TEXTURE0 + FFT_A_UNIT);
	glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, m_fftaTex.m_id);
    glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
    glTexImage3D(GL_TEXTURE_2D_ARRAY_EXT, 0, GL_RGBA16F_ARB, FFT_SIZE, FFT_SIZE, 5, 0, GL_RGB, GL_FLOAT, NULL);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY_EXT);

    glActiveTexture(GL_TEXTURE0 + FFT_B_UNIT);
	glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, m_fftbTex.m_id);
    glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
    glTexImage3D(GL_TEXTURE_2D_ARRAY_EXT, 0, GL_RGBA16F_ARB, FFT_SIZE, FFT_SIZE, 5, 0, GL_RGB, GL_FLOAT, NULL);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY_EXT);

    glActiveTexture(GL_TEXTURE0 + BUTTERFLY_UNIT);
	glBindTexture(GL_TEXTURE_2D, m_butterflyTex.m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    data = computeButterflyLookupTexture();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, FFT_SIZE, PASSES, 0, GL_RGBA, GL_FLOAT, data);
    delete[] data;

    generateWavesSpectrum();

	glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_variancesFbo.m_id);
    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

	glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_fftFbo1.m_id);
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    GLenum drawBuffers[5] = {
        GL_COLOR_ATTACHMENT0_EXT,
        GL_COLOR_ATTACHMENT1_EXT,
        GL_COLOR_ATTACHMENT2_EXT,
        GL_COLOR_ATTACHMENT3_EXT,
        GL_COLOR_ATTACHMENT4_EXT
    };
    glDrawBuffers(5, drawBuffers);
    for (int i = 0; i < 5; ++i) {
		glFramebufferTextureLayer(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + i, m_fftaTex.m_id, 0, i);
    }
    glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

	glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_fftFbo2.m_id);
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glFramebufferTexture(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, m_fftaTex.m_id, 0);
	glFramebufferTexture(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, m_fftbTex.m_id, 0);
    glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

	glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_fbo.m_id);
    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

    computeSlopeVarianceTex();
}

void BrunetonWater::update(const WorldClock& worldClock)
{
	glBindVertexArray(m_initProgram.m_vertexArray.m_id);
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_fftFbo1.m_id);
    glViewport(0, 0, FFT_SIZE, FFT_SIZE);
	glUseProgram(m_initProgram.m_program->m_id);
	glUniform1f(m_initProgram.m_locId_fftSize, (float)FFT_SIZE);
	glUniform4f(m_initProgram.m_locId_inverseGridSizes,
        2.0f * PI * FFT_SIZE / GRID1_SIZE,
        2.0f * PI * FFT_SIZE / GRID2_SIZE,
        2.0f * PI * FFT_SIZE / GRID3_SIZE,
        2.0f * PI * FFT_SIZE / GRID4_SIZE);
	glUniform1f(m_initProgram.m_locId_t, (float)worldClock.getT());
    m_initProgram.drawQuad();
	glBindVertexArray(0);

    // FFT passes

	glBindVertexArray(m_fftXProgram.m_vertexArray.m_id);
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_fftFbo2.m_id);
	glUseProgram(m_fftXProgram.m_program->m_id);
	glUniform1i(m_fftXProgram.m_locId_nLayers, choppy ? 5 : 3);
    for (int i = 0; i < PASSES; ++i) 
	{
		glUniform1f(m_fftXProgram.m_locId_pass, float(i + 0.5) / PASSES);
        if (i%2 == 0) 
		{
			glUniform1i(m_fftXProgram.m_locId_imgSampler, FFT_A_UNIT);
            glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
        } 
		else 
		{
            glUniform1i(m_fftXProgram.m_locId_imgSampler, FFT_B_UNIT);
            glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
        }
        m_fftXProgram.drawQuad();
    }

	glBindVertexArray(m_fftYProgram.m_vertexArray.m_id);
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_fftFbo2.m_id);
	glUseProgram(m_fftYProgram.m_program->m_id);
	glUniform1i(m_fftYProgram.m_locId_nLayers, choppy ? 5 : 3);
    for (int i = PASSES; i < 2 * PASSES; ++i) 
	{
        glUniform1f(m_fftYProgram.m_locId_pass, float(i - PASSES + 0.5) / PASSES);
        if (i%2 == 0) 
		{
            glUniform1i(m_fftYProgram.m_locId_imgSampler, FFT_A_UNIT);
            glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
        } 
		else 
		{
            glUniform1i(m_fftYProgram.m_locId_imgSampler, FFT_B_UNIT);
            glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
        }
        m_fftYProgram.drawQuad();
    }

    glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

    glActiveTexture(GL_TEXTURE0 + FFT_A_UNIT);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY_EXT);

	glViewport(0, 0, GLOBALS.getWindowWidth(), GLOBALS.getWindowHeight());
}

void BrunetonWater::addToOverlayBar(TwBar* bar)
{
	TwAddVarRW(bar, "Colour", TW_TYPE_COLOR4F, &m_renderProgram.m_seaColour, " group=Water ");
	TwAddVarRW(bar, "Stretch", TW_TYPE_FLOAT, &m_renderProgram.m_stretch, " group=Water ");
	TwAddVarRW(bar, "Scale", TW_TYPE_FLOAT, &m_renderProgram.m_scale, " group=Water ");
}

BrunetonWater* BrunetonWater::buildFromXMLNode(XMLNode& node)
{
	XMLChildFinder finder(node);

	return new BrunetonWater(
		finder.required("Colour", buildFVec4FromXMLNode), 
		finder.required("Stretch", buildFloatFromXMLNode), 
		finder.required("Scale", buildFloatFromXMLNode)
	);
}

#endif
