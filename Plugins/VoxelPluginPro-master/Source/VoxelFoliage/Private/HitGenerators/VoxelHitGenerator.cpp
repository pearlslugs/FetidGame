// Copyright 2021 Phyronnaz

#include "HitGenerators/VoxelHitGenerator.h"
#include "VoxelFoliageRandomGenerator.h"
#include "VoxelFoliageUtilities.h"
#include "VoxelFoliageProxy.h"
#include "VoxelRuntime.h"

FVoxelHitGenerator::FVoxelHitGenerator(const FParameters& Parameters)
	: Stats(Parameters.Stats)
	, Bounds(Parameters.Bounds)
	, Settings(Parameters.Settings)
	, Accelerator(Parameters.Accelerator)
	, CancelTracker(Parameters.CancelTracker)
	, RandomGenerator(Parameters.RandomGenerator)
	, SpawnSettings(Parameters.SpawnSettings)
	, Basis(FVoxelFoliageUtilities::GetBasisFromBounds(Settings, Bounds))
{
}

TArray<FVoxelFoliageHit> FVoxelHitGenerator::Generate()
{
	VOXEL_ASYNC_FUNCTION_COUNTER();

	ensure(Bounds.Size().X == Bounds.Size().Y && Bounds.Size().Y == Bounds.Size().Z);
	const int32 BoundsSize = Bounds.Size().X;

	const int32 NumRays = FMath::FloorToInt(FMath::Square(double(BoundsSize) / double(SpawnSettings.DistanceBetweenInstances.GetInVoxels(Settings.VoxelSize))));

	return GenerateImpl(NumRays);
}
