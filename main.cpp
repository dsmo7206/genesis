#include <stdio.h>
#include <vector>
#include <iostream>

#include "glstuff.h"
#include "globals.h"
#include "scene.h"
#include "overlay.h"
#include "planet.h"
#include "utils.h"
#include "xml.h"
#include "camera.h"
#include "compute_queue.h"
#include "planet_data_buffer.h"
#include "world_clock.h"
#include "fullscreen_quad.h"
#include "gbuffer.h"
#include "bruneton_atmosphere.h"

void makeUniformBlocks(
	std::shared_ptr<Bruneton::ResUniformBlock>& resUniformBlock,
	std::shared_ptr<Bruneton::PlanetUniformBlock>& planetUniformBlock,
	std::shared_ptr<Bruneton::AtmosphereUniformBlock>& atmosphereUniformBlock,
	std::shared_ptr<Bruneton::DynamicDrawUniformBlock>& dynamicDrawUniformBlock
)
{
	resUniformBlock = std::make_shared<Bruneton::ResUniformBlock>();
	resUniformBlock->m_resR = 32;
	resUniformBlock->m_resMu = 128;
	resUniformBlock->m_resMuS = 32;
	resUniformBlock->m_resNu = 8;
	resUniformBlock->m_transmittance_w = 256;
	resUniformBlock->m_transmittance_h = 64;
	resUniformBlock->m_transmittance_integral_samples = 500;
	resUniformBlock->m_irradiance_integral_samples = 32;
	resUniformBlock->m_inscatter_integral_samples = 50;
	resUniformBlock->m_inscatter_spherical_integral_samples = 16;
	resUniformBlock->m_sky_w = 64;
	resUniformBlock->m_sky_h = 16;

	planetUniformBlock = std::make_shared<Bruneton::PlanetUniformBlock>();
	planetUniformBlock->m_rg = 6360.0;
	planetUniformBlock->m_rt = 6420.0;
	planetUniformBlock->m_rL = 6421.0;

	atmosphereUniformBlock = std::make_shared<Bruneton::AtmosphereUniformBlock>();
	atmosphereUniformBlock->m_betaR = glm::vec3(5.8e-3f, 1.35e-2f, 3.31e-2f);
	atmosphereUniformBlock->m_hr = 8.0f;
	atmosphereUniformBlock->m_hm = 3.0f;
	atmosphereUniformBlock->m_betaMSca = glm::vec3(3e-3f);
	atmosphereUniformBlock->m_betaMEx = atmosphereUniformBlock->m_betaMSca / 0.9f;
	atmosphereUniformBlock->m_mieG = 0.65f;

	dynamicDrawUniformBlock = std::make_shared<Bruneton::DynamicDrawUniformBlock>();
}

