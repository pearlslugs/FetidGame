// Copyright 2021 Phyronnaz

#include "VoxelHISMBuildTask.h"
#include "VoxelHierarchicalInstancedStaticMeshComponent.h"
#include "VoxelInstancedMeshManager.h"
#include "VoxelUtilities/VoxelThreadingUtilities.h"

#include "Engine/StaticMesh.h"

DEFINE_UNIQUE_VOXEL_ID(FVoxelHISMBuildTaskId);

static TAutoConsoleVariable<int32> CVarLogHISMBuildTimes(
	TEXT("voxel.foliage.LogCullingTreeBuildTimes"),
	0,
	TEXT("If true, will log all the HISM build times"),
	ECVF_Default);

TAutoConsoleVariable<int32> CVarRandomColorPerHISM(
	TEXT("voxel.foliage.RandomColorPerHISM"),
	0,
	TEXT("If true, will set the instance random to be a hash of the HISM pointer"),
	ECVF_Default);

FVoxelHISMBuildTask::FVoxelHISMBuildTask(
	UVoxelHierarchicalInstancedStaticMeshComponent* Component,
	const TArray<FVoxelFoliageMatrix>& Matrices,
	const TArray<float>& CustomData)
	: FVoxelAsyncWork(STATIC_FNAME("HISM Build Task"), EVoxelTaskType::HISMBuild, EPriority::Null, true)
	, CancelCounter(MakeVoxelShared<FThreadSafeCounter>())
	, MeshBox(Component->GetStaticMesh()->GetBounds().GetBox())
	, DesiredInstancesPerLeaf(Component->DesiredInstancesPerLeaf())
	, InstancedMeshManager(Component->Voxel_InstancedMeshManager)
	, Component(Component)
	, CustomData(CustomData)
	, BuiltData(MakeVoxelShared<FVoxelHISMBuiltData>())
{
	check(Matrices.Num() > 0 && Matrices.Num() < 1e8); // If you have 100M instances something went terribly wrong
	check(CustomData.Num() % Matrices.Num() == 0);

	BuiltData->UniqueId = UniqueId;
	{
		VOXEL_SCOPE_COUNTER("Copy Matrices");
		BuiltData->BuiltInstancesMatrices = Matrices;
	}
}

