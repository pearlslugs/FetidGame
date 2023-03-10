// Copyright 2021 Phyronnaz

#include "VoxelInstancedMeshManager.h"
#include "VoxelHierarchicalInstancedStaticMeshComponent.h"
#include "VoxelFoliageActor.h"
#include "VoxelEvents/VoxelEventManager.h"
#include "VoxelData/VoxelDataIncludes.h"
#include "VoxelMessages.h"
#include "VoxelWorld.h"
#include "VoxelPool.h"
#include "VoxelUtilities/VoxelThreadingUtilities.h"

#include "GameFramework/Actor.h"
#include "Engine/StaticMesh.h"

DECLARE_DWORD_COUNTER_STAT(TEXT("InstancedMeshManager Num Spawned Actors"), STAT_FVoxelInstancedMeshManager_NumSpawnedActors, STATGROUP_VoxelCounters);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Num HISM"), STAT_FVoxelInstancedMeshManager_NumHISM, STATGROUP_VoxelCounters);

DEFINE_VOXEL_SUBSYSTEM_PROXY(UVoxelInstancedMeshSubsystemProxy);

void FVoxelInstancedMeshManager::Create()
{
	Super::Create();

	RuntimeData->OnRecomputeComponentPositions.AddThreadSafeSP(this, &FVoxelInstancedMeshManager::RecomputeComponentPositions);
}

void FVoxelInstancedMeshManager::Destroy()
{
	Super::Destroy();
	
	StopTicking();

	// Needed for RegenerateFoliage
	for (const auto& HISM : HISMs)
	{
		if (HISM.IsValid())
		{
			HISM->DestroyComponent();
		}
	}
	DEC_DWORD_STAT_BY(STAT_FVoxelInstancedMeshManager_NumHISM, HISMs.Num());
	HISMs.Empty();
	
	MeshSettingsToChunks.Empty();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelInstancedMeshInstancesRef FVoxelInstancedMeshManager::AddInstances(
	const FVoxelInstancedMeshWeakKey& Key, 
	const FVoxelFoliageTransforms& Transforms, 
	const TArray<float>& CustomData,
	const FVoxelIntBox& Bounds)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!ensure(CustomData.Num() == Transforms.Matrices.Num() * Key.NumCustomDataChannels) ||
		!ensure(Transforms.Matrices.Num() > 0)) 
	{
		return {};
	}
	
	if (NumInstances > Settings.MaxNumberOfFoliageInstances)
	{
		if (!bMaxNumInstancesReachedFired)
		{
			bMaxNumInstancesReachedFired = true;
			if (RuntimeData->OnMaxFoliageInstancesReached.IsBound())
			{
				RuntimeData->OnMaxFoliageInstancesReached.Broadcast();
			}
			else
			{
				FVoxelMessages::Error(FString::Printf(TEXT("Max number of foliage instances reached: %lli"), Settings.MaxNumberOfFoliageInstances), Settings.Owner.Get());
			}
		}
		return {};
	}
	
	NumInstances += Transforms.Matrices.Num();

	const FIntVector Position = Settings.ComputeFoliageTransformsOffset(Bounds);
	ensure(Transforms.TransformsOffset == Position);
	
	FHISMChunks& Chunks = MeshSettingsToChunks.FindOrAdd(Key);
	const FIntVector ChunkKey = FVoxelUtilities::DivideFloor(Position, Settings.HISMChunkSize);

	FHISMChunk& Chunk = Chunks.Chunks.FindOrAdd(ChunkKey);
	Chunk.Bounds += Bounds;
	
	auto& HISM = Chunk.HISM;
	if (!HISM.IsValid())
	{
		HISM = CreateHISM(Key, Position);
		HISMs.Add(HISM);
		INC_DWORD_STAT(STAT_FVoxelInstancedMeshManager_NumHISM);
	}

	if (!ensure(HISM.IsValid()))
	{
		// Happens sometimes
		return {};
	}

	FVoxelInstancedMeshInstancesRef Ref;
	Ref.HISM = HISM;
	Ref.Section = HISM->Voxel_AppendTransforms(Transforms.Matrices, CustomData, Bounds);
	if (ensure(Ref.IsValid()))
	{
		Ref.NumInstances = Transforms.Matrices.Num();
	}
	
	return Ref;
}

void FVoxelInstancedMeshManager::RemoveInstances(FVoxelInstancedMeshInstancesRef Ref)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	NumInstances -= Ref.NumInstances;
	ensure(NumInstances >= 0);

	if (!ensure(Ref.IsValid())) return;
	if (!/*ensure*/(Ref.HISM.IsValid())) return; // Can be raised due to DeleteTickable being delayed

	Ref.HISM->Voxel_RemoveSection(Ref.Section);

	if (Ref.HISM->Voxel_IsEmpty())
	{
		ensure(HISMs.Remove(Ref.HISM) == 1);
		DEC_DWORD_STAT(STAT_FVoxelInstancedMeshManager_NumHISM);
		
		LOG_VOXEL(Verbose, TEXT("Instanced Mesh Manager: Removing HISM for mesh %s"), Ref.HISM->GetStaticMesh() ? *Ref.HISM->GetStaticMesh()->GetPathName() : TEXT("NULL"));
		
		// TODO pooling?
		Ref.HISM->DestroyComponent();
	}
}

