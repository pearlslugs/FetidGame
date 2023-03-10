// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelMinimal.h"
#include "VoxelEnums.h"
#include "VoxelIntBox.h"
#include "Templates/SubclassOf.h"
#include "VoxelFoliageMatrix.h"
#include "VoxelInstancedMeshSettings.h"
#include "VoxelTickable.h"
#include "VoxelSubsystem.h"
#include "VoxelInstancedMeshManager.generated.h"

struct FVoxelInstancesSection;
struct FVoxelHISMBuiltData;
class AActor;
class AVoxelWorld;
class AVoxelFoliageActor;
class UStaticMesh;
class UVoxelHierarchicalInstancedStaticMeshComponent;
class FVoxelPool;
class FVoxelData;
class FVoxelEventManager;
struct FVoxelIntBox;

struct VOXELFOLIAGE_API FVoxelInstancedMeshManagerSettings
{
	const TWeakObjectPtr<AActor> ComponentsOwner;
	const TVoxelSharedRef<FIntVector> WorldOffset;
	const TVoxelSharedRef<FVoxelPool> Pool;
	const TVoxelSharedRef<FVoxelEventManager> EventManager;
	const uint32 HISMChunkSize;
	const uint32 CollisionDistanceInChunks;
	const float VoxelSize;
	const int64 MaxNumberOfInstances;
	const FSimpleDelegate OnMaxInstanceReached;

	FVoxelInstancedMeshManagerSettings(
		const AVoxelWorld* World,
		const TVoxelSharedRef<FVoxelPool>& Pool,
		const TVoxelSharedRef<FVoxelEventManager>& EventManager);
};

struct FVoxelInstancedMeshInstancesRef
{
public:
	FVoxelInstancedMeshInstancesRef() = default;

	bool IsValid() const
	{
		return Section.IsValid();
	}
	
private:
	TWeakObjectPtr<UVoxelHierarchicalInstancedStaticMeshComponent> HISM;
	TVoxelWeakPtr<FVoxelInstancesSection> Section;
	int32 NumInstances = 0;

	friend class FVoxelInstancedMeshManager;
};

UCLASS()
class VOXELFOLIAGE_API UVoxelInstancedMeshSubsystemProxy : public UVoxelStaticSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelInstancedMeshManager);
};

class VOXELFOLIAGE_API FVoxelInstancedMeshManager : public IVoxelSubsystem, public FVoxelTickable
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelInstancedMeshSubsystemProxy);

	//~ Begin IVoxelSubsystem Interface
	virtual void Create() override;
	virtual void Destroy() override;
	virtual EVoxelSubsystemFlags GetFlags() const override { return EVoxelSubsystemFlags::RecreateFoliage; }
	//~ End IVoxelSubsystem Interface

public:
	FVoxelInstancedMeshInstancesRef AddInstances(
		const FVoxelInstancedMeshWeakKey& Key, 
		const FVoxelFoliageTransforms& Transforms, 
		const TArray<float>& CustomData,
		const FVoxelIntBox& Bounds);
	void RemoveInstances(FVoxelInstancedMeshInstancesRef Ref);

	static const TArray<int32>& GetRemovedIndices(FVoxelInstancedMeshInstancesRef Ref);

	AVoxelFoliageActor* SpawnActor(
		const FVoxelInstancedMeshWeakKey& Key,
		const FVoxelFoliageTransform& Transform) const;
	void SpawnActors(
		const FVoxelInstancedMeshWeakKey& Key,
		const FVoxelFoliageTransforms& Transforms,
		TArray<AVoxelFoliageActor*>& OutActors) const;

	void SpawnActorsInArea(
		const FVoxelIntBox& Bounds,
		const FVoxelData& Data,
		EVoxelSpawnerActorSpawnType SpawnType,
		TArray<AVoxelFoliageActor*>& OutActors) const;
	
	TMap<FVoxelInstancedMeshWeakKey, TArray<FVoxelFoliageTransforms>> RemoveInstancesInArea(
		const FVoxelIntBox& Bounds,
		const FVoxelData& Data,
		EVoxelSpawnerActorSpawnType SpawnType) const;

	AVoxelFoliageActor* SpawnActorByIndex(UVoxelHierarchicalInstancedStaticMeshComponent* Component, int32 InstanceIndex);
	
	void RecomputeComponentPositions();

protected:
	//~ Begin FVoxelTickable Interface
	virtual void Tick(float DeltaTime) override;
	//~ End FVoxelTickable Interface

public:
	void HISMBuildTaskCallback(TWeakObjectPtr<UVoxelHierarchicalInstancedStaticMeshComponent> Component, const TVoxelSharedRef<FVoxelHISMBuiltData>& BuiltData);

private:
	struct FQueuedBuildCallback
	{
		TWeakObjectPtr<UVoxelHierarchicalInstancedStaticMeshComponent> Component;
		TVoxelSharedPtr<FVoxelHISMBuiltData> Data;
	};
	TQueue<FQueuedBuildCallback, EQueueMode::Mpsc> HISMBuiltDataQueue;

private:
	UVoxelHierarchicalInstancedStaticMeshComponent* CreateHISM(const FVoxelInstancedMeshWeakKey& Key, const FIntVector& Position) const;

private:
	struct FHISMChunk
	{
		FVoxelIntBoxWithValidity Bounds;
		TWeakObjectPtr<UVoxelHierarchicalInstancedStaticMeshComponent> HISM;
	};
	struct FHISMChunks
	{
		TMap<FIntVector, FHISMChunk> Chunks;
	};
	TMap<FVoxelInstancedMeshWeakKey, FHISMChunks> MeshSettingsToChunks;

	TSet<TWeakObjectPtr<UVoxelHierarchicalInstancedStaticMeshComponent>> HISMs;

	int64 NumInstances = 0;
	bool bMaxNumInstancesReachedFired = false;
	const uint32 CollisionChunkSize = 32; // Also change in AVoxelWorld::PostEditChangeProperty
};
