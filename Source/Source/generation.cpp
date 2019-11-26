#include "stdafx.h"
//#include "biome.h"
#include "block.h"
#include "level.h"
#include "chunk.h"
#include "utilities.h"
#include "generation.h"
#include "chunk_manager.h"
#include <noise/noise.h>
#include "vendor/noiseutils.h"
#include <execution>

int maxHeight = 255;

using namespace noise;

void WorldGen::GenerateSimpleWorld(int xSize, int ySize, int zSize, float sparse, std::vector<ChunkPtr>& updateList)
{	// this loop is not parallelizable (unless VAO constructor is moved)
	for (int xc = 0; xc < xSize; xc++)
	{
		for (int yc = 0; yc < ySize; yc++)
		{
			for (int zc = 0; zc < zSize; zc++)
			{
				Chunk* init = Chunk::chunks[glm::ivec3(xc, yc, zc)] = new Chunk(true);
				init->SetPos(glm::ivec3(xc, yc, zc));
				updateList.push_back(init);
					
				for (int x = 0; x < Chunk::CHUNK_SIZE; x++)
				{
					for (int y = 0; y < Chunk::CHUNK_SIZE; y++)
					{
						for (int z = 0; z < Chunk::CHUNK_SIZE; z++)
						{
							if (Utils::get_random(0, 1) > sparse)
								continue;
							init->At(x, y, z).SetType(static_cast<Block::BlockType>(static_cast<int>(Utils::get_random(1, Block::bCount))), false);
						}
					}
				}
			}
		}
	}
}


