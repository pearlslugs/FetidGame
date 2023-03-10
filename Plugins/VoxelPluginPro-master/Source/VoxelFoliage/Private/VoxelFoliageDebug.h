// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelTickable.h"
#include "VoxelSubsystem.h"
#include "VoxelFoliageMatrix.h"
#include "VoxelFoliageRayHandler.h"
#include "HitGenerators/VoxelHitGenerator.h"
#include "VoxelFoliageDebug.generated.h"

class FVoxelFoliageProxy;

UCLASS()
class VOXELFOLIAGE_API UVoxelFoliageDebugSubsystemProxy : public UVoxelStaticSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelFoliageDebug);
};

class VOXELFOLIAGE_API FVoxelFoliageDebug : public IVoxelSubsystem, public FVoxelTickable
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelFoliageDebugSubsystemProxy);

	void PrintFoliageStats();

public:
	bool ShouldStoreDebugRays() const;
	void ReportChunk_AnyThread(
		const FVoxelFoliageProxy& Proxy, 
		const FVoxelIntBox& Bounds,
		const TArray<FVoxelFoliageDebugRay>& Rays,
		const TArray<FVoxelFoliageHit>& Hits,
		const TVoxelSharedPtr<FVoxelFoliageTransforms>& Transforms);

protected:
	//~ Begin IVoxelSubsystem Interface
	virtual void Destroy() override;
	//~ End IVoxelSubsystem Interface

	//~ Begin FVoxelTickable Interface
	virtual void Tick(float DeltaTime) override;
	//~ End FVoxelTickable Interface

private:
	struct FDebugChunk
	{
		TVoxelWeakPtr<const FVoxelFoliageProxy> Proxy;
		FVoxelIntBox Bounds;
		TArray<FVoxelFoliageDebugRay> Rays;
		TArray<FVoxelFoliageHit> Hits;
		FVoxelFoliageTransforms Transforms;
	};
	TMap<TVoxelWeakPtr<const FVoxelFoliageProxy>, TMap<FVoxelIntBox, TVoxelSharedPtr<FDebugChunk>>> Chunks;
	TQueue<TVoxelSharedPtr<FDebugChunk>, EQueueMode::Mpsc> QueuedChunks;

	bool EnableDebug() const;
};