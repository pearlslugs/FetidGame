// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelVector.h"

struct FVoxelBasis;
struct FVoxelIntBox;

struct FVoxelFoliageStats;
struct FVoxelFoliageSpawnSettings;

class FVoxelFoliageProxy;
class FVoxelFoliageSubsystem;
class FVoxelFoliageRayHandler;
class FVoxelFoliageRandomGenerator;

class FVoxelRuntimeSettings;
class FVoxelCancelTrackerGroup;
class FVoxelConstDataAccelerator;

struct FVoxelFoliageUtilities
{
	static TUniquePtr<FVoxelFoliageRayHandler> CreateRayHandler(
		const FVoxelFoliageSubsystem& FoliageSubsystem,
		const FVoxelFoliageSpawnSettings& SpawnSettings,
		const FVoxelCancelTrackerGroup& CancelTracker,
		const FVoxelIntBox& Bounds, 
		FVoxelFoliageStats& Stats);
	
	static FVoxelBasis GetBasisFromBounds(const FVoxelRuntimeSettings& Settings, const FVoxelIntBox& Bounds);

	static FVoxelVector GetRayDirection(
		const FVoxelRuntimeSettings& Settings,
		const FVoxelVector& Start,
		const FIntVector& ChunkPosition);
};