void WorldGen::GenerateHeightMapWorld(int x, int z, LevelPtr level)
{
	module::DEFAULT_PERLIN_FREQUENCY;
	module::DEFAULT_PERLIN_LACUNARITY;
	module::DEFAULT_PERLIN_OCTAVE_COUNT;
	module::DEFAULT_PERLIN_PERSISTENCE;
	module::DEFAULT_PERLIN_QUALITY;
	module::DEFAULT_PERLIN_SEED;

	module::DEFAULT_RIDGED_FREQUENCY;
	module::DEFAULT_RIDGED_LACUNARITY;
	module::DEFAULT_RIDGED_OCTAVE_COUNT;
	module::DEFAULT_RIDGED_QUALITY;
	module::DEFAULT_RIDGED_SEED;

	module::Const canvas0;
	canvas0.SetConstValue(-1.0);
	module::Const gray;
	gray.SetConstValue(0);

	// add a little bit of plains to a blank canvas
	module::Perlin plains;
	plains.SetLacunarity(1);
	plains.SetOctaveCount(3);
	plains.SetFrequency(.1);
	plains.SetPersistence(.5);
	module::Perlin plainsPicker;
	plainsPicker.SetSeed(0);
	plainsPicker.SetLacunarity(0);
	plainsPicker.SetFrequency(.2);
	plainsPicker.SetOctaveCount(3);
	module::Select plainsSelect;
	plainsSelect.SetBounds(-1, 0);
	plainsSelect.SetEdgeFalloff(.1);
	plainsSelect.SetSourceModule(0, canvas0);
	plainsSelect.SetSourceModule(1, plains);
	plainsSelect.SetControlModule(plainsPicker);
	module::Select canvas1;
	canvas1.SetEdgeFalloff(1.5);
	canvas1.SetSourceModule(0, canvas0);
	canvas1.SetSourceModule(1, plainsSelect);
	canvas1.SetControlModule(canvas0);

	module::Perlin hillsLumpy;
	hillsLumpy.SetFrequency(.2);
	hillsLumpy.SetOctaveCount(2);
	hillsLumpy.SetPersistence(1.5);
	module::Const hillsHeight;
	hillsHeight.SetConstValue(1);
	module::Add hills;
	hills.SetSourceModule(0, hillsLumpy);
	hills.SetSourceModule(1, hillsHeight);
	module::Perlin hillsPicker;
	hillsPicker.SetSeed(1);
	hillsPicker.SetLacunarity(0);
	hillsPicker.SetFrequency(.2);
	hillsPicker.SetOctaveCount(3);
	module::Select hillsSelect;
	hillsSelect.SetBounds(-1, 0);
	hillsSelect.SetEdgeFalloff(.1);
	hillsSelect.SetSourceModule(0, canvas0);
	hillsSelect.SetSourceModule(1, hills);
	hillsSelect.SetControlModule(hillsPicker);
	module::Select canvas2;
	canvas2.SetEdgeFalloff(1.1);
	canvas2.SetBounds(-1, -.5);
	canvas2.SetSourceModule(0, canvas1);
	canvas2.SetSourceModule(1, hillsSelect);
	canvas2.SetControlModule(canvas1);

	module::RidgedMulti tunneler;
	// higher lacunarity = thinner tunnels
	tunneler.SetLacunarity(2.);
	// connectivity/complexity of tunnels (unsure)
	tunneler.SetOctaveCount(5);
	// higher frequency = more common, thicker tunnels 
	// raise lacunarity as frequency decreases
	tunneler.SetFrequency(.01);
	module::Invert inverter;
	inverter.SetSourceModule(0, tunneler);

	//module::Cylinders one;
	//module::RidgedMulti two;
	//module::Perlin control;
	//module::Blend blender;
	//blender.SetSourceModule(0, one);
	//blender.SetSourceModule(1, two);
	//blender.SetControlModule(control);

	//module::Const one1;
	//module::Voronoi two1;
	//module::Perlin control1;
	//module::Blend blender1;
	//blender1.SetSourceModule(0, one1);
	//blender1.SetSourceModule(1, two1);
	//blender1.SetControlModule(blender);

	//module::Blend poop;
	//poop.SetSourceModule(0, blender);
	//poop.SetSourceModule(1, blender1);
	//poop.SetControlModule(blender1);
	//noise.GetValue()
	utils::NoiseMap heightMap;
	utils::NoiseMapBuilderPlane heightMapBuilder;
	heightMapBuilder.SetSourceModule(canvas2);
	heightMapBuilder.SetDestNoiseMap(heightMap);
	heightMapBuilder.SetDestSize(Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE);

	int counter = 0;
	for (int xc = 0; xc < x; xc++)
	{
		for (int zc = 0; zc < z; zc++)
		{
			heightMapBuilder.SetBounds(xc, xc + 1, zc, zc + 1);
			heightMapBuilder.Build();

			// generate EVERYTHING
			for (int i = 0; i < Chunk::CHUNK_SIZE; i++)
			{
				for (int k = 0; k < Chunk::CHUNK_SIZE; k++)
				{
					int worldX = xc * Chunk::CHUNK_SIZE + i;
					int worldZ = zc * Chunk::CHUNK_SIZE + k;
					//utils::Color* color = height.GetSlabPtr(i, k);
					float height = *heightMap.GetConstSlabPtr(i, k);
					height = height < -1 ? -1 : height;

					int y = (int)Utils::mapToRange(height, -1.f, 1.f, -0.f, 100.f);
					level->UpdateBlockAt(glm::ivec3(worldX, y, worldZ), Block::BlockType::bGrass);

					// generate subsurface
					for (int j = -10; j < y; j++)
						level->UpdateBlockAt(glm::ivec3(worldX, j, worldZ), Block::BlockType::bStone);

					// extra layer(s) of grass on the top
					level->UpdateBlockAt(glm::ivec3(worldX, y - 1, worldZ), Block::BlockType::bDirt);
					level->UpdateBlockAt(glm::ivec3(worldX, y - 2, worldZ), Block::BlockType::bDirt);
					level->UpdateBlockAt(glm::ivec3(worldX, y - 3, worldZ), Block::BlockType::bDirt);
				}
			}

			// generate tunnels
			//std::for_each(
			//	std::execution::par,
			//	Chunk::chunks.begin(),
			//	Chunk::chunks.end(),
			//	[&](const std::pair<glm::ivec3, Chunk*>& p)
			//{
			//	if (!p.second)
			//		return;
			//	for (int xb = 0; xb < Chunk::CHUNK_SIZE; xb++)
			//	{
			//		for (int yb = 0; yb < Chunk::CHUNK_SIZE; yb++)
			//		{
			//			for (int zb = 0; zb < Chunk::CHUNK_SIZE; zb++)
			//			{
			//				glm::dvec3 pos = (p.first * Chunk::CHUNK_SIZE) + glm::ivec3(xb, yb, zb);
			//				double val = tunneler.GetValue(pos.x, pos.y, pos.z);
			//				//std::cout << val << '\n';
			//				if (val > .9)
			//					p.second->At(xb, yb, zb).SetType(Block::bAir);
			//			}
			//		}
			//	}
			//});

			//utils::WriterBMP writer;
			//writer.SetSourceImage(image);
			//writer.SetDestFilename(std::string("tutorial" + std::to_string(counter++) + ".bmp"));
			//writer.WriteDestFile();
		}
	}


}


