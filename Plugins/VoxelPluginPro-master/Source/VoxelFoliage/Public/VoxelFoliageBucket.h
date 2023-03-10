// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelIntBox.h"
#include "VoxelCancelCounter.h"

enum class EVoxelFoliageSpawnType;
class FVoxelFoliageSubsystem;
class FVoxelFoliageProxy;
class FVoxelFoliageProxyResult;
struct FVoxelFoliageResultData;

DECLARE_UNIQUE_VOXEL_ID(FVoxelFoliageTaskId);
DECLARE_UNIQUE_VOXEL_ID(FVoxelFoliageBiomeId);
DECLARE_UNIQUE_VOXEL_ID(FVoxelFoliageProxyId);

struct FVoxelFoliageBucketKey
{
	int32 LOD = 0;
	int32 GenerationDistanceInChunks = 0;
	bool bInfiniteGenerationDistance = false;
	EVoxelFoliageSpawnType SpawnType = {};

	int32 GetChunkSize() const
	{
		return FOLIAGE_CHUNK_SIZE << LOD;
	}

	friend bool operator==(const FVoxelFoliageBucketKey& Lhs, const FVoxelFoliageBucketKey& Rhs)
	{
		return
			Lhs.LOD == Rhs.LOD &&
			Lhs.GenerationDistanceInChunks == Rhs.GenerationDistanceInChunks &&
			Lhs.bInfiniteGenerationDistance == Rhs.bInfiniteGenerationDistance &&
			Lhs.SpawnType == Rhs.SpawnType;
	}
	friend uint32 GetTypeHash(const FVoxelFoliageBucketKey& Key)
	{
		return HashCombine(HashCombine(HashCombine(
			GetTypeHash(Key.LOD),
			GetTypeHash(Key.GenerationDistanceInChunks)),
			GetTypeHash(Key.bInfiniteGenerationDistance)),
			GetTypeHash(Key.SpawnType));
	}
	friend bool operator<(const FVoxelFoliageBucketKey& Lhs, const FVoxelFoliageBucketKey& Rhs)
	{
		// Just to be deterministic
		return
			Lhs.LOD < Rhs.LOD&&
			Lhs.GenerationDistanceInChunks < Rhs.GenerationDistanceInChunks&&
			int32(Lhs.bInfiniteGenerationDistance) < int32(Rhs.bInfiniteGenerationDistance) &&
			int32(Lhs.SpawnType) < int32(Rhs.SpawnType);
	}
};

class FVoxelFoliageChunk
{
public:
	class FFoliageData
	{
	public:
		FVoxelFoliageTaskId TaskId;
		TVoxelSharedPtr<FVoxelFoliageProxyResult> FoliageResult;

		void CancelTask()
		{
			TaskId = {};
			CancelCounter.Cancel();
		}
		FVoxelCancelCounter GetCancelCounterForTask() const
		{
			return CancelCounter;
		}

	private:
		FVoxelCancelCounter CancelCounter;
	};
	TMap<FVoxelFoliageProxyId, FFoliageData> Foliages;
};

class FVoxelFoliageBucket : public TVoxelSharedFromThis<FVoxelFoliageBucket>
{
public:
	const int32 ChunkSize;
	const FVoxelFoliageBucketKey Key;
	// Used to reduce the search area when regenerating foliage
	FVoxelIntBoxWithValidity SpawnedBounds;
	
	TMap<FVoxelFoliageProxyId, TVoxelSharedPtr<FVoxelFoliageProxy>> Foliages;
	TMap<FIntVector, TVoxelSharedPtr<FVoxelFoliageChunk>> Chunks;

	explicit FVoxelFoliageBucket(const FVoxelFoliageBucketKey& Key);
	~FVoxelFoliageBucket();
	
	FIntVector GetChunkKey(const FVoxelIntBox& Bounds) const
	{
		ensure(Bounds.Size() == FIntVector(ChunkSize));
		return Bounds.Min;
	}

	void Spawn(const FVoxelIntBox& Bounds, FVoxelFoliageSubsystem& FoliageSubsystem, const FVoxelFoliageBiomeId* BiomeToSpawn = nullptr);
	void Despawn(const FVoxelIntBox& Bounds, const FVoxelFoliageSubsystem& FoliageSubsystem);
	
	void FinishSpawning(
		const FVoxelIntBox& Bounds,
		FVoxelFoliageSubsystem& FoliageSubsystem,
		FVoxelFoliageProxyId FoliageId,
		FVoxelFoliageTaskId TaskId,
		const TVoxelSharedPtr<FVoxelFoliageResultData>& Data);
};