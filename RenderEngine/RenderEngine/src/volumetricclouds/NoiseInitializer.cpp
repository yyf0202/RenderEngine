#include "volumetricclouds/NoiseInitializer.h"

#include <gl/glew.h>
#include <iostream>

#include "datatables/MeshTable.h"
#include "datatables/ProgramTable.h"
#include "textures/Texture2D.h"
#include "textures/Texture3D.h"

Engine::CloudSystem::NoiseInitializer * Engine::CloudSystem::NoiseInitializer::INSTANCE = new Engine::CloudSystem::NoiseInitializer();

Engine::CloudSystem::NoiseInitializer & Engine::CloudSystem::NoiseInitializer::getInstance()
{
	return *Engine::CloudSystem::NoiseInitializer::INSTANCE;
}

Engine::CloudSystem::NoiseInitializer::NoiseInitializer()
{
	perlinWorleyGen = worleyGen = NULL;
	weatherGen = NULL;
	PerlinWorleyFBM = NULL;
	WorleyFBM = NULL;
	CurlNoise = NULL;
	WeatherData = NULL;

	initialized = false;
}

const Engine::TextureInstance * Engine::CloudSystem::NoiseInitializer::getPerlinWorleyFBM() const
{
	return PerlinWorleyFBM;
}

const Engine::TextureInstance * Engine::CloudSystem::NoiseInitializer::getWorleyFBM() const
{
	return WorleyFBM;
}

const Engine::TextureInstance * Engine::CloudSystem::NoiseInitializer::getCurlNoise() const
{
	return CurlNoise;
}

const Engine::TextureInstance * Engine::CloudSystem::NoiseInitializer::getWeatherData() const
{
	return WeatherData;
}

void Engine::CloudSystem::NoiseInitializer::init()
{
	if (initialized)
		return;

	initShader();
	initTextures();
	
	initialized = true;
}

void Engine::CloudSystem::NoiseInitializer::initShader()
{
	perlinWorleyGen = new Engine::VolumeTextureProgram("shaders/clouds/generation/perlinworley.comp");
	perlinWorleyGen->initialize();
	worleyGen = new Engine::VolumeTextureProgram("shaders/clouds/generation/worley.comp");
	worleyGen->initialize();
	weatherGen = new Engine::WeatherTextureProgram();
	weatherGen->initialize();
}