static module::Perlin snowCover;
static module::Const canvas0;
static module::Const gray;
static module::Perlin plains;
static module::Const plainsHeight;
static module::Add plainsFinal;
static module::Perlin plainsPicker;
static module::Select plainsSelect;
static module::Select canvas1;
static module::Perlin hillsLumpy;
static module::Const hillsHeight;
static module::Add hills;
static module::Perlin hillsPicker;
static module::Select hillsSelect;
static module::Select canvas2;
static module::RidgedMulti tunneler;
static module::Invert inverter;
//static utils::NoiseMap heightMap;
//static utils::NoiseMapBuilderPlane heightMapBuilder;
static module::RidgedMulti oceans;
static module::Const oceanHeight;
static module::Const oceanSmoothValue;
static module::Multiply oceanSmooth;
static module::Add finalOcean;
static module::Select finalCanvas;

static module::RidgedMulti riversBase;
static module::Turbulence rivers;
//static utils::NoiseMap riverMap;
//static utils::NoiseMapBuilderPlane riverMapBuilder;

static model::Plane riverMapBuilder;
static model::Plane heightMapBuilder;

static module::Perlin temperatureNoise;
static module::Perlin humidityNoise;
static model::Plane temperature;
static model::Plane humidity;
void WorldGen::InitNoiseFuncs()
{
	{
		// init temp + humidity maps
		temperatureNoise.SetFrequency(.002);
		temperatureNoise.SetSeed(0);
		temperatureNoise.SetOctaveCount(1);
		humidityNoise.SetFrequency(.003);
		humidityNoise.SetSeed(1);
		humidityNoise.SetOctaveCount(1);
		temperature.SetModule(temperatureNoise);
		humidity.SetModule(humidityNoise);

		snowCover.SetFrequency(.01);
		canvas0.SetConstValue(-1.0);
		gray.SetConstValue(0);

		riversBase.SetLacunarity(5);
		//riversBase.SetPersistence(.5);
		riversBase.SetNoiseQuality(noise::NoiseQuality::QUALITY_STD);
		riversBase.SetOctaveCount(3);
		riversBase.SetFrequency(.0100);
		riversBase.SetNoiseQuality(NoiseQuality::QUALITY_BEST);

		rivers.SetSourceModule(0, riversBase);
		rivers.SetRoughness(1);
		rivers.SetPower(.5);
		rivers.SetFrequency(.2);

		//rivers.SetPersistence(.1);

		// add a little bit of plains to a blank canvas
		plains.SetLacunarity(1);
		plains.SetOctaveCount(3);
		plains.SetFrequency(.0016);
		plains.SetPersistence(.8);
		plains.SetNoiseQuality(NoiseQuality::QUALITY_STD);
		plainsHeight.SetConstValue(-.3); // height modifier
		plainsFinal.SetSourceModule(0, plainsHeight);
		plainsFinal.SetSourceModule(1, plains);
		plainsPicker.SetSeed(0);
		plainsPicker.SetLacunarity(0);
		plainsPicker.SetFrequency(.003);
		plainsPicker.SetOctaveCount(3);
		plainsPicker.SetNoiseQuality(NoiseQuality::QUALITY_STD);
		plainsSelect.SetBounds(-1, 0); // % of world to be (-1 to 1)
		plainsSelect.SetEdgeFalloff(0.15);
		plainsSelect.SetSourceModule(0, canvas0);
		plainsSelect.SetSourceModule(1, plainsFinal);
		plainsSelect.SetControlModule(plainsPicker);
		canvas1.SetEdgeFalloff(.5);
		canvas1.SetSourceModule(0, canvas0);
		canvas1.SetSourceModule(1, plainsSelect);
		canvas1.SetControlModule(canvas0);

		hillsLumpy.SetFrequency(.006);
		hillsLumpy.SetOctaveCount(2);
		hillsLumpy.SetPersistence(1.0);
		hillsLumpy.SetNoiseQuality(NoiseQuality::QUALITY_STD);
		hillsHeight.SetConstValue(0);
		hills.SetSourceModule(0, hillsLumpy);
		hills.SetSourceModule(1, hillsHeight);
		hillsPicker.SetSeed(1);
		hillsPicker.SetLacunarity(0);
		hillsPicker.SetFrequency(.003);
		hillsPicker.SetOctaveCount(3);
		hillsPicker.SetNoiseQuality(NoiseQuality::QUALITY_STD);
		hillsSelect.SetBounds(-1, -.2);
		hillsSelect.SetEdgeFalloff(.2);
		hillsSelect.SetSourceModule(0, canvas0);
		hillsSelect.SetSourceModule(1, hills);
		hillsSelect.SetControlModule(hillsPicker);
		canvas2.SetEdgeFalloff(.5);
		canvas2.SetBounds(-1, -.8); // this value makes a big difference somehow
		canvas2.SetSourceModule(0, canvas1);
		canvas2.SetSourceModule(1, hillsSelect);
		canvas2.SetControlModule(canvas1);


		// do oceans very last
		oceans.SetSeed(2);
		oceans.SetOctaveCount(1);
		oceans.SetFrequency(.017);
		oceans.SetNoiseQuality(NoiseQuality::QUALITY_STD);
		oceanSmoothValue.SetConstValue(.3);
		oceanHeight.SetConstValue(-1.4);
		oceanSmooth.SetSourceModule(0, oceans);
		oceanSmooth.SetSourceModule(1, oceanSmoothValue);
		finalOcean.SetSourceModule(0, oceanSmooth);
		finalOcean.SetSourceModule(1, oceanHeight);
		finalCanvas.SetEdgeFalloff(.21);
		finalCanvas.SetBounds(-.99, 1);
		finalCanvas.SetSourceModule(0, finalOcean);
		finalCanvas.SetSourceModule(1, canvas2);
		finalCanvas.SetControlModule(canvas2);

		// higher lacunarity = thinner tunnels
		tunneler.SetLacunarity(2.);
		// connectivity/complexity of tunnels (unsure)
		tunneler.SetOctaveCount(5);
		// higher frequency = more common, thicker tunnels 
		// raise lacunarity as frequency decreases
		tunneler.SetFrequency(.01);
		inverter.SetSourceModule(0, tunneler);

		riverMapBuilder.SetModule(rivers);
		//riverMapBuilder.SetSourceModule(rivers);
		//riverMapBuilder.SetDestNoiseMap(riverMap);
		//riverMapBuilder.SetDestSize(Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE);

		heightMapBuilder.SetModule(finalCanvas);
		//heightMapBuilder.SetSourceModule(canvas2);
		//heightMapBuilder.SetDestNoiseMap(heightMap);
		//heightMapBuilder.SetDestSize(Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE);
	}
}