const TArray<int32>& FVoxelInstancedMeshManager::GetRemovedIndices(FVoxelInstancedMeshInstancesRef Ref)
{
	const auto Pinned = Ref.Section.Pin();
	if (ensure(Pinned.IsValid()))
	{
		// Fine to return a ref, as Section is only used on the game thread so it won't be deleted
		return Pinned->RemovedIndices;
	}
	else
	{
		static const TArray<int32> EmptyArray;
		return EmptyArray;
	}
}

AVoxelFoliageActor* FVoxelInstancedMeshManager::SpawnActor(
	const FVoxelInstancedMeshWeakKey& Key,
	const FVoxelFoliageTransform& Transform) const
{
	VOXEL_FUNCTION_COUNTER();
	INC_DWORD_STAT(STAT_FVoxelInstancedMeshManager_NumSpawnedActors);
	
	if (!Key.ActorClass) return nullptr;

	const AVoxelWorld* VoxelWorld = Settings.VoxelWorld.Get();
	UWorld* World = Settings.World.Get();
	if (!ensureVoxelSlow(VoxelWorld) ||
		!ensureVoxelSlow(World) ||
		!World->IsGameWorld())
	{
		return nullptr;
	}

	FVoxelTransform LocalTransform = FTransform(Transform.Matrix.GetCleanMatrix());
	LocalTransform.SetTranslation(LocalTransform.GetTranslation() + FVoxelVector(Transform.TransformOffset) * Settings.VoxelSize);
	
	const FVoxelTransform GlobalTransform = LocalTransform * VoxelWorld->GetVoxelTransform();
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.bDeferConstruction = true;

	AVoxelFoliageActor* Actor = World->SpawnActor<AVoxelFoliageActor>(Key.ActorClass, SpawnParameters);
	if (!Actor) return nullptr;

	UStaticMeshComponent* StaticMeshComponent = Actor->StaticMeshComponent;
	if (Actor->bAutomaticallySetMesh && ensure(StaticMeshComponent))
	{
		StaticMeshComponent->SetStaticMesh(Key.Mesh.Get());

		for (int32 Index =0; Index < Key.Materials.Num(); Index++)
		{
			StaticMeshComponent->SetMaterial(Index, Key.Materials[Index].Get());
		}
	}

	Actor->FinishSpawning(GlobalTransform);

	if (StaticMeshComponent)
	{
		StaticMeshComponent->RecreatePhysicsState();
	}

	return Actor;
}

void FVoxelInstancedMeshManager::SpawnActors(
	const FVoxelInstancedMeshWeakKey& Key,
	const FVoxelFoliageTransforms& Transforms,
	TArray<AVoxelFoliageActor*>& OutActors) const
{
	VOXEL_FUNCTION_COUNTER();
	
	for (auto& Matrix : Transforms.Matrices)
	{
		auto* Actor = SpawnActor(Key, { Transforms.TransformsOffset, Matrix });
		if (!Actor) return;
		OutActors.Emplace(Actor);
	}
}

void FVoxelInstancedMeshManager::SpawnActorsInArea(
	const FVoxelIntBox& Bounds,
	const FVoxelData& Data,
	EVoxelSpawnerActorSpawnType SpawnType,
	TArray<AVoxelFoliageActor*>& OutActors) const
{
	VOXEL_FUNCTION_COUNTER();

	const auto TransformsMap = RemoveInstancesInArea(Bounds, Data, SpawnType);

	for (auto& It : TransformsMap)
	{
		for (const auto& Transforms : It.Value)
		{
			SpawnActors(It.Key, Transforms, OutActors);
		}
	}
}

TMap<FVoxelInstancedMeshWeakKey, TArray<FVoxelFoliageTransforms>> FVoxelInstancedMeshManager::RemoveInstancesInArea(
	const FVoxelIntBox& Bounds, 
	const FVoxelData& Data, 
	EVoxelSpawnerActorSpawnType SpawnType) const
{
	VOXEL_FUNCTION_COUNTER();

	const auto ExtendedBounds = Bounds.Extend(1); // As we are accessing floats, they can be between Max - 1 and Max

	TMap<FVoxelInstancedMeshWeakKey, TArray<FVoxelFoliageTransforms>> TransformsMap;
	
	FVoxelReadScopeLock Lock(Data, ExtendedBounds, "SpawnActorsInArea");
	
	const TUniquePtr<FVoxelConstDataAccelerator> Accelerator =
		SpawnType == EVoxelSpawnerActorSpawnType::All
		? nullptr
		: MakeUnique<FVoxelConstDataAccelerator>(Data, ExtendedBounds);
	
	for (auto& MeshSettingsIt : MeshSettingsToChunks)
	{
		TArray<FVoxelFoliageTransforms>& TransformsArray = TransformsMap.FindOrAdd(MeshSettingsIt.Key);
		for (auto& ChunkIt : MeshSettingsIt.Value.Chunks)
		{
			const FHISMChunk& Chunk = ChunkIt.Value;
			if (Chunk.HISM.IsValid() && Chunk.Bounds.IsValid() && Chunk.Bounds.GetBox().Intersect(Bounds))
			{
				auto Transforms = Chunk.HISM->Voxel_RemoveInstancesInArea(Bounds, Accelerator.Get(), SpawnType);
				TransformsArray.Emplace(MoveTemp(Transforms));
			}
		}
	}

	return TransformsMap;
}

