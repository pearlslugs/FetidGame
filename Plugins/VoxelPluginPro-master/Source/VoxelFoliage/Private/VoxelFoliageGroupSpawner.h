// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelIntBox.h"
#include "VoxelCancelCounter.h"
#include "VoxelFoliageProxy.h"

class FVoxelData;
class FVoxelFoliageDebug;
class FVoxelRuntimeSettings;
class FVoxelFoliageQueryCache;
class FVoxelFoliageRandomGenerator;

class FVoxelFoliageGroupSpawner
{
public:
	FVoxelFoliageStats& TotalStats;
	const FVoxelIntBox Bounds;
	const FVoxelCancelTrackerGroup CancelTracker;
	
	FVoxelFoliageRayHandler* const RayHandler;

	const FVoxelData& Data;
	const FVoxelConstDataAccelerator& Accelerator;
	const FVoxelFoliageQueryCache& DensityCache;

	const FVoxelRuntimeSettings& Settings;
	const FVoxelFoliageDebug& FoliageDebug;
	
	const FVoxelFoliageProxy::FBiome BaseBiome;
	const FVoxelFoliageSpawnSettings& SpawnSettings;
	const FVoxelFoliageRandomGenerator& RandomGenerator;

	const TArray<FVoxelFoliageProxy*>& Foliages;
	const TMap<FVoxelFoliageGroupId, TArray<FVoxelFoliageProxy*>> GroupToFoliages;
	
	TMap<FVoxelFoliageProxyId, TVoxelSharedRef<FVoxelFoliageResultData>> Spawn() const;

private:
	TArray<FVoxelFoliageHit> FindHits() const;
	TMap<FVoxelFoliageGroupId, TArray<FVoxelFoliageHit>> SplitHitsByBiome(const TArray<FVoxelFoliageHit>& Hits) const;
	void SplitGroupHits(TMap<FVoxelFoliageProxy*, TArray<FVoxelFoliageHit>>& OutHits, FVoxelFoliageGroupId GroupId, const TArray<FVoxelFoliageHit>& Hits) const;
	
private:
	void ApplyDensity(const FVoxelFoliageProxy& Foliage, FVoxelFoliageStats& Stats, TArray<FVoxelFoliageHit>& Hits) const;
	void CheckFloating_Height(FVoxelFoliageStats& Stats, TArray<FVoxelFoliageHit>& Hits) const;
	void ComputeGradient_Height(FVoxelFoliageStats& Stats, TArray<FVoxelFoliageHit>& Hits) const;
	void ComputeWorldUp(FVoxelFoliageStats& Stats, TArray<FVoxelFoliageHit>& Hits) const;
	void CheckCanSpawn(const FVoxelFoliageProxy& Foliage, FVoxelFoliageStats& Stats, TArray<FVoxelFoliageHit>& Hits) const;

	void CreateTransforms(
		FVoxelFoliageTransforms& Transforms,
		const FVoxelFoliageProxy& Foliage,
		FVoxelFoliageStats& Stats,
		TArray<FVoxelFoliageHit>& Hits) const;

	void ComputeCustomData(
		TArray<float>& CustomData,
		const FVoxelFoliageProxy& Foliage,
		FVoxelFoliageStats& Stats,
		TArray<FVoxelFoliageHit>& Hits) const;
};