// Copyright 2021 Phyronnaz

#include "VoxelFoliageBucket.h"
#include "VoxelFoliageProxy.h"
#include "VoxelFoliageSubsystem.h"
#include "VoxelFoliageTask.h"
#include "VoxelPool.h"

FVoxelFoliageBucket::FVoxelFoliageBucket(const FVoxelFoliageBucketKey& Key)
	: ChunkSize(Key.GetChunkSize())
	, Key(Key)
{
}

FVoxelFoliageBucket::~FVoxelFoliageBucket()
{
	for (auto& ChunkIt : Chunks)
	{
		for (auto& FoliageIt : ChunkIt.Value->Foliages)
		{
			FoliageIt.Value.CancelTask();
		}
	}

	// Make sure to delete the results before the proxies, to pass their destructor check
	Chunks.Reset();
	Foliages.Reset();
}

void FVoxelFoliageBucket::Spawn(const FVoxelIntBox& Bounds, FVoxelFoliageSubsystem& FoliageSubsystem, const FVoxelFoliageBiomeId* BiomeToSpawn)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	ensure(Foliages.Num() > 0);

	SpawnedBounds += Bounds;

	TVoxelSharedPtr<FVoxelFoliageChunk>& Chunk = Chunks.FindOrAdd(GetChunkKey(Bounds));
	// We shouldn't have any empty chunk around
	ensure(!Chunk || Chunk->Foliages.Num() > 0);

	if (!Chunk)
	{
		Chunk = MakeVoxelShared<FVoxelFoliageChunk>();
	}

	TArray<FVoxelFoliageProxyId> FoliagesToSpawn;
	
	for (auto& It : Foliages)
	{
		const FVoxelFoliageProxyId FoliageId = It.Key;
		const FVoxelFoliageProxy& Foliage = *It.Value;

		if (BiomeToSpawn && Foliage.Biome.Id != *BiomeToSpawn)
		{
			continue;
		}

		FVoxelFoliageChunk::FFoliageData& FoliageData = Chunk->Foliages.FindOrAdd(FoliageId);
		ensure(!FoliageData.TaskId.IsValid());
		
		if (!FoliageData.FoliageResult)
		{
			FoliagesToSpawn.Add(FoliageId);
		}
		else
		{
			FVoxelFoliageProxyResult& Result = *FoliageData.FoliageResult;
			ensure(&Result.Proxy == It.Value.Get());
			ensure(!Result.IsCreated() || Foliage.bDoNotDespawn);
			ensure(Result.NeedsToBeSaved() || Foliage.bDoNotDespawn);
			// If we cannot be despawned we weren't destroyed, and there is nothing to do
			if (!Foliage.bDoNotDespawn)
			{
				Result.Create(FoliageSubsystem);
			}
		}
	}

	if (FoliagesToSpawn.Num() == 0)
	{
		return;
	}

	// Sort by biome - groups are processed in a single task
	FoliagesToSpawn.Sort([&](FVoxelFoliageProxyId A, FVoxelFoliageProxyId B)
	{
		const FVoxelFoliageProxy& FoliageA = *Foliages[A];
		const FVoxelFoliageProxy& FoliageB = *Foliages[B];

		ensure(FoliageA.Biome.Id == FoliageB.Biome.Id || FoliageA.Group.Id != FoliageB.Group.Id);

		return FoliageA.Biome.Id < FoliageB.Biome.Id;
	});
	
	TArray<IVoxelQueuedWork*> Tasks;
	
	FVoxelFoliageBiomeId BiomeId;
	TArray<FVoxelFoliageProxyId> QueuedFoliages;

	const auto AddQueuedFoliages = [&]()
	{
		if (QueuedFoliages.Num() == 0)
		{
			return;
		}

		TArray<TVoxelWeakPtr<FVoxelFoliageProxy>> WeakFoliages;
		FVoxelCancelTrackerGroup CancelTracker;
		for (FVoxelFoliageProxyId QueuedFoliageId : QueuedFoliages)
		{
			WeakFoliages.Add(Foliages[QueuedFoliageId]);
			CancelTracker.AddTracker(Chunk->Foliages[QueuedFoliageId].GetCancelCounterForTask());
		}

		auto* Task = new FVoxelFoliageTask(Bounds, CancelTracker, AsShared(), FoliageSubsystem, WeakFoliages);
		Tasks.Add(Task);

		for (FVoxelFoliageProxyId QueuedFoliageId : QueuedFoliages)
		{
			Chunk->Foliages[QueuedFoliageId].TaskId = Task->TaskId;
		}

		QueuedFoliages.Reset();
	};
	
	for (const FVoxelFoliageProxyId FoliageId : FoliagesToSpawn)
	{
		const TVoxelSharedPtr<FVoxelFoliageProxy>& Foliage = Foliages[FoliageId];
		
		if (BiomeId != Foliage->Biome.Id)
		{
			AddQueuedFoliages();
		}

		BiomeId = Foliage->Biome.Id;
		QueuedFoliages.Add(FoliageId);
	}
	AddQueuedFoliages();
	
	FoliageSubsystem.GetSubsystemChecked<FVoxelPool>().QueueTasks(EVoxelTaskType::FoliageBuild, Tasks);
}