AVoxelFoliageActor* FVoxelInstancedMeshManager::SpawnActorByIndex(UVoxelHierarchicalInstancedStaticMeshComponent* Component, int32 InstanceIndex)
{
	VOXEL_FUNCTION_COUNTER();
	
	if (!ensure(Component)) 
	{
		return nullptr;
	}

	// Not one of ours
	if (!ensure(HISMs.Contains(Component)))
	{
		return nullptr;
	}
	
	FVoxelFoliageTransform Transform;
	if (!Component->Voxel_RemoveInstanceByIndex(InstanceIndex, Transform))
	{
		return nullptr;
	}

	ensure(Component->Voxel_GetMeshAndActorSettings().Mesh == Component->GetStaticMesh());

	return SpawnActor(Component->Voxel_GetMeshAndActorSettings(), Transform);
}

void FVoxelInstancedMeshManager::RecomputeComponentPositions()
{
	VOXEL_FUNCTION_COUNTER();

	for (const TWeakObjectPtr<UVoxelHierarchicalInstancedStaticMeshComponent>& HISM : HISMs)
	{
		UVoxelHierarchicalInstancedStaticMeshComponent* Component = HISM.Get();
		if (ensure(Component))
		{
			Settings.SetComponentPosition(*Component, Component->Voxel_GetVoxelPosition(), false);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelInstancedMeshManager::Tick(float DeltaTime)
{
	VOXEL_FUNCTION_COUNTER();
	
	FQueuedBuildCallback Callback;
	while (HISMBuiltDataQueue.Dequeue(Callback))
	{
		auto* HISM = Callback.Component.Get();
		if (!HISM) continue;

		HISM->Voxel_FinishBuilding(*Callback.Data);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelInstancedMeshManager::HISMBuildTaskCallback(
	TWeakObjectPtr<UVoxelHierarchicalInstancedStaticMeshComponent> Component, 
	const TVoxelSharedRef<FVoxelHISMBuiltData>& BuiltData)
{
	HISMBuiltDataQueue.Enqueue({ Component, BuiltData });
}

UVoxelHierarchicalInstancedStaticMeshComponent* FVoxelInstancedMeshManager::CreateHISM(const FVoxelInstancedMeshWeakKey& Key, const FIntVector& Position) const
{
	VOXEL_FUNCTION_COUNTER();

	LOG_VOXEL(Verbose, TEXT("Instanced Mesh Manager: Creating a new HISM for mesh %s"), *Key.Mesh->GetPathName());

	AActor* ComponentsOwner = Settings.ComponentsOwner.Get();
	if (!ensureVoxelSlow(ComponentsOwner))
	{
		return nullptr;
	}

	const FVoxelInstancedMeshSettings& MeshSettings = Key.InstanceSettings;

	UVoxelHierarchicalInstancedStaticMeshComponent* HISM;
	if (MeshSettings.HISMTemplate)
	{
		HISM = NewObject<UVoxelHierarchicalInstancedStaticMeshComponent>(ComponentsOwner, MeshSettings.HISMTemplate.Get(), NAME_None, RF_Transient);
	}
	else
	{
		HISM = NewObject<UVoxelHierarchicalInstancedStaticMeshComponent>(ComponentsOwner, NAME_None, RF_Transient);
	}

	HISM->Voxel_Init(
		GetSubsystemChecked<FVoxelPool>().AsShared(),
		SharedThis(const_cast<FVoxelInstancedMeshManager*>(this)),
		Settings.VoxelSize,
		Position,
		Key);	
	Settings.SetupComponent(*HISM);
	HISM->RegisterComponent();

	Settings.SetComponentPosition(*HISM, Position, false);
	
	if (MeshSettings.BodyInstance.GetCollisionEnabled() != ECollisionEnabled::NoCollision)
	{
		GetSubsystemChecked<FVoxelEventManager>().BindEvent(
			true,
			CollisionChunkSize,
			Settings.FoliageCollisionDistanceInVoxel / int32(CollisionChunkSize),
			FChunkDelegate::CreateUObject(HISM, &UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_EnablePhysics),
			FChunkDelegate::CreateUObject(HISM, &UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_DisablePhysics));
	}

	return HISM;
}