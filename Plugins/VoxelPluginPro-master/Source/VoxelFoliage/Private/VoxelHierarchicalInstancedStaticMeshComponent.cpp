// Copyright 2021 Phyronnaz

#include "VoxelHierarchicalInstancedStaticMeshComponent.h"
#include "VoxelInstancedMeshManager.h"
#include "VoxelHISMBuildTask.h"
#include "VoxelDebug/VoxelDebugUtilities.h"
#include "VoxelData/VoxelDataIncludes.h"
#include "VoxelMinimal.h"
#include "VoxelPool.h"
#include "VoxelWorld.h"

#include "Async/Async.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

#if VOXEL_ENGINE_VERSION < 426
#include "Engine/Private/InstancedStaticMesh.h"
#else
#include "Engine/InstancedStaticMesh.h"
#endif

DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Voxel HISM Num Instances"), STAT_VoxelHISMComponent_NumInstances, STATGROUP_VoxelCounters);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Voxel HISM Num Physics Bodies"), STAT_VoxelHISMComponent_NumPhysicsBodies, STATGROUP_VoxelCounters);
DECLARE_DWORD_COUNTER_STAT(TEXT("Voxel HISM Num Floating Mesh Checked"), STAT_VoxelHISMComponent_NumFloatingMeshChecked, STATGROUP_VoxelCounters);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelHISMMemory);

static TAutoConsoleVariable<int32> CVarShowHISMCollisions(
	TEXT("voxel.foliage.ShowCollisions"),
	0,
	TEXT("If true, will show a debug point on HISM instances with collisions"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarBVHDebugDepth(
	TEXT("voxel.foliage.BVHDebugLevel"),
	0,
	TEXT("If above or equal to 1, debug BVH nodes at that depth, 1 being the root"),
	ECVF_Default);

static const FMatrix EmptyMatrix = FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector).ToMatrixWithScale();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelHierarchicalInstancedStaticMeshComponent::UVoxelHierarchicalInstancedStaticMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if !UE_BUILD_SHIPPING
	PrimaryComponentTick.bCanEverTick = true;
#endif

	Voxel_UpdateAllocatedMemory();
}

