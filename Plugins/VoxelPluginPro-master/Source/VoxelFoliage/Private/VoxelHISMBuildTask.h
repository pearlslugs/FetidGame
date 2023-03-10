// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelMinimal.h"
#include "VoxelAsyncWork.h"
#include "VoxelFoliageMatrix.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

class FVoxelInstancedMeshManager;
class UVoxelHierarchicalInstancedStaticMeshComponent;

DECLARE_UNIQUE_VOXEL_ID(FVoxelHISMBuildTaskId);

extern TAutoConsoleVariable<int32> CVarRandomColorPerHISM;

struct FVoxelHISMBuiltData
{
	FVoxelHISMBuildTaskId UniqueId;

	TArray<FVoxelFoliageMatrix> BuiltInstancesMatrices;
	
	TArray<int32> InstancesToBuiltInstances;
	TArray<int32> BuiltInstancesToInstances;
	
	TUniquePtr<FStaticMeshInstanceData> InstanceBuffer;
	TArray<FClusterNode> ClusterTree;
	int32 OcclusionLayerNum = 0;
};

// Will auto delete
class FVoxelHISMBuildTask : public FVoxelAsyncWork
{
	GENERATED_VOXEL_ASYNC_WORK_BODY(FVoxelHISMBuildTask)

public:
	const FVoxelHISMBuildTaskId UniqueId = FVoxelHISMBuildTaskId::New();
	const TVoxelSharedRef<FThreadSafeCounter> CancelCounter;

	const FBox MeshBox;
	const int32 DesiredInstancesPerLeaf;
	const TVoxelWeakPtr<FVoxelInstancedMeshManager> InstancedMeshManager;
	const TWeakObjectPtr<UVoxelHierarchicalInstancedStaticMeshComponent> Component;

	const TArray<float> CustomData; 
	
	// Output
	const TVoxelSharedRef<FVoxelHISMBuiltData> BuiltData;

	FVoxelHISMBuildTask(
		UVoxelHierarchicalInstancedStaticMeshComponent* Component,
		const TArray<FVoxelFoliageMatrix>& Matrices,
		const TArray<float>& CustomData);

	//~ Begin FVoxelAsyncWork Interface
	virtual void DoWork() override;
	//~ End FVoxelAsyncWork Interface
};