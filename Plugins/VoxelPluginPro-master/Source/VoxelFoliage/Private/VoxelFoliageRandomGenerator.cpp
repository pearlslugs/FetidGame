// Copyright 2021 Phyronnaz

#include "VoxelFoliageRandomGenerator.h"
#include "VoxelFoliageProxy.h"
#include "VoxelFoliageSpawnSettings.h"
#include "VoxelUtilities/VoxelBaseUtilities.h"
#include "Math/Sobol.h"

FVoxelFoliageRandomGenerator::FVoxelFoliageRandomGenerator(const FVoxelIntBox& Bounds, const FVoxelFoliageProxy& Foliage)
{
	uint32 Seed = FVoxelUtilities::MurmurHash32(Bounds.GetMurmurHash() ^ FVoxelUtilities::MurmurHash32(Foliage.Seed));
	PositionGenerator.Init(Foliage.SpawnSettings.RandomGenerator, Seed);

	//////////////////////////////////////////////////////////////////////////////
	
	Seed = FVoxelUtilities::MurmurHash32(Seed);
	GroupSplitStream.Initialize(Seed);

	Seed = FVoxelUtilities::MurmurHash32(Seed);
	DensityStream.Initialize(Seed);

	Seed = FVoxelUtilities::MurmurHash32(Seed);
	HeightRestrictionStream.Initialize(Seed);

	Seed = FVoxelUtilities::MurmurHash32(Seed);
	RandomInstanceIdStream.Initialize(Seed);

	//////////////////////////////////////////////////////////////////////////////
	
	Seed = FVoxelUtilities::MurmurHash32(Seed);
	ScaleStream.Initialize(Seed);
	
	Seed = FVoxelUtilities::MurmurHash32(Seed);
	YawStream.Initialize(Seed);
	
	Seed = FVoxelUtilities::MurmurHash32(Seed);
	PitchStream.Initialize(Seed);
	
	Seed = FVoxelUtilities::MurmurHash32(Seed);
	RandomAlignStream.Initialize(Seed);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelFoliageRandomGenerator::FPositionGenerator::Init(EVoxelFoliageRandomGenerator InRandomGenerator, uint32 Seed)
{
	RandomGenerator = InRandomGenerator;
	
	if (RandomGenerator == EVoxelFoliageRandomGenerator::Sobol)
	{
		const uint32 SeedX = Seed;
		const uint32 SeedY = FVoxelUtilities::MurmurHash32(Seed);
		Value = FSobol::Evaluate(0, CellBits, FIntPoint::ZeroValue, FIntPoint(SeedX, SeedY));
	}
	else
	{
		check(RandomGenerator == EVoxelFoliageRandomGenerator::Halton);
		Index = Seed;
	}
}

FVector2D FVoxelFoliageRandomGenerator::FPositionGenerator::GetValue() const
{
	if (RandomGenerator == EVoxelFoliageRandomGenerator::Sobol)
	{
		Value = FSobol::Next(Index++, CellBits, Value);
	}
	else
	{
		Value.X = FVoxelUtilities::Halton<2>(Index);
		Value.Y = FVoxelUtilities::Halton<3>(Index);
		Index++;
	}

	return Value;
}