int _main(int argc, char** argv)
{
	// Load everything
	{
		// Load XML file
		XMLDocument doc;
		const std::string content = stringFromFile("settings.xml");
		doc.parse<0>((char*)content.c_str());

		XMLNode* node = doc.first_node();
		if (!nodeNameIs(*node, "Settings"))
			raiseXMLException(doc, "Expected Settings node");
		XMLChildFinder finder(*node);
	
		// Load settings from xml; this populates various fields
		// within the globals namespace which we need before
		// initialising OpenGL (namely: full screen or not)
		finder.required("Globals", loadGlobalsFromXMLNode);

		// Initialisations
		if (initialiseGL())
			return -1;
		initialiseOverlay();

		GLOBALS.initialise();
		initPlanetDataBufferAndConstants();
		ShaderStages::initialise();
		initialiseSkyBox();
	
		// Load everything else from xml, creating planets etc.
		// Because this constructs OpenGL objects, we need this 
		// to happen AFTER OpenGL has been initialised above, 
		// otherwise we get the program crashing.
		for (auto scenePtr : finder.requiredVector("Scenes", Scene::buildFromXMLNode))
			SCENE_MAP.emplace(scenePtr->m_name, scenePtr);
	
		GLOBALS.setInputToOverlay(GLOBALS.getInputToOverlay()); // Need to repeat here
	}

	// For speed computation
	double lastRefreshTime = glfwGetTime();
	double lastFrameTime = lastRefreshTime;
	int framesSinceLastRefresh = 0;

	WorldClock worldClock(1.0, 0.0);
	
	Bruneton::Atmosphere::initialiseShaders();
	
	std::shared_ptr<Bruneton::ResUniformBlock> resUniformBlock;
	std::shared_ptr<Bruneton::PlanetUniformBlock> planetUniformBlock;
	std::shared_ptr<Bruneton::AtmosphereUniformBlock> atmosphereUniformBlock;
	std::shared_ptr<Bruneton::DynamicDrawUniformBlock> dynamicDrawUniformBlock;
	
	//makeUniformBlocks(resUniformBlock, planetUniformBlock, atmosphereUniformBlock, dynamicDrawUniformBlock);
	
	//Bruneton::Atmosphere atmos(0, 0, resUniformBlock, planetUniformBlock, atmosphereUniformBlock, dynamicDrawUniformBlock);
	

	bool firstFrame = true;
	double frameStartTime;

	glViewport(0, 0, 1920, 1080);

	//GBuffer gbuffer;
	//gbuffer.init(1920, 1080);
	//gbuffer.init(1200, 800);
	//FullscreenQuad fsquad;
	//fsquad.m_texId = gbuffer.m_texturePosition.m_id;
	//fsquad.m_texId = gbuffer.m_textureDepth.m_id;
	//fsquad.m_texId = gbuffer.m_textureNormal.m_id;
	//fsquad.m_texId = atmos.m_textureTransmittance.m_id;
	float exposure;

	// Main loop
	int frame = 0;
	do
	{
		//exposure = (++frame) * 20.0 / 300.0;
		exposure = 3.0;
		if (frame == 300)
			frame = 0;

		// Get key presses etc
		glfwPollEvents();
		
		// Measure speed
		frameStartTime = glfwGetTime();
		const double deltaTime = frameStartTime - lastFrameTime; 
		lastFrameTime = frameStartTime;
		++framesSinceLastRefresh;

		if (frameStartTime - lastRefreshTime >= GLOBALS.m_diagnosticsRefreshTime)
		{
			GLOBALS.m_framesPerSecond = framesSinceLastRefresh / GLOBALS.m_diagnosticsRefreshTime;
			GLOBALS.m_millisPerFrame = 1000.0 / GLOBALS.m_framesPerSecond;
			framesSinceLastRefresh = 0;
			lastRefreshTime = frameStartTime;
		}
		
		// Update world clock
		worldClock.updateFromSystemClock(deltaTime);

		// Update scenes
		static double mouseXPos = GLOBALS.getWindowWidth() / 2;
		static double mouseYPos = GLOBALS.getWindowHeight() / 2;

		if (!GLOBALS.getInputToOverlay())
		{
			glfwGetCursorPos(appWindow, &mouseXPos, &mouseYPos);
			glfwSetCursorPos(appWindow, GLOBALS.getWindowWidth() / 2, GLOBALS.getWindowHeight() / 2);
		}

		for (auto &it : SCENE_MAP)
		{
			it.second->updateGeneral(worldClock, mouseXPos, mouseYPos);
			it.second->updateForCamera(it.second->m_cameras["Main"]);
		}

		// Run computes
		if (!firstFrame)
		{
			const double desiredFrameEndTime = frameStartTime + 1.0 / GLOBALS.m_desiredFPS;
			ComputeQueue::get().runUntil(desiredFrameEndTime);
		}

		//gbuffer.bindForWriting();
 
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw everything
		for (auto &it : SCENE_MAP) 
			it.second->draw(it.second->m_cameras["Main"]);
		
		if (firstFrame)
		{
			glFinish();

			const double t1 = glfwGetTime();
			ComputeQueue::get().runAll();

			const double t2 = glfwGetTime();
			printf("Total vertices: %u\n", PLANET_PATCH_CONSTANTS->m_totalVertices);
			printf("Took %lf sec\n", t2 - t1);

			firstFrame = false;
		}

		//atmos.draw();

		// Bind the screen for drawing
		//gbuffer.BindForReading();

		//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//glDisable(GL_DEPTH_TEST);
		//fsquad.draw(exposure);
		//glEnable(GL_DEPTH_TEST);

		drawOverlay();
		glfwSwapBuffers(appWindow);
		++GLOBALS.m_frameNumber;
	} 
	while (
		// Check if the ESC key was pressed or the window was closed
		glfwGetKey(appWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		!glfwWindowShouldClose(appWindow)
	);

	GLOBALS.m_shuttingDown = true;
 
	// Close GUI and OpenGL window, and terminate GLFW
	killOverlay();
	glfwTerminate();
 
	return 0;
}
 
int main(int argc, char** argv)
{
	try
	{
		return _main(argc, argv);
	}
	catch (std::exception& e)
	{
		glfwDestroyWindow(appWindow);
		printf("Caught exception: %s\n", e.what());
		printf("Press any key to continue...\n");
		std::getchar();
		return 1;
	}
}
