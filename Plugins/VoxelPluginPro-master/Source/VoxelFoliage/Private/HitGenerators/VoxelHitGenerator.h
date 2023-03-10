// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelBasis.h"
#include "VoxelIntBox.h"

struct FVoxelFoliageStats;
struct FVoxelFoliageDensity;
struct FVoxelFoliageSpawnSettings;

class FVoxelCancelTracker;
class FVoxelRuntimeSettings;
class FVoxelConstDataAccelerator;
class FVoxelCancelTrackerGroup;
class FVoxelFoliageRandomGenerator;

struct FVoxelFoliageHit
{
	// Relative to chunk bounds.min
	FVector LocalPosition = FVector(ForceInit);
	FVector Normal = FVector(ForceInit);
	FVector WorldUp = FVector(ForceInit);
	
	struct FHeightGradient
	{
		FVoxelVector MinX = FVoxelVector(ForceInit);
		FVoxelVector MaxX = FVoxelVector(ForceInit);
		FVoxelVector MinY = FVoxelVector(ForceInit);
		FVoxelVector MaxY = FVoxelVector(ForceInit);
	};
	FHeightGradient HeightGradient;

	struct FHeightFloating
	{
		// These positions should be inside/outside the surface, else we should not be spawned
		FVoxelVector InsideSurface = FVoxelVector(ForceInit);
		FVoxelVector OutsideSurface = FVoxelVector(ForceInit);
	};
	FHeightFloating HeightFloating;

	FVoxelVector GetPosition(const FIntVector& ChunkPosition) const
	{
		return FVoxelVector(ChunkPosition) + LocalPosition;
	}
};

class FVoxelHitGenerator
{
public:
	FVoxelFoliageStats& Stats;
	const FVoxelIntBox Bounds;
	const FVoxelRuntimeSettings& Settings;
	const FVoxelConstDataAccelerator& Accelerator;
	const FVoxelCancelTrackerGroup& CancelTracker;

	const FVoxelFoliageRandomGenerator& RandomGenerator;
	const FVoxelFoliageSpawnSettings& SpawnSettings;
	
	const FVoxelBasis Basis;
	
	struct FParameters
	{
		FVoxelFoliageStats& Stats;
		const FVoxelIntBox Bounds;
		const FVoxelRuntimeSettings& Settings;
		const FVoxelConstDataAccelerator& Accelerator;
		const FVoxelCancelTrackerGroup& CancelTracker;
		
		const FVoxelFoliageRandomGenerator& RandomGenerator;
		const FVoxelFoliageSpawnSettings& SpawnSettings;
	};

	explicit FVoxelHitGenerator(const FParameters& Parameters);
	virtual ~FVoxelHitGenerator() = default;

public:
	TArray<FVoxelFoliageHit> Generate();

protected:
	virtual TArray<FVoxelFoliageHit> GenerateImpl(int32 NumRays) = 0;
};