void WorldGen::GenerateChunk(glm::ivec3 cpos, LevelPtr level)
{
	// generate everything
	for (int i = 0; i < Chunk::CHUNK_SIZE; i++)			// x
	{
		int worldX = cpos.x * Chunk::CHUNK_SIZE + i;
		for (int k = 0; k < Chunk::CHUNK_SIZE; k++)		// z
		{
			// height-based values that don't care about y
			int worldZ = cpos.z * Chunk::CHUNK_SIZE + k;
			float height = heightMapBuilder.GetValue(worldX, worldZ);
			float riverVal = riverMapBuilder.GetValue(worldX, worldZ);
			double humidity = GetHumidity(worldX, worldZ);
			float slope = getSlope(heightMapBuilder, worldX, worldZ);

			// iterate over all of Y because there may be
			// aerial features like floating islands, etc.
			for (int j = 0; j < Chunk::CHUNK_SIZE; j++)	// y
			{
				int worldY = cpos.y * Chunk::CHUNK_SIZE + j;
				glm::ivec3 wpos(worldX, worldY, worldZ);
				int y = (int)Utils::mapToRange(height, -1.f, 1.f, 0.f, 150.f);
				if (worldY > y && worldY > 0) // early skip
					continue;
				const Biome& curBiome = BiomeManager::GetBiome(GetTemperature(worldX, worldY, worldZ), humidity, GetTerrainType(wpos));

				int riverModifier = 0;
				if (riverVal > 0 && slope < 0.004f)
					riverModifier = glm::clamp(Utils::mapToRange(riverVal, -.35f, .35f, -4.f, 5.f), 0.f, 30.f);
				int actualHeight = y - riverModifier;

				// TODO: make rivers have sand around/under them
				if (worldY > actualHeight && worldY < y - 1)
					level->GenerateBlockAt(wpos, Block::BlockType::bWater);
				if (worldY < 0 && worldY > actualHeight) // make ocean
					level->GenerateBlockAt(wpos, Block::BlockType::bWater);

				// top cover
				if (worldY == actualHeight)
				{
					// surface features
					level->GenerateBlockAt(wpos, curBiome.surfaceCover);

					// generate surface prefabs
					if (worldY > 0)
					{
						for (const auto& p : curBiome.surfaceFeatures)
						{
							if (Utils::get_random(0, 1) < p.first && actualHeight == y)
								GeneratePrefab(PrefabManager::GetPrefab(p.second), wpos + glm::ivec3(0, 1, 0), level);
						}
					}
				}
				// just under top cover
				if (worldY >= actualHeight - 3 && worldY < actualHeight)
					level->GenerateBlockAt(glm::ivec3(worldX, worldY, worldZ), Block::BlockType::bDirt);

				// generate subsurface layer (rocks)
				if (worldY >= -30 && worldY < actualHeight - 3)
				{
					level->GenerateBlockAt(glm::ivec3(worldX, worldY, worldZ), Block::BlockType::bStone);
					if (Utils::get_random(0, 1) > .99999f)
						GeneratePrefab(PrefabManager::GetPrefab(Prefab::DungeonSmall), wpos, level);
				}
			}
		}
	}

	// generate simple tunnels
	for (int xb = 0; xb < Chunk::CHUNK_SIZE; xb++)
	{
		for (int yb = 0; yb < Chunk::CHUNK_SIZE; yb++)
		{
			for (int zb = 0; zb < Chunk::CHUNK_SIZE; zb++)
			{
				glm::dvec3 pos = (cpos * Chunk::CHUNK_SIZE) + glm::ivec3(xb, yb, zb);
				double val = tunneler.GetValue(pos.x, pos.y, pos.z);
				if (val > .9 && level->GetBlockAt(pos).GetType() != Block::bWater)
					//level->GenerateBlockAtCheap(glm::ivec3(pos.x, pos.y, pos.z), Block::bAir);
					level->GenerateBlockAt(glm::ivec3(pos.x, pos.y, pos.z), Block::bAir);
			}
		}
	}


}