UVoxelHierarchicalInstancedStaticMeshComponent::~UVoxelHierarchicalInstancedStaticMeshComponent()
{
	DEC_VOXEL_MEMORY_STAT_BY(STAT_VoxelHISMMemory, Voxel_AllocatedMemory);
	DEC_DWORD_STAT_BY(STAT_VoxelHISMComponent_NumInstances, Voxel_UnbuiltMatrices.Num());
	ensure(Voxel_InstanceBodies.Num() == 0);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_Init(
	TVoxelWeakPtr<FVoxelPool> Pool, 
	TVoxelWeakPtr<FVoxelInstancedMeshManager> InstancedMeshManager, 
	float VoxelSize,
	const FIntVector& VoxelPosition,
	const FVoxelInstancedMeshKey& Key)
{
	Voxel_Pool = Pool;
	Voxel_InstancedMeshManager = InstancedMeshManager;
	Voxel_VoxelSize = VoxelSize;
	Voxel_VoxelPosition = VoxelPosition;
	Voxel_MeshKey = Key;
	
	auto& MeshSettings = Key.InstanceSettings;
	
	Voxel_BuildDelay = MeshSettings.BuildDelay;
	bDisableCollision = true;

#define EQ(X) X = MeshSettings.X;
	EQ(bAffectDynamicIndirectLighting)
	EQ(bAffectDistanceFieldLighting)
	EQ(bCastShadowAsTwoSided)
	EQ(bReceivesDecals)
	EQ(bUseAsOccluder)
	EQ(BodyInstance)
	EQ(LightingChannels)
	EQ(bRenderCustomDepth)
	EQ(CustomDepthStencilValue)
#undef EQ
	InstanceStartCullDistance = MeshSettings.CullDistance.Min;
	InstanceEndCullDistance = MeshSettings.CullDistance.Max;
	CastShadow = MeshSettings.bCastShadow;
	bCastDynamicShadow = MeshSettings.bCastShadow;
	BodyInstance = MeshSettings.BodyInstance;
	SetCustomNavigableGeometry(MeshSettings.CustomNavigableGeometry);	

	SetStaticMesh(Key.Mesh);

	for (int32 MaterialIndex = 0; MaterialIndex < Key.Materials.Num(); MaterialIndex++)
	{
		SetMaterial(MaterialIndex, Key.Materials[MaterialIndex]);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelWeakPtr<FVoxelInstancesSection> UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_AppendTransforms(
	const TArray<FVoxelFoliageMatrix>& InTransforms, 
	const TArray<float>& CustomData,
	const FVoxelIntBox& InBounds)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(GetStaticMesh()) ||
		!ensure(InTransforms.Num() > 0) ||
		!ensure(CustomData.Num() == Voxel_MeshKey.NumCustomDataChannels * InTransforms.Num()))
	{
		return nullptr;
	}
	
	INC_DWORD_STAT_BY(STAT_VoxelHISMComponent_NumInstances, InTransforms.Num());

	const int32 FirstInstanceIndex = Voxel_UnbuiltMatrices.Num();

	const auto NewSection = MakeVoxelShared<FVoxelInstancesSection>();
	NewSection->StartIndex = FirstInstanceIndex;
	NewSection->Num = InTransforms.Num();
	Voxel_Sections.Emplace(NewSection);
	
	{
		VOXEL_SCOPE_COUNTER("Append");
		Voxel_UnbuiltMatrices.Append(InTransforms);
		Voxel_CustomData.Append(CustomData);
	}
	
	Voxel_PendingNewInstancesBounds.Add(InBounds);

	Voxel_ScheduleBuildTree();
	Voxel_UpdateAllocatedMemory();

	return NewSection;
}

void UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_RemoveSection(const TVoxelWeakPtr<FVoxelInstancesSection>& Section)
{
	VOXEL_FUNCTION_COUNTER();

	const auto PinnedSection = Section.Pin();
	if (!ensure(PinnedSection.IsValid())) return;

	const int32 StartIndex = PinnedSection->StartIndex;
	const int32 Num = PinnedSection->Num;
	
	if (!ensure(Voxel_UnbuiltMatrices.IsValidIndex(StartIndex)) ||
		!ensure(StartIndex + Num <= Voxel_UnbuiltMatrices.Num())) return;

	if (!ensure(Voxel_Sections.Remove(PinnedSection) == 1)) return;

	ensure(PinnedSection.IsUnique());
	
	const auto Fix = [&](int32& Index)
	{
		ensure(Voxel_UnbuiltMatrices.IsValidIndex(Index));
		if (Index >= StartIndex)
		{
			if (Index < StartIndex + Num)
			{
				Index = -1;
			}
			else
			{
				Index -= Num;
			}
		}
	};

	{
		VOXEL_SCOPE_COUNTER("Fix Existing Sections");
		for (auto& ExistingSection : Voxel_Sections)
		{
			Fix(ExistingSection->StartIndex);
		}
	}
	
	{
		VOXEL_SCOPE_COUNTER("Fix UnbuiltInstancesToClear");
		for (auto& Index : Voxel_UnbuiltInstancesToClear)
		{
			Fix(Index);
		}
		Voxel_UnbuiltInstancesToClear.RemoveSwap(-1);
	}

	{
		VOXEL_SCOPE_COUNTER("Fix bodies");
		for (auto& It : Voxel_InstanceBodies)
		{
			for (FBodyWithTransform& Body : It.Value)
			{
				Fix(Body.BodyInstance->InstanceBodyIndex);
				
				if (Body.BodyInstance->InstanceBodyIndex == -1)
				{
					DEC_DWORD_STAT(STAT_VoxelHISMComponent_NumPhysicsBodies);
					Body.BodyInstance->TermBody();
					delete Body.BodyInstance;
					Body.BodyInstance = nullptr;
				}
			}
			It.Value.RemoveAllSwap([](const FBodyWithTransform& Body)
			{
				return !Body.BodyInstance;
			});
		}
	}

	{
		VOXEL_SCOPE_COUNTER("RemoveAt");
		Voxel_UnbuiltMatrices.RemoveAt(StartIndex, Num);
		Voxel_CustomData.RemoveAt(StartIndex * Voxel_MeshKey.NumCustomDataChannels, Num * Voxel_MeshKey.NumCustomDataChannels);
	}
	DEC_DWORD_STAT_BY(STAT_VoxelHISMComponent_NumInstances, Num);

	if (Voxel_UnbuiltMatrices.Num() == 0)
	{
		ensure(Voxel_UnbuiltInstancesToClear.Num() == 0);
		ensure(Voxel_CustomData.Num() == 0);
		// Having 0 instances creates a whole bunch of issues, so add one
		Voxel_UnbuiltMatrices.Add(FVoxelFoliageMatrix(EmptyMatrix));
		Voxel_CustomData.Init(0, Voxel_MeshKey.NumCustomDataChannels);
	}

	// Generate a dummy ID to order deletions with tasks
	Voxel_Mappings.DeletionsStack.Add({ FVoxelHISMBuildTaskId::New(), StartIndex, Num });

#if VOXEL_DEBUG && 0
	{
		VOXEL_SCOPE_COUNTER("Debug");
		for (int32 UnbuiltIndex = 0; UnbuiltIndex < Voxel_UnbuiltMatrices.Num(); UnbuiltIndex++)
		{
			const int32 BuiltIndex = Voxel_Mappings.GetBuiltIndex(UnbuiltIndex);
			if (BuiltIndex == -1) continue;
			ensure(Voxel_UnbuiltMatrices[UnbuiltIndex] == Voxel_BuiltMatrices[BuiltIndex]);
		}
		for (int32 BuildIndex = 0; BuildIndex < Voxel_BuiltMatrices.Num(); BuildIndex++)
		{
			const int32 UnbuiltIndex = Voxel_Mappings.GetUnbuiltIndex(BuildIndex);
			if (UnbuiltIndex == -1) continue;
			ensure(Voxel_UnbuiltMatrices[UnbuiltIndex] == Voxel_BuiltMatrices[BuildIndex]);
		}
	}
#endif

	Voxel_ScheduleBuildTree();
	Voxel_UpdateAllocatedMemory();

	ensure(Voxel_CustomData.Num() == Voxel_MeshKey.NumCustomDataChannels * Voxel_UnbuiltMatrices.Num());
}

bool UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_IsEmpty() const
{
	return Voxel_UnbuiltMatrices.Num() == 0 || (Voxel_UnbuiltMatrices.Num() == 1 && Voxel_UnbuiltMatrices[0] == FVoxelFoliageMatrix(EmptyMatrix));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_StartBuildTree()
{
	VOXEL_FUNCTION_COUNTER();

	ensure(Voxel_TaskUniqueId.IsValid() == Voxel_TaskCancelCounterPtr.IsValid());
	
	if (!ensure(GetStaticMesh()) ||
		!ensure(Voxel_UnbuiltMatrices.Num() > 0) ||
		!ensure(Voxel_CustomData.Num() == Voxel_MeshKey.NumCustomDataChannels * Voxel_UnbuiltMatrices.Num()))
	{
		return;
	}

	const TVoxelSharedPtr<FVoxelPool> Pool = Voxel_Pool.Pin();
	if (!ensure(Pool.IsValid())) return;

	if (Voxel_TaskCancelCounterPtr.IsValid())
	{
		Voxel_TaskCancelCounterPtr->Increment();
		Voxel_TaskCancelCounterPtr.Reset();
	}
	
	auto* Task = new FVoxelHISMBuildTask(this, Voxel_UnbuiltMatrices, Voxel_CustomData);
	Voxel_TaskUniqueId = Task->UniqueId;
	Voxel_TaskCancelCounterPtr = Task->CancelCounter;
	Pool->QueueTask(Task);

	ensure(!Voxel_TaskIdToNewInstancesBounds.Contains(Voxel_TaskUniqueId));
	// Not true when removing instances ensure(Voxel_PendingNewInstancesBounds.Num() > 0);
	Voxel_TaskIdToNewInstancesBounds.Add(Voxel_TaskUniqueId, MoveTemp(Voxel_PendingNewInstancesBounds));

	Voxel_UpdateAllocatedMemory();
}

void UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_FinishBuilding(FVoxelHISMBuiltData& BuiltData)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(Voxel_TaskUniqueId.IsValid() == Voxel_TaskCancelCounterPtr.IsValid());

	if (!Voxel_TaskUniqueId.IsValid())
	{
		// Task is too late, newer one already completed
		ensure(!Voxel_TaskIdToNewInstancesBounds.Contains(BuiltData.UniqueId));
		ensure(Voxel_UnbuiltInstancesToClear.Num() == 0);
		return;
	}

	ensure(Voxel_TaskUniqueId >= BuiltData.UniqueId); // Else the task time traveled
	const bool bIsLatestTask = BuiltData.UniqueId == Voxel_TaskUniqueId;

	for (int32 UnbuiltIndex : Voxel_UnbuiltInstancesToClear)
	{
		if (!BuiltData.InstancesToBuiltInstances.IsValidIndex(UnbuiltIndex))
		{
			ensure(!bIsLatestTask);
			continue;
		}
		const int32 BuiltIndex = BuiltData.InstancesToBuiltInstances[UnbuiltIndex];
		BuiltData.InstanceBuffer->SetInstance(BuiltIndex, EmptyMatrix, 0);
		BuiltData.BuiltInstancesMatrices[BuiltIndex] = FVoxelFoliageMatrix(EmptyMatrix);
	}
	
	if (bIsLatestTask)
	{
		Voxel_TaskUniqueId = {};
		Voxel_TaskCancelCounterPtr.Reset();
		Voxel_UnbuiltInstancesToClear.Reset();
	}
	else
	{
		// Do not reset the task id nor cancel it as this is not the right one yet
	}

	const int32 NumInstances = BuiltData.InstanceBuffer->GetNumInstances();
	check(NumInstances > 0);
	check(NumInstances == BuiltData.BuiltInstancesMatrices.Num());
	check(NumInstances == BuiltData.InstancesToBuiltInstances.Num());
	check(NumInstances == BuiltData.BuiltInstancesToInstances.Num());
	// Not true with timer delays ensure(!bIsLatestTask || NumInstances == Voxel_UnbuiltMatrices.Num());

	Voxel_BuiltMatrices = MoveTemp(BuiltData.BuiltInstancesMatrices);

	Voxel_Mappings.InstancesToBuiltInstances = MoveTemp(BuiltData.InstancesToBuiltInstances);
	Voxel_Mappings.BuiltInstancesToInstances = MoveTemp(BuiltData.BuiltInstancesToInstances);

	// Remove any old deletions
	// < safe as we are generating a dummy id when deleting
	Voxel_Mappings.DeletionsStack.RemoveAll([&](auto& Deletion) { return Deletion.TaskUniqueId < BuiltData.UniqueId; });
	
	constexpr bool bRequireCPUAccess = true;
	if (!PerInstanceRenderData.IsValid() || PerInstanceRenderData->InstanceBuffer.RequireCPUAccess != bRequireCPUAccess)
	{
		if (ensure(PerInstanceRenderData.IsValid()))
		{
			VOXEL_SCOPE_COUNTER("ReleasePerInstanceRenderData");
			ReleasePerInstanceRenderData();
		}

		VOXEL_SCOPE_COUNTER("InitPerInstanceRenderData");
		InitPerInstanceRenderData(true, BuiltData.InstanceBuffer.Get(), bRequireCPUAccess);
	}
	else
	{
		VOXEL_SCOPE_COUNTER("UpdateFromPreallocatedData");
		PerInstanceRenderData->UpdateFromPreallocatedData(*BuiltData.InstanceBuffer);
	}

	{
		VOXEL_SCOPE_COUNTER("AcceptPrebuiltTree");
		AcceptPrebuiltTree(BuiltData.ClusterTree, BuiltData.OcclusionLayerNum, NumInstances);
	}

	for (auto It = Voxel_TaskIdToNewInstancesBounds.CreateIterator(); It; ++It)
	{
		if (It.Key() <= BuiltData.UniqueId)
		{
			for (auto& Chunk : It.Value())
			{
				Voxel_RefreshPhysics(Chunk);
			}
			It.RemoveCurrent();
		}
	}

	Voxel_UpdateAllocatedMemory();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_EnablePhysics(const FVoxelIntBox& Chunk)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(!Voxel_InstanceBodies.Contains(Chunk))) return;
	TArray<FBodyWithTransform>& Bodies = Voxel_InstanceBodies.Add(Chunk);

	Voxel_EnablePhysicsImpl(Chunk, Bodies);

	Voxel_UpdateAllocatedMemory();
}

void UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_DisablePhysics(const FVoxelIntBox& Chunk)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(Voxel_InstanceBodies.Contains(Chunk))) return;
	auto Bodies = Voxel_InstanceBodies.FindAndRemoveChecked(Chunk);
	
	Voxel_DisablePhysicsImpl(Bodies);

	Voxel_UpdateAllocatedMemory();
}

void UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_RefreshPhysics(const FVoxelIntBox& BoundsToUpdate)
{
	VOXEL_FUNCTION_COUNTER();

	for (auto& It : Voxel_InstanceBodies)
	{
		const auto& Chunk = It.Key;
		auto& Bodies = It.Value;
		if (Chunk.Intersect(BoundsToUpdate))
		{
			Voxel_DisablePhysicsImpl(Bodies);
			ensure(Bodies.Num() == 0);
			Voxel_EnablePhysicsImpl(Chunk, Bodies);
		}
	}

	Voxel_UpdateAllocatedMemory();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFoliageTransforms UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_RemoveInstancesInArea(
	const FVoxelIntBox& VoxelBounds,
	const FVoxelConstDataAccelerator* Accelerator,
	EVoxelSpawnerActorSpawnType SpawnType)
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelFoliageTransforms OutTransforms;
	OutTransforms.TransformsOffset = Voxel_VoxelPosition;

	check(SpawnType == EVoxelSpawnerActorSpawnType::All || Accelerator);
	
	TArray<int32> BuiltIndicesToClear;
	FVoxelIntBoxWithValidity BoundsToUpdate;

	const auto Lambda = [&](const int32 BuiltIndex)
	{
		if (Voxel_Mappings.GetUnbuiltIndex(BuiltIndex) == -1)
		{
			// Deleted
			return;
		}
		
		INC_DWORD_STAT_BY(STAT_VoxelHISMComponent_NumFloatingMeshChecked, 1);

		const FVoxelFoliageMatrix Matrix = Voxel_BuiltMatrices[BuiltIndex];
		const FTransform LocalInstanceTransform = FTransform(Matrix.GetCleanMatrix());
		
		if (LocalInstanceTransform.GetScale3D().IsNearlyZero())
		{
			return;
		}

		const FVoxelVector GlobalVoxelPosition = Voxel_GetGlobalVoxelPosition(Matrix);
		if (!VoxelBounds.Contains(FVoxelIntBox(GlobalVoxelPosition)))
		{
			return;
		}

		if (SpawnType == EVoxelSpawnerActorSpawnType::All || Accelerator->GetFloatValue(GlobalVoxelPosition.X, GlobalVoxelPosition.Y, GlobalVoxelPosition.Z, 0) > 0)
		{
			OutTransforms.Matrices.Add(Matrix);
			BuiltIndicesToClear.Add(BuiltIndex);
			BoundsToUpdate += FVoxelIntBox(GlobalVoxelPosition);
		}
	};
	if (!ensure(ClusterTreePtr.IsValid())) return {};
	Voxel_IterateInstancesInBounds(*ClusterTreePtr, Voxel_VoxelBoundsToLocal(VoxelBounds), Lambda);

	if (BuiltIndicesToClear.Num() > 0)
	{
		Voxel_SetInstancesScaleToZero(BuiltIndicesToClear);
		Voxel_RemoveInstancesFromSections(BuiltIndicesToClear);
		Voxel_RefreshPhysics(BoundsToUpdate.GetBox());
	}
	
	return OutTransforms;
}

