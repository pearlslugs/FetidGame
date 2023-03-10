// Copyright 2021 Phyronnaz

#include "VoxelFoliageTask.h"
#include "VoxelFoliageProxy.h"
#include "VoxelFoliageSubsystem.h"
#include "VoxelFoliageUtilities.h"
#include "VoxelFoliageGroupSpawner.h"
#include "VoxelFoliageRayHandler.h"
#include "VoxelFoliageDebug.h"
#include "VoxelFoliageDensityHelper.h"
#include "VoxelFoliageRandomGenerator.h"

#include "VoxelData/VoxelDataIncludes.h"

#include "Misc/ScopeExit.h"

DEFINE_UNIQUE_VOXEL_ID(FVoxelFoliageTaskId);

FVoxelFoliageTask::FVoxelFoliageTask(
	const FVoxelIntBox& Bounds,
	const FVoxelCancelTrackerGroup CancelTracker,
	const TVoxelWeakPtr<FVoxelFoliageBucket>& WeakBucket,
	FVoxelFoliageSubsystem& FoliageSubsystem,
	const TArray<TVoxelWeakPtr<FVoxelFoliageProxy>>& WeakFoliages)
	: FVoxelAsyncWork(STATIC_FNAME("Foliage Task"), EVoxelTaskType::FoliageBuild, EPriority::InvokersDistance, true)
	, Bounds(Bounds)
	, CancelTracker(CancelTracker)
	, WeakBucket(WeakBucket)
	, WeakFoliageSubsystem(FoliageSubsystem.AsShared())
	, WeakFoliages(WeakFoliages)
{
	check(WeakFoliages.Num() > 0);
	PriorityHandler = FVoxelPriorityHandler(Bounds, FoliageSubsystem);
}

#define CHECK_CANCELED() if (CancelTracker.IsCanceled()) return;

void FVoxelFoliageTask::DoWork()
{
	CHECK_CANCELED();

	const TVoxelSharedPtr<FVoxelFoliageSubsystem> FoliageSubsystem = WeakFoliageSubsystem.Pin();
	if (!FoliageSubsystem)
	{
		ensure(CancelTracker.IsCanceled());
		return;
	}
	
	const auto Data = FoliageSubsystem->GetSubsystem<FVoxelData>();
	const auto FoliageDebug = FoliageSubsystem->GetSubsystem<FVoxelFoliageDebug>();

	if (!Data || !FoliageDebug)
	{
		ensure(CancelTracker.IsCanceled());
		return;
	}

	TArray<FVoxelFoliageProxy*> Foliages;
	TArray<TVoxelSharedPtr<FVoxelFoliageProxy>> PinnedFoliages;
	for (const TVoxelWeakPtr<FVoxelFoliageProxy>& WeakFoliage : WeakFoliages)
	{
		const TVoxelSharedPtr<FVoxelFoliageProxy> Foliage = WeakFoliage.Pin();
		if (Foliage.IsValid())
		{
			if (Foliages.Num() > 0)
			{
				ensure(Foliages.Last()->Biome.Id == Foliage->Biome.Id);
				ensure(Foliages.Last()->Biome.OutputName == Foliage->Biome.OutputName);
				ensure(Foliages.Last()->Seed == Foliage->Seed);
				ensure(Foliages.Last()->SpawnSettings == Foliage->SpawnSettings);

				if (Foliages.Last()->Group.Id == Foliage->Group.Id && Foliage->Group.Id.IsValid())
				{
					ensure(Foliages.Last()->Biome.OutputName == Foliage->Biome.OutputName);
					ensure(Foliages.Last()->Biome.OutputValue == Foliage->Biome.OutputValue);
				}
			}
			Foliages.Add(Foliage.Get());
			PinnedFoliages.Add(Foliage);
		}
	}
	if (Foliages.Num() == 0)
	{
		ensure(CancelTracker.IsCanceled());
		return;
	}
	
	FVoxelFoliageStats Stats;
	ON_SCOPE_EXIT
	{
		for (FVoxelFoliageProxy* Foliage : Foliages)
		{
			Foliage->AddToTotalStats(Stats);
		}
	};
	VOXEL_SCOPED_STAT(Stats.TotalShared, 1);

	const FVoxelFoliageRandomGenerator RandomGenerator(Bounds, *Foliages[0]);
	const FVoxelFoliageProxy::FBiome BaseBiome = Foliages[0]->Biome;
	const FVoxelFoliageSpawnSettings& SpawnSettings = Foliages[0]->SpawnSettings;

	const auto FoliageCallback = [&](
		FVoxelFoliageProxyId FoliageId,
		TVoxelSharedRef<FVoxelFoliageResultData> FoliageData = MakeVoxelShared<FVoxelFoliageResultData>())
	{
		// Note: we create Transforms even if empty
		FoliageSubsystem->TaskCallbacks.Enqueue({ WeakBucket, FoliageId, TaskId, Bounds, FoliageData });
	};
	
	// Create ray handler, adding the meshes to the embree scene
	TUniquePtr<FVoxelFoliageRayHandler> RayHandler;
	if (SpawnSettings.SpawnType == EVoxelFoliageSpawnType::Ray)
	{
		RayHandler = FVoxelFoliageUtilities::CreateRayHandler(*FoliageSubsystem, SpawnSettings, CancelTracker, Bounds, Stats);

		if (!RayHandler)
		{
			// Empty chunk - make sure to fire all callbacks and then exit
			for (FVoxelFoliageProxy* Foliage : Foliages)
			{
				FoliageCallback(Foliage->UniqueId);
			}
			return;
		}
		
		RayHandler->bStoreDebugRays = FoliageDebug->ShouldStoreDebugRays();
	}
	
	CHECK_CANCELED();

	TMap<FVoxelFoliageProxyId, TVoxelSharedRef<FVoxelFoliageResultData>> FoliageDatas;
	{
		// Lock the data
		// Used by height spawning, so needs to be done now
		const FVoxelIntBox LockedBounds = Bounds.Extend(2); // For neighbors: +1; For max included vs excluded: +1
		FVoxelReadScopeLock Lock(*Data, LockedBounds, FUNCTION_FNAME);
		const FVoxelConstDataAccelerator Accelerator(*Data, LockedBounds);

		CHECK_CANCELED();

		FVoxelFoliageQueryCache DensityCache(Stats, Accelerator);

		TMap<FVoxelFoliageGroupId, TArray<FVoxelFoliageProxy*>> GroupToFoliages;
		for (FVoxelFoliageProxy* Foliage : Foliages)
		{
			GroupToFoliages.FindOrAdd(Foliage->Group.Id).Add(Foliage);
		}
		
		FVoxelFoliageGroupSpawner GroupSpawner
		{
			Stats,
			Bounds,
			CancelTracker,
			RayHandler.Get(),
			*Data,
			Accelerator,
			DensityCache,
			FoliageSubsystem->Settings,
			*FoliageDebug,
			BaseBiome,
			SpawnSettings,
			RandomGenerator,
			Foliages,
			GroupToFoliages
		};
		FoliageDatas = GroupSpawner.Spawn();
	}
	
	CHECK_CANCELED();

	for (FVoxelFoliageProxy* Foliage : Foliages)
	{
		if (const TVoxelSharedRef<FVoxelFoliageResultData>* FoliageData = FoliageDatas.Find(Foliage->UniqueId))
		{
			FoliageCallback(Foliage->UniqueId, *FoliageData);
		}
		else
		{
			FoliageCallback(Foliage->UniqueId);
		}
	}
}

#undef CHECK_CANCELED