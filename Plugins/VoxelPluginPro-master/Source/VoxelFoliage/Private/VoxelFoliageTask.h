// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelAsyncWork.h"
#include "VoxelCancelCounter.h"

class FVoxelFoliageProxy;
class FVoxelFoliageBucket;
class FVoxelFoliageSubsystem;

DECLARE_UNIQUE_VOXEL_ID(FVoxelFoliageTaskId);

class FVoxelFoliageTask : public FVoxelAsyncWork
{
	GENERATED_VOXEL_ASYNC_WORK_BODY(FVoxelFoliageTask)

public:
	const FVoxelFoliageTaskId TaskId = FVoxelFoliageTaskId::New();
	
	const FVoxelIntBox Bounds;
	const FVoxelCancelTrackerGroup CancelTracker;
	const TVoxelWeakPtr<FVoxelFoliageBucket> WeakBucket;
	const TVoxelWeakPtr<FVoxelFoliageSubsystem> WeakFoliageSubsystem;
	// All the foliages must have the same biome
	const TArray<TVoxelWeakPtr<FVoxelFoliageProxy>> WeakFoliages;

	FVoxelFoliageTask(
		const FVoxelIntBox& Bounds,
		const FVoxelCancelTrackerGroup CancelTracker,
		const TVoxelWeakPtr<FVoxelFoliageBucket>& WeakBucket,
		FVoxelFoliageSubsystem& FoliageSubsystem,
		const TArray<TVoxelWeakPtr<FVoxelFoliageProxy>>& WeakFoliages);

	//~ Begin FVoxelAsyncWork Interface
	virtual void DoWork() override;
	//~ End FVoxelAsyncWork Interface
};