bool UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_RemoveInstanceByIndex(int32 InstanceIndex, FVoxelFoliageTransform& OutTransform)
{
	VOXEL_FUNCTION_COUNTER();
	
	const int32 BuiltIndex = Voxel_Mappings.GetBuiltIndex(InstanceIndex);
	if (BuiltIndex == -1)
	{
		return false;
	}

	const auto Matrix = Voxel_BuiltMatrices[BuiltIndex];

	OutTransform.TransformOffset = Voxel_VoxelPosition;
	OutTransform.Matrix = Matrix;

	Voxel_SetInstancesScaleToZero({ BuiltIndex });
	Voxel_RemoveInstancesFromSections({ BuiltIndex });

	const FVoxelIntBox MatrixBounds = FVoxelIntBox(Voxel_GetGlobalVoxelPosition(Matrix));
	
	for (auto& It : Voxel_InstanceBodies)
	{
		const FVoxelIntBox& Chunk = It.Key;
		TArray<FBodyWithTransform>& Bodies = It.Value;
		if (Chunk.Intersect(MatrixBounds))
		{
			for (int32 BodyIndex = 0; BodyIndex < Bodies.Num(); BodyIndex++)
			{
				FBodyInstance* Body = Bodies[BodyIndex].BodyInstance;
				if (Body->InstanceBodyIndex == InstanceIndex)
				{
					DEC_DWORD_STAT(STAT_VoxelHISMComponent_NumPhysicsBodies);
					Body->TermBody();
					delete Body;
					Bodies.RemoveAtSwap(BodyIndex);
					return true;
				}
			}
		}
	}

#if VOXEL_DEBUG
	for (auto& It : Voxel_InstanceBodies)
	{
		for (FBodyWithTransform& Body : It.Value)
		{
			ensure(Body.BodyInstance->InstanceBodyIndex != InstanceIndex);
		}
	}
#endif
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelHierarchicalInstancedStaticMeshComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	VOXEL_FUNCTION_COUNTER();
	
	ensure(Voxel_TaskUniqueId.IsValid() == Voxel_TaskCancelCounterPtr.IsValid());
	
	Voxel_TaskUniqueId = {};
	if (Voxel_TaskCancelCounterPtr.IsValid())
	{
		Voxel_TaskCancelCounterPtr->Increment();
		Voxel_TaskCancelCounterPtr.Reset();
	}
	
	DEC_DWORD_STAT_BY(STAT_VoxelHISMComponent_NumInstances, Voxel_UnbuiltMatrices.Num());

	// Free up memory
	Voxel_UnbuiltMatrices.Empty();
	Voxel_BuiltMatrices.Empty();
	Voxel_Mappings.InstancesToBuiltInstances.Empty();
	Voxel_Mappings.BuiltInstancesToInstances.Empty();
	Voxel_Mappings.DeletionsStack.Empty();
	Voxel_UnbuiltInstancesToClear.Empty();
	Voxel_TaskIdToNewInstancesBounds.Empty();
	Voxel_PendingNewInstancesBounds.Empty();
	
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

bool UVoxelHierarchicalInstancedStaticMeshComponent::ShouldCreatePhysicsState() const
{
	return UInstancedStaticMeshComponent::ShouldCreatePhysicsState();
}

void UVoxelHierarchicalInstancedStaticMeshComponent::OnDestroyPhysicsState()
{
	VOXEL_FUNCTION_COUNTER();
	
	Super::OnDestroyPhysicsState();

	for (auto& It : Voxel_InstanceBodies)
	{
		Voxel_DisablePhysicsImpl(It.Value);
	}
	Voxel_InstanceBodies.Empty();
}

void UVoxelHierarchicalInstancedStaticMeshComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	VOXEL_FUNCTION_COUNTER();
	
	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);

	if (bPhysicsStateCreated && !(UpdateTransformFlags & EUpdateTransformFlags::SkipPhysicsUpdate))
	{
		for (auto& It : Voxel_InstanceBodies)
		{
			for (FBodyWithTransform& Body : It.Value)
			{
				const FTransform Transform = Body.LocalTransform * GetComponentTransform();
				Body.BodyInstance->SetBodyTransform(Transform, Teleport);
				Body.BodyInstance->UpdateBodyScale(Transform.GetScale3D());
			}
		}
	}
}