WorldGen::TerrainType WorldGen::GetTerrainType(glm::ivec3 wpos)
{
	// TODO: special cases of height e.g. being very deep (underworld, etc.) or being high (sky islands)

	// the order of these gotta be in the order in which they're generated
	double pVal = plainsSelect.GetValue(wpos.x, 0, wpos.z);
	if (pVal != canvas0.GetConstValue())
		return TerrainType::tPlains;
	double hVal = hillsSelect.GetValue(wpos.x, 0, wpos.z);
	if (hVal != canvas0.GetConstValue())
		return TerrainType::tHills;
	return TerrainType::tOcean; // anywhere no biome has been generated will be this
}


double WorldGen::GetTemperature(double x, double y, double z)
{
	return temperature.GetValue(x, z) - y * .007;
}


double WorldGen::GetHumidity(double x, double z)
{
	return humidity.GetValue(x, z);
}


void WorldGen::GeneratePrefab(const Prefab& prefab, glm::ivec3 wpos, LevelPtr level)
{
	for (const auto& pair : prefab.blocks)
	{
		// written blocks so they don't get overwritten by normal terrain generation
		level->GenerateBlockAt(wpos + pair.first, pair.second);
	}
}


static double magic = .2; // increase isolevel by this amount (makes it somewhat accurate)
void WorldGen::Generate3DNoiseChunk(glm::ivec3 cpos, LevelPtr level)
{
	for (int xb = 0; xb < Chunk::CHUNK_SIZE; xb++)
	{
		for (int yb = 0; yb < Chunk::CHUNK_SIZE; yb++)
		{
			for (int zb = 0; zb < Chunk::CHUNK_SIZE; zb++)
			{
				glm::dvec3 pos = (cpos * Chunk::CHUNK_SIZE) + glm::ivec3(xb, yb, zb);

				// method 2
				level->GenerateBlockAt(glm::ivec3(pos), Block::bAir); // air by default
				cell Cell = Chunk::buildCellFromVoxel(pos);
				for (double& density : Cell.val)
					if (Utils::mapToRange(density, -1.f, 1.f, 0.f, 1.f) > Chunk::isolevel + magic)
					{
						level->GenerateBlockAt(glm::ivec3(pos), Block::bGrass); // replace by grass
						continue;
					}
			}
		}
	}
}


// density value of point for cube marched worlds
double WorldGen::GetDensity(const glm::vec3& wpos)
{
	static bool init = true;
	static module::RidgedMulti dense;
	
	if (init)
	{
		// higher lacunarity = thinner tunnels
		dense.SetLacunarity(2.);
		// connectivity/complexity of tunnels (unsure)
		dense.SetOctaveCount(3);
		// higher frequency = more common, thicker tunnels 
		// raise lacunarity as frequency decreases
		dense.SetFrequency(.02);
		// init generator here
	}

  glm::vec3 positions[] =
  {
    { -.5f, -.5f,  .5f },
    {  .5f, -.5f,  .5f },
    {  .5f, -.5f, -.5f },
    { -.5f, -.5f, -.5f },
    { -.5f,  .5f,  .5f },
    {  .5f,  .5f,  .5f },
    {  .5f,  .5f, -.5f },
    { -.5f,  .5f, -.5f }
  };

  double densidee = dense.GetValue(wpos.x, wpos.y, wpos.z);

	return densidee;
}


// slope of point for determining if certain features (e.g. water) can be placed there
float WorldGen::getSlope(model::Plane& pl, int x, int z)
{
	//float height = *heightmap.GetConstSlabPtr(x, z);
	float height = pl.GetValue(x, z);

	// Compute the differentials by stepping over 1 in both directions.
	float val1 = pl.GetValue(x + 1, z);
	float val2 = pl.GetValue(x, z + 1);
	
	//if (val1 == 0 || val2 == 0)
	//	return 0;
	float dx = val1 - height;
	float dz = val2 - height;
	
	// The "steepness" is the magnitude of the gradient vector
	// For a faster but less accurate computation, you can just use abs(dx) + abs(dy)
	return glm::sqrt(dx * dx + dz * dz);
}
