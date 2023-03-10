// Copyright 2021 Phyronnaz

#include "VoxelFoliageBlueprintLibrary.h"
#include "VoxelFoliageInterface.h"
#include "VoxelFoliageSubsystem.h"
#include "VoxelInstancedMeshManager.h"
#include "VoxelHierarchicalInstancedStaticMeshComponent.h"
#include "VoxelTools/VoxelToolHelpers.h"

void UVoxelFoliageBlueprintLibrary::SpawnVoxelSpawnerActorsInArea(
	TArray<AVoxelFoliageActor*>& OutActors, 
	AVoxelWorld* World,
	FVoxelIntBox Bounds, 
	EVoxelSpawnerActorSpawnType SpawnType)
{
	VOXEL_FUNCTION_COUNTER();
	CHECK_VOXELWORLD_IS_CREATED_VOID();
	CHECK_BOUNDS_ARE_VALID_VOID();

	OutActors.Reset();
	
	World->GetSubsystemChecked<FVoxelInstancedMeshManager>().SpawnActorsInArea(Bounds, World->GetSubsystemChecked<FVoxelData>(), SpawnType, OutActors);
}

AVoxelFoliageActor* UVoxelFoliageBlueprintLibrary::SpawnVoxelSpawnerActorByInstanceIndex(
	AVoxelWorld* World, 
	UVoxelHierarchicalInstancedStaticMeshComponent* Component, 
	int32 InstanceIndex)
{
	VOXEL_FUNCTION_COUNTER();
	CHECK_VOXELWORLD_IS_CREATED();

	if (!Component)
	{
		FVoxelMessages::Error(FUNCTION_ERROR("Invalid Component"));
		return nullptr;
	}
	if (Component->GetOwner() != World)
	{
		FVoxelMessages::Error(FUNCTION_ERROR("Component is not a component of World"));
		return nullptr;
	}

	return World->GetSubsystemChecked<FVoxelInstancedMeshManager>().SpawnActorByIndex(Component, InstanceIndex);
}

void UVoxelFoliageBlueprintLibrary::AddInstances(
	AVoxelWorld* World,
	const TArray<FTransform>& Transforms,
	const TArray<float>& CustomData,
	FVoxelInstancedMeshKey MeshKey,
	FVector FloatingDetectionOffset)
{
	VOXEL_FUNCTION_COUNTER();
	CHECK_VOXELWORLD_IS_CREATED_VOID();
	
	if (!MeshKey.Mesh)
	{
		FVoxelMessages::Error(FUNCTION_ERROR("Mesh is null!"));
		return;
	}

	if (CustomData.Num() != MeshKey.NumCustomDataChannels * Transforms.Num())
	{
		FVoxelMessages::Error(FUNCTION_ERROR("CustomData.Num() should be equal to MeshKey.NumCustomDataChannels * Transforms.Num()!"));
		return;
	}

	if (Transforms.Num() == 0)
	{
		return;
	}

	auto& InstancedMeshManager = World->GetSubsystemChecked<FVoxelInstancedMeshManager>();

	FVoxelIntBoxWithValidity BoundsWithValidity;
	for (auto& Transform : Transforms)
	{
		BoundsWithValidity += World->GlobalToLocal(Transform.GetTranslation());
	}
	const FVoxelIntBox Bounds = BoundsWithValidity.GetBox();
	
	FVoxelFoliageTransforms VoxelSpawnerTransforms;
	VoxelSpawnerTransforms.TransformsOffset = InstancedMeshManager.Settings.ComputeFoliageTransformsOffset(Bounds);
	VoxelSpawnerTransforms.Matrices.Reserve(Transforms.Num());
	
	for (int32 Index = 0; Index < Transforms.Num(); Index++)
	{
		FTransform Transform = Transforms[Index];
		Transform.AddToTranslation(-World->VoxelSize * FVector(VoxelSpawnerTransforms.TransformsOffset));
		
		FVoxelFoliageMatrix Matrix(Transform.ToMatrixWithScale());
		Matrix.SetPositionOffset(-FloatingDetectionOffset);
		VoxelSpawnerTransforms.Matrices.Add(Matrix);
	}

	InstancedMeshManager.AddInstances(MeshKey, VoxelSpawnerTransforms, CustomData, Bounds);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFoliageSave UVoxelFoliageBlueprintLibrary::GetSpawnersSave(AVoxelWorld* World)
{
	VOXEL_FUNCTION_COUNTER();
	CHECK_VOXELWORLD_IS_CREATED();

	FVoxelFoliageSave Save;
	World->GetSubsystemChecked<FVoxelFoliageSubsystem>().SaveTo(Save.NewMutable());
	return Save;
}

void UVoxelFoliageBlueprintLibrary::LoadFromSpawnersSave(AVoxelWorld* World, const FVoxelFoliageSave& Save)
{
	VOXEL_FUNCTION_COUNTER();
	CHECK_VOXELWORLD_IS_CREATED_VOID();

	World->GetSubsystemChecked<FVoxelFoliageSubsystem>().LoadFrom(Save.Const());
}