void Engine::CloudSystem::NoiseInitializer::initTextures()
{
	Engine::Texture3D * perlinWorley = new Engine::Texture3D("perlinworleyfbm", 128, 128, 128);
	perlinWorley->setGenerateMipMaps(false);
	perlinWorley->setMemoryLayoutFormat(GL_RGBA8);
	perlinWorley->setImageFormatType(GL_RGBA);
	perlinWorley->setPixelFormatType(GL_FLOAT);

	PerlinWorleyFBM = new Engine::TextureInstance(perlinWorley);
	PerlinWorleyFBM->setAnisotropicFilterEnabled(false);
	PerlinWorleyFBM->setRComponentWrapType(GL_REPEAT);
	PerlinWorleyFBM->setSComponentWrapType(GL_REPEAT);
	PerlinWorleyFBM->setTComponentWrapType(GL_REPEAT);
	PerlinWorleyFBM->setMagnificationFilterType(GL_NEAREST);
	PerlinWorleyFBM->setMinificationFilterType(GL_NEAREST);
	PerlinWorleyFBM->generateTexture();
	PerlinWorleyFBM->uploadTexture();
	//PerlinWorleyFBM->configureTexture();

	Engine::Texture3D * worley = new Engine::Texture3D("worleyfbm", 32, 32, 32);
	worley->setGenerateMipMaps(false);
	worley->setMemoryLayoutFormat(GL_RGBA8);
	worley->setImageFormatType(GL_RGBA);
	worley->setPixelFormatType(GL_FLOAT);

	WorleyFBM = new Engine::TextureInstance(worley);
	WorleyFBM->setAnisotropicFilterEnabled(false);
	WorleyFBM->setRComponentWrapType(GL_REPEAT);
	WorleyFBM->setSComponentWrapType(GL_REPEAT);
	WorleyFBM->setTComponentWrapType(GL_REPEAT);
	WorleyFBM->setMagnificationFilterType(GL_LINEAR);
	WorleyFBM->setMinificationFilterType(GL_LINEAR);
	WorleyFBM->generateTexture();
	WorleyFBM->uploadTexture();
	//WorleyFBM->configureTexture();

	Engine::Texture2D * curl = new Engine::Texture2D("curl", 0, 128, 128);
	curl->setGenerateMipMaps(false);
	curl->setMemoryLayoutFormat(GL_RGBA8);
	curl->setImageFormatType(GL_RGBA);
	curl->setPixelFormatType(GL_FLOAT);

	CurlNoise = new Engine::TextureInstance(curl);
	CurlNoise->setAnisotropicFilterEnabled(false);
	CurlNoise->setSComponentWrapType(GL_REPEAT);
	CurlNoise->setTComponentWrapType(GL_REPEAT);
	CurlNoise->setMagnificationFilterType(GL_LINEAR_MIPMAP_LINEAR);
	CurlNoise->setMinificationFilterType(GL_LINEAR_MIPMAP_LINEAR);
	CurlNoise->generateTexture();
	CurlNoise->uploadTexture();
	CurlNoise->configureTexture();

	Engine::Texture2D * weather = new Engine::Texture2D("weather", 0, 2048, 2048);
	weather->setGenerateMipMaps(false);
	weather->setMemoryLayoutFormat(GL_RGBA8);
	weather->setImageFormatType(GL_RGBA);
	weather->setPixelFormatType(GL_FLOAT);

	WeatherData = new Engine::TextureInstance(weather);
	WeatherData->setAnisotropicFilterEnabled(false);
	WeatherData->setSComponentWrapType(GL_MIRRORED_REPEAT);
	WeatherData->setTComponentWrapType(GL_MIRRORED_REPEAT);
	WeatherData->setMagnificationFilterType(GL_LINEAR);
	WeatherData->setMinificationFilterType(GL_LINEAR);
	WeatherData->generateTexture();
	WeatherData->uploadTexture();
	WeatherData->configureTexture();
}

void Engine::CloudSystem::NoiseInitializer::clean()
{
	if (perlinWorleyGen != NULL)
	{
		perlinWorleyGen->destroy();
		delete perlinWorleyGen;
	}


	if (worleyGen != NULL)
	{
		worleyGen->destroy();
		delete worleyGen;
	}
}

void Engine::CloudSystem::NoiseInitializer::render()
{
	init();
	
	std::cout << "Generating Perlin-Worley + 3 Worley octaves volume texture (128x128x128)..." << std::endl;
	glUseProgram(perlinWorleyGen->getProgramId());
	perlinWorleyGen->bindOutput(PerlinWorleyFBM);
	perlinWorleyGen->dispatch(128, 128, 128, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	std::cout << "Done!" << std::endl;

	glBindTexture(GL_TEXTURE_3D, PerlinWorleyFBM->getTexture()->getTextureId());
	glGenerateMipmap(GL_TEXTURE_3D);
	PerlinWorleyFBM->configureTexture();
	
	std::cout << "Generating 3 Worley octave volume texture (32x32x32)..." << std::endl;
	glUseProgram(worleyGen->getProgramId());
	worleyGen->bindOutput(WorleyFBM);
	worleyGen->dispatch(32, 32, 32, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	std::cout << "Done!" << std::endl;

	glBindTexture(GL_TEXTURE_3D, WorleyFBM->getTexture()->getTextureId());
	glGenerateMipmap(GL_TEXTURE_3D);
	WorleyFBM->configureTexture();

	std::cout << "Generating Weather texture (1024x1024)..." << std::endl;
	glUseProgram(weatherGen->getProgramId());
	weatherGen->bindOutput(WeatherData);
	weatherGen->dispatch(2048, 2048, 1, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	std::cout << "Done!" << std::endl;
	//clean();
}