inline void DrawBVH(UVoxelLineBatchComponent& LineBatchComponent, const FTransform& LocalToWorld, float Lifetime, const FColor& Color, const TArray<FClusterNode>& ClusterTree, int32 Depth, int32 Index = 0)
{
	if (ClusterTree.Num() == 0) return;

	const auto& Node = ClusterTree[Index];
	if (Depth <= 0 || Node.FirstChild == -1)
	{
		const FBox Box(Node.BoundMin, Node.BoundMax);
		UVoxelDebugUtilities::DrawDebugBox(LineBatchComponent,  LocalToWorld, Box, Lifetime, 0, Color);
		return;
	}

	for (int32 ChildIndex = Node.FirstChild; ChildIndex <= Node.LastChild; ChildIndex++)
	{
		DrawBVH(LineBatchComponent, LocalToWorld, Lifetime, Color, ClusterTree, Depth - 1, ChildIndex);
	}
}

void UVoxelHierarchicalInstancedStaticMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	VOXEL_FUNCTION_COUNTER();
	
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CVarShowHISMCollisions.GetValueOnGameThread())
	{
		if (!GetStaticMesh()) return;
		
		const FBox MeshBox = GetStaticMesh()->GetBounds().GetBox();
		for (auto& It : Voxel_InstanceBodies)
		{
			for (const FBodyWithTransform& Body : It.Value)
			{
				const FBox BodyBox = MeshBox.TransformBy(Body.BodyInstance->GetUnrealWorldTransform().GetScaled(Body.BodyInstance->Scale3D));
				DrawDebugBox(
					GetWorld(),
					BodyBox.GetCenter(),
					BodyBox.GetExtent(),
					FColor::Red,
					false,
					DeltaTime * 2);
			}
		}
	}

	if (CVarBVHDebugDepth.GetValueOnGameThread() && ClusterTreePtr.IsValid())
	{
		const int32 Depth = CVarBVHDebugDepth.GetValueOnGameThread() - 1;
		AVoxelWorld* VoxelWorld = Cast<AVoxelWorld>(GetOwner());
		if (ensure(VoxelWorld))
		{
			DrawBVH(
				VoxelWorld->GetLineBatchComponent(),
				GetComponentTransform(),
				GetWorld()->GetDeltaSeconds() * 1.5f,
				FColor(FVoxelUtilities::MurmurHash64(uint64(this))),
				*ClusterTreePtr,
				Depth);
		}
	}

	if (CVarRandomColorPerHISM.GetValueOnGameThread() && !Voxel_DebugMaterial)
	{
		Voxel_DebugMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Voxel/MaterialHelpers/Debug_UsePerInstanceRandomAsColor"));
		for (int32 Index = 0; Index < GetNumMaterials(); Index++)
		{
			SetMaterial(Index, Voxel_DebugMaterial);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int32 UVoxelHierarchicalInstancedStaticMeshComponent::FVoxelMappings::GetBuiltIndex(int32 UnbuiltIndex) const
{
	// Reverse back the changes
	for (int32 StackIndex = DeletionsStack.Num() - 1; StackIndex >= 0; StackIndex--)
	{
		auto& Deletion = DeletionsStack[StackIndex];
		if (Deletion.StartIndex <= UnbuiltIndex) // If we were affected by the deletion
		{
			UnbuiltIndex += Deletion.Num; // Translate back
		}
	}
	if (InstancesToBuiltInstances.IsValidIndex(UnbuiltIndex))
	{
		return InstancesToBuiltInstances[UnbuiltIndex];
	}
	else
	{
		return -1;
	}
}

int32 UVoxelHierarchicalInstancedStaticMeshComponent::FVoxelMappings::GetUnbuiltIndex(const int32 BuiltIndex) const
{
	if (!BuiltInstancesToInstances.IsValidIndex(BuiltIndex))
	{
		return -1;
	}
	
	int32 UnbuiltIndex = BuiltInstancesToInstances[BuiltIndex];
	
	// The UnbuiltIndex is the one of the last tree build: need to apply deletions to it
	for (int32 StackIndex = 0; StackIndex < DeletionsStack.Num(); StackIndex++)
	{
		auto& Deletion = DeletionsStack[StackIndex];
		if (Deletion.StartIndex <= UnbuiltIndex) 
		{
			if (UnbuiltIndex < Deletion.StartIndex + Deletion.Num)
			{
				// Got deleted
				return -1;
			}

			// Apply the deletion
			UnbuiltIndex -= Deletion.Num;
		}
	}
	
	// ensureVoxelSlowNoSideEffects(GetBuiltIndex(UnbuiltIndex) == BuiltIndex);
	return UnbuiltIndex;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_UpdateAllocatedMemory()
{
	VOXEL_FUNCTION_COUNTER();
	
	ensure(Voxel_CustomData.Num() == Voxel_MeshKey.NumCustomDataChannels * Voxel_UnbuiltMatrices.Num());
	
	DEC_VOXEL_MEMORY_STAT_BY(STAT_VoxelHISMMemory, Voxel_AllocatedMemory);
	
	Voxel_AllocatedMemory = 0;
	Voxel_AllocatedMemory += Voxel_UnbuiltMatrices.GetAllocatedSize();
	Voxel_AllocatedMemory += Voxel_BuiltMatrices.GetAllocatedSize();
	Voxel_AllocatedMemory += Voxel_CustomData.GetAllocatedSize();
	Voxel_AllocatedMemory += Voxel_Mappings.InstancesToBuiltInstances.GetAllocatedSize();
	Voxel_AllocatedMemory += Voxel_Mappings.BuiltInstancesToInstances.GetAllocatedSize();
	Voxel_AllocatedMemory += Voxel_Mappings.DeletionsStack.GetAllocatedSize();
	Voxel_AllocatedMemory += Voxel_UnbuiltInstancesToClear.GetAllocatedSize();
	Voxel_AllocatedMemory += Voxel_TaskIdToNewInstancesBounds.GetAllocatedSize();
	Voxel_AllocatedMemory += Voxel_PendingNewInstancesBounds.GetAllocatedSize();
	Voxel_AllocatedMemory += Voxel_InstanceBodies.GetAllocatedSize();

	INC_VOXEL_MEMORY_STAT_BY(STAT_VoxelHISMMemory, Voxel_AllocatedMemory);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_ScheduleBuildTree()
{
	VOXEL_FUNCTION_COUNTER();
	
	if (Voxel_BuildDelay <= 0)
	{
		Voxel_StartBuildTree();
	}
	else
	{
		auto& TimerManager = GetWorld()->GetTimerManager();
		TimerManager.SetTimer(
			Voxel_TimerHandle,
			this,
			&UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_StartBuildTree,
			Voxel_BuildDelay,
			false);
	}
}

void UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_RemoveInstancesFromSections(const TArray<int32>& BuiltIndices)
{
	VOXEL_FUNCTION_COUNTER();

#if VOXEL_DEBUG && 0
	for (int32 Index = 0; Index < Voxel_Sections.Num() - 1; Index++)
	{
		ensure(Voxel_Sections[Index]->StartIndex < Voxel_Sections[Index + 1]->StartIndex);
		ensure(Voxel_Sections[Index]->StartIndex + Voxel_Sections[Index]->Num == Voxel_Sections[Index + 1]->StartIndex);
	}
	ensure(Voxel_UnbuiltMatrices.Num() == 0 || Voxel_Sections.Last()->StartIndex + Voxel_Sections.Last()->Num == Voxel_UnbuiltMatrices.Num());
#endif
	
	for (int32 BuiltIndex : BuiltIndices)
	{
		const int32 UnbuiltIndex = Voxel_Mappings.GetUnbuiltIndex(BuiltIndex);
		if (!ensure(UnbuiltIndex != -1)) continue;

		const int32 SectionIndex = Algo::UpperBoundBy(Voxel_Sections, UnbuiltIndex, [](auto& Section) { return Section->StartIndex; });
		
		auto& Section = *Voxel_Sections[SectionIndex - 1];
		
		if (!ensure(Section.StartIndex <= UnbuiltIndex && UnbuiltIndex < Section.StartIndex + Section.Num)) continue;

		Section.RemovedIndices.Add(UnbuiltIndex - Section.StartIndex);
	}
}

void UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_SetInstancesScaleToZero(const TArray<int32>& BuiltIndices)
{
	VOXEL_FUNCTION_COUNTER();

	const auto InstanceBuffer = PerInstanceRenderData.IsValid() ? PerInstanceRenderData->InstanceBuffer_GameThread : nullptr;
	ensure(InstanceBuffer.IsValid() || Voxel_TaskUniqueId.IsValid());
	for (int32 BuiltIndex : BuiltIndices)
	{
		const int32 UnbuiltIndex = Voxel_Mappings.GetUnbuiltIndex(BuiltIndex);
		check(UnbuiltIndex >= 0);
		
		if (InstanceBuffer.IsValid())
		{
			InstanceBuffer->SetInstance(BuiltIndex, EmptyMatrix, 0);
		}
		ensure(Voxel_BuiltMatrices[BuiltIndex] == Voxel_UnbuiltMatrices[UnbuiltIndex]);
		Voxel_BuiltMatrices[BuiltIndex] = FVoxelFoliageMatrix(EmptyMatrix);
		Voxel_UnbuiltMatrices[UnbuiltIndex] = FVoxelFoliageMatrix(EmptyMatrix);

		if (Voxel_TaskUniqueId.IsValid())
		{
			Voxel_UnbuiltInstancesToClear.Add(UnbuiltIndex);
		}
	}

	if (InstanceBuffer.IsValid())
	{
		ENQUEUE_RENDER_COMMAND(UVoxelHierarchicalInstancedStaticMeshComponent_UpdateBuffer)(
			[PerInstanceRenderData = PerInstanceRenderData](FRHICommandListImmediate& RHICmdList)
			{
				PerInstanceRenderData->InstanceBuffer.InstanceData = PerInstanceRenderData->InstanceBuffer_GameThread;
				PerInstanceRenderData->InstanceBuffer.UpdateRHI();
			}
		);
		MarkRenderStateDirty();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_EnablePhysicsImpl(const FVoxelIntBox& Chunk, TArray<FBodyWithTransform>& OutBodies) const
{
	VOXEL_FUNCTION_COUNTER();
	ensure(bPhysicsStateCreated);
	
	// We want this function to be const
	auto* const Component = const_cast<UPrimitiveComponent*>(static_cast<const UPrimitiveComponent*>(this));
	
	UBodySetup* const BodySetup = Component->GetBodySetup();
	if (!ensure(BodySetup)) return;

	const FVoxelIntBox LocalBounds = Voxel_VoxelBoundsToLocal(Chunk);
	
	TArray<FBodyInstance*> BodiesToInit;
	TArray<FTransform> Transforms;
	const auto Lambda = [&](const int32 BuiltIndex)
	{
		const FTransform LocalInstanceTransform = FTransform(Voxel_BuiltMatrices[BuiltIndex].GetCleanMatrix());
		const FTransform GlobalInstanceTransform = LocalInstanceTransform * GetComponentTransform();

		if (GlobalInstanceTransform.GetScale3D().IsNearlyZero() ||
			!LocalBounds.ContainsFloat(LocalInstanceTransform.GetTranslation()))
		{
			return;
		}

		FBodyInstance* Instance = new FBodyInstance();
		INC_DWORD_STAT(STAT_VoxelHISMComponent_NumPhysicsBodies);

		Instance->CopyBodyInstancePropertiesFrom(&BodyInstance);
		Instance->bAutoWeld = false;

		// Make sure we never enable bSimulatePhysics for ISMComps
		Instance->bSimulatePhysics = false;

		// Set the body index to the UNBUILT index
		const int32 UnbuiltIndex = Voxel_Mappings.GetUnbuiltIndex(BuiltIndex);
		if (UnbuiltIndex == -1)
		{
			// Deleted
			return;
		}
		Instance->InstanceBodyIndex = UnbuiltIndex;
		ensure(Voxel_UnbuiltMatrices[Instance->InstanceBodyIndex] == Voxel_BuiltMatrices[BuiltIndex]);

		OutBodies.Add({ Instance, LocalInstanceTransform });
		
		BodiesToInit.Add(Instance);
		Transforms.Add(GlobalInstanceTransform);
	};
	
	if (!ensure(ClusterTreePtr.IsValid())) return;
	Voxel_IterateInstancesInBounds(*ClusterTreePtr, LocalBounds, Lambda);

	if (BodiesToInit.Num() == 0) return;

	VOXEL_SCOPE_COUNTER("InitStaticBodies");
	FBodyInstance::InitStaticBodies(BodiesToInit, Transforms, BodySetup, Component, GetWorld()->GetPhysicsScene());
}

void UVoxelHierarchicalInstancedStaticMeshComponent::Voxel_DisablePhysicsImpl(TArray<FBodyWithTransform>& Bodies) const
{
	VOXEL_FUNCTION_COUNTER();
	
	for (const FBodyWithTransform& Body : Bodies)
	{
		DEC_DWORD_STAT(STAT_VoxelHISMComponent_NumPhysicsBodies);
		Body.BodyInstance->TermBody();
		delete Body.BodyInstance;
	}
	Bodies.Reset();
}