void FVoxelFoliageBucket::Despawn(const FVoxelIntBox& Bounds, const FVoxelFoliageSubsystem& FoliageSubsystem)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	
	const TVoxelSharedPtr<FVoxelFoliageChunk> Chunk = Chunks.FindRef(GetChunkKey(Bounds));
	if (!Chunk)
	{
		return;
	}
	
	for (auto It = Chunk->Foliages.CreateIterator(); It; ++It)
	{
		FVoxelFoliageChunk::FFoliageData& FoliageData = It.Value();

		// Cancel any pending task
		FoliageData.CancelTask();

		if (!FoliageData.FoliageResult)
		{
			continue;
		}
		FVoxelFoliageProxyResult& Result = *FoliageData.FoliageResult;
		
		if (Result.Proxy.bDoNotDespawn)
		{
			continue;
		}

		if (Result.IsCreated())
		{
			Result.Destroy(FoliageSubsystem);
		}

		if (Result.NeedsToBeSaved())
		{
			continue;
		}

		// Chunk can be removed
		It.RemoveCurrent();
	}

	// Remove chunk if empty
	if (Chunk->Foliages.Num() == 0)
	{
		Chunks.Remove(GetChunkKey(Bounds));
	}
}

void FVoxelFoliageBucket::FinishSpawning(
	const FVoxelIntBox& Bounds, 
	FVoxelFoliageSubsystem& FoliageSubsystem, 
	FVoxelFoliageProxyId FoliageId, 
	FVoxelFoliageTaskId TaskId,
	const TVoxelSharedPtr<FVoxelFoliageResultData>& Data)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!ensure(TaskId.IsValid()) ||
		!ensure(Data))
	{
		return;
	}
	
	const TVoxelSharedPtr<FVoxelFoliageProxy> Foliage = Foliages.FindRef(FoliageId);
	if (!ensure(Foliage))
	{
		return;
	}
	
	const TVoxelSharedPtr<FVoxelFoliageChunk> Chunk = Chunks.FindRef(GetChunkKey(Bounds));
	if (!ensure(Chunk) ||
		!ensure(Chunk->Foliages.Contains(FoliageId)))
	{
		return;
	}

	FVoxelFoliageChunk::FFoliageData& FoliageData = Chunk->Foliages.FindChecked(FoliageId);
	if (FoliageData.TaskId != TaskId)
	{
		// Canceled
		ensure(TaskId < FoliageData.TaskId || !FoliageData.TaskId.IsValid());
		return;
	}
	
	if (!ensure(!FoliageData.FoliageResult))
	{
		return;
	}

	FoliageData.TaskId = {};
	FoliageData.FoliageResult = MakeVoxelShared<FVoxelFoliageProxyResult>(*Foliage, Bounds, Data.ToSharedRef());
	FoliageData.FoliageResult->Create(FoliageSubsystem);
}