void FVoxelHISMBuildTask::DoWork()
{
	const double StartTime = FPlatformTime::Seconds();
	const int32 NumInstances = BuiltData->BuiltInstancesMatrices.Num();
	check(NumInstances > 0);

	check(CustomData.Num() % NumInstances == 0);
	const int32 NumCustomDataChannels = CustomData.Num() / NumInstances;
	
	BuiltData->InstanceBuffer = MakeUnique<FStaticMeshInstanceData>(false);
	{
		VOXEL_ASYNC_SCOPE_COUNTER("AllocateInstances");
		BuiltData->InstanceBuffer->AllocateInstances(
			NumInstances,
			ONLY_UE_25_AND_HIGHER(NumCustomDataChannels,)
			EResizeBufferFlags::AllowSlackOnGrow | EResizeBufferFlags::AllowSlackOnReduce,
			true);
	}

	TArray<FMatrix> CleanTransforms;
	CleanTransforms.SetNumUninitialized(NumInstances);

	{
		const bool bRandomColorPerHISM = CVarRandomColorPerHISM.GetValueOnAnyThread() != 0;
		float RandomColor = 0.f;
		if (bRandomColorPerHISM)
		{
			const uint32 RandomInt = FVoxelUtilities::MurmurHash64(uint64(Component.Get()));
			RandomColor = *reinterpret_cast<const float*>(&RandomInt);
		}

		{
			VOXEL_ASYNC_SCOPE_COUNTER("SetInstances");
			for (int32 InstanceIndex = 0; InstanceIndex < NumInstances; InstanceIndex++)
			{
				const FVoxelFoliageMatrix& Matrix = BuiltData->BuiltInstancesMatrices[InstanceIndex];
				const FMatrix CleanMatrix = Matrix.GetCleanMatrix();

				const float RandomId = bRandomColorPerHISM ? RandomColor : Matrix.GetRandomInstanceId();

				CleanTransforms[InstanceIndex] = CleanMatrix;
				BuiltData->InstanceBuffer->SetInstance(InstanceIndex, CleanMatrix, RandomId);
			}
		}

#if VOXEL_ENGINE_VERSION  >= 425
		{
			VOXEL_ASYNC_SCOPE_COUNTER("Set Custom Data");
			for (int32 InstanceIndex = 0; InstanceIndex < NumInstances; InstanceIndex++)
			{
				for (int32 Channel = 0; Channel < NumCustomDataChannels; Channel++)
				{
					const float Data = CustomData[Channel + NumCustomDataChannels * InstanceIndex];
					BuiltData->InstanceBuffer->SetInstanceCustomData(InstanceIndex, Channel, Data);
				}
			}
		}
#endif
	}

	// Only check if we're canceled before the BuildTree, as it's the true expensive operation here
	// and if we've finished it, we might as well use the result
	if (CancelCounter->GetValue() > 0)
	{
		return;
	}
	
	TArray<int32> SortedInstances;
	TArray<int32> InstanceReorderTable;
	{
		VOXEL_ASYNC_SCOPE_COUNTER("BuildTreeAnyThread");
		static_assert(sizeof(FVoxelFoliageMatrix) == sizeof(FMatrix), "");
		TArray<float> CustomDataFloats;
		UHierarchicalInstancedStaticMeshComponent::BuildTreeAnyThread(
			CleanTransforms,
			// Done manually
			ONLY_UE_25_AND_HIGHER(CustomDataFloats,)
			ONLY_UE_25_AND_HIGHER(0,)
			MeshBox,
			BuiltData->ClusterTree,
			SortedInstances,
			InstanceReorderTable,
			BuiltData->OcclusionLayerNum,
			DesiredInstancesPerLeaf,
			false
		);
	}

	{
		VOXEL_ASYNC_SCOPE_COUNTER("Build Reorder Table");
		BuiltData->InstancesToBuiltInstances = InstanceReorderTable;
		BuiltData->BuiltInstancesToInstances.Init(-1, InstanceReorderTable.Num());
		for (int32 Index = 0; Index < InstanceReorderTable.Num(); Index++)
		{
			BuiltData->BuiltInstancesToInstances[InstanceReorderTable[Index]] = Index;
		}
		for (int32 Index : BuiltData->BuiltInstancesToInstances) check(Index != -1);
	}
	
	// in-place sort the instances
	{
		VOXEL_ASYNC_SCOPE_COUNTER("Sort Instances");
		for (int32 FirstUnfixedIndex = 0; FirstUnfixedIndex < NumInstances; FirstUnfixedIndex++)
		{
			const int32 LoadFrom = SortedInstances[FirstUnfixedIndex];
			if (LoadFrom != FirstUnfixedIndex)
			{
				check(LoadFrom > FirstUnfixedIndex);
				BuiltData->InstanceBuffer->SwapInstance(FirstUnfixedIndex, LoadFrom);
				// Also keep up to date the transforms array
				BuiltData->BuiltInstancesMatrices.Swap(FirstUnfixedIndex, LoadFrom);
				const int32 SwapGoesTo = InstanceReorderTable[FirstUnfixedIndex];
				check(SwapGoesTo > FirstUnfixedIndex);
				check(SortedInstances[SwapGoesTo] == FirstUnfixedIndex);
				SortedInstances[SwapGoesTo] = LoadFrom;
				InstanceReorderTable[LoadFrom] = SwapGoesTo;
				InstanceReorderTable[FirstUnfixedIndex] = FirstUnfixedIndex;
				SortedInstances[FirstUnfixedIndex] = FirstUnfixedIndex;
			}
		}
	}

	const double EndTime = FPlatformTime::Seconds();
	if (CVarLogHISMBuildTimes.GetValueOnAnyThread() != 0)
	{
		LOG_VOXEL(
			Log,
			TEXT("Building the HISM culling tree took %2.2fms; Instances: %d"),
			(EndTime - StartTime) * 1000,
			NumInstances);
	}
	
	auto PinnedInstancedMeshManager = InstancedMeshManager.Pin();
	if (PinnedInstancedMeshManager.IsValid())
	{
		PinnedInstancedMeshManager->HISMBuildTaskCallback(Component, BuiltData);
	}
	
#undef CHECK_CANCEL
}