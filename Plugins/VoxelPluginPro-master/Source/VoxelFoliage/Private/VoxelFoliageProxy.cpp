// Copyright 2021 Phyronnaz

#include "VoxelFoliageProxy.h"
#include "VoxelFoliageSubsystem.h"
#include "VoxelFoliageDebug.h"
#include "VoxelFoliageRandomGenerator.h"
#include "VoxelInstancedMeshManager.h"
#include "HitGenerators/VoxelHeightHitGenerator.h"
#include "VoxelUtilities/VoxelMathUtilities.h"
#include "VoxelUtilities/VoxelGeneratorUtilities.h"
#include "VoxelData/VoxelDataIncludes.h"
#include "VoxelGenerators/VoxelGeneratorCache.h"
#include "VoxelMessages.h"

#include "Misc/ScopeExit.h"
#include "Engine/StaticMesh.h"

DEFINE_UNIQUE_VOXEL_ID(FVoxelFoliageProxyId);
DEFINE_UNIQUE_VOXEL_ID(FVoxelFoliageGroupId);
DEFINE_UNIQUE_VOXEL_ID(FVoxelFoliageBiomeId);

inline TArray<FVoxelFoliageDensity> GetDensities(const UVoxelFoliage& Foliage, const FVoxelFoliageSubsystem& FoliageSubsystem)
{
	TArray<FVoxelFoliageDensity> Result;
	for (const FVoxelFoliageDensity& Density : Foliage.Densities)
	{
		FVoxelFoliageDensity& NewDensity = Result.Add_GetRef(Density);
		
		if (Density.Type == EVoxelFoliageDensityType::GeneratorOutput)
		{
			if (!Density.bUseMainGenerator)
			{
				NewDensity.GeneratorInstance = FoliageSubsystem.GetSubsystemChecked<FVoxelGeneratorCache>().MakeGeneratorInstance(Density.CustomGenerator);
			}

			FVoxelGeneratorInstance& Generator = Density.bUseMainGenerator ? *FoliageSubsystem.GetSubsystemChecked<FVoxelData>().Generator : *NewDensity.GeneratorInstance;
			if (!Generator.GetOutputsPtrMap<v_flt>().Contains(Density.GeneratorOutputName))
			{
				FVoxelMessages::Warning(FVoxelUtilities::GetMissingGeneratorOutputErrorString<v_flt>(Density.GeneratorOutputName, Generator), Foliage.GetClass());
			}
		}
	}
	return Result;
}

inline TArray<FVoxelFoliageCustomData> GetCustomDatas(const UVoxelFoliage& Foliage, const FVoxelFoliageSubsystem& FoliageSubsystem)
{
	TArray<FVoxelFoliageCustomData> Result;
	for (const FVoxelFoliageCustomData& CustomData : Foliage.CustomDatas)
	{
		FVoxelFoliageCustomData& NewCustomData = Result.Add_GetRef(CustomData);
		
		if (CustomData.Type == EVoxelFoliageCustomDataType::ColorGeneratorOutput ||
			CustomData.Type == EVoxelFoliageCustomDataType::FloatGeneratorOutput)
		{
			if (!CustomData.bUseMainGenerator)
			{
				NewCustomData.GeneratorInstance = FoliageSubsystem.GetSubsystemChecked<FVoxelGeneratorCache>().MakeGeneratorInstance(CustomData.CustomGenerator);
			}

			FVoxelGeneratorInstance& Generator = CustomData.bUseMainGenerator ? *FoliageSubsystem.GetSubsystemChecked<FVoxelData>().Generator : *NewCustomData.GeneratorInstance;

			if (CustomData.Type == EVoxelFoliageCustomDataType::ColorGeneratorOutput)
			{
				if (!Generator.GetOutputsPtrMap<FColor>().Contains(CustomData.ColorGeneratorOutputName))
				{
					FVoxelMessages::Warning(FVoxelUtilities::GetMissingGeneratorOutputErrorString<FColor>(CustomData.ColorGeneratorOutputName, Generator), Foliage.GetClass());
				}
			}
			else
			{
				check(CustomData.Type == EVoxelFoliageCustomDataType::FloatGeneratorOutput);
				
				if (!Generator.GetOutputsPtrMap<v_flt>().Contains(CustomData.FloatGeneratorOutputName))
				{
					FVoxelMessages::Warning(FVoxelUtilities::GetMissingGeneratorOutputErrorString<v_flt>(CustomData.FloatGeneratorOutputName, Generator), Foliage.GetClass());
				}
			}
		}
	}
	return Result;
}

FVoxelFoliageProxy::FVoxelFoliageProxy(
	const UVoxelFoliage& Foliage,
	const FVoxelFoliageSubsystem& FoliageSubsystem,
	const FInit& Init)
	: Foliage(&Foliage)
	, FoliageName(Init.Name)

	, Biome(Init.Biome)
	, Group(Init.Group)

	, Guid(Foliage.Guid)
	, Seed(Init.SeedOverride.Get(FVoxelUtilities::MurmurHash32(Foliage.Guid.A ^ Foliage.Guid.B ^ Foliage.Guid.C ^ Foliage.Guid.D)))
	, MeshKey(Init.MeshKey)

	, SpawnSettings(Init.SpawnSettingsOverride.Get(Foliage.SpawnSettings))
	, Densities(GetDensities(Foliage, FoliageSubsystem))
	, CustomDatas(GetCustomDatas(Foliage, FoliageSubsystem))

	, bEnableSlopeRestriction(Foliage.bEnableSlopeRestriction)
	, GroundSlopeAngle(Foliage.GroundSlopeAngle)
	, bEnableHeightRestriction(Foliage.bEnableHeightRestriction)
	, HeightRestriction(Foliage.HeightRestriction)
	, HeightRestrictionFalloff(Foliage.HeightRestrictionFalloff)
	, Scaling(Foliage.Scaling)
	, RotationAlignment(Foliage.RotationAlignment)
	, bRandomYaw(Foliage.bRandomYaw)
	, RandomPitchAngle(Foliage.RandomPitchAngle)
	, LocalPositionOffset(Foliage.LocalPositionOffset)
	, LocalRotationOffset(Foliage.LocalRotationOffset)
	, GlobalPositionOffset(Foliage.GlobalPositionOffset)
	, FloatingDetectionOffset(Foliage.FloatingDetectionOffset)

	, bSave(Foliage.bSave)
	, bDoNotDespawn(Foliage.bDoNotDespawn)
{
	ensure(Init.Biome.Id.IsValid());
	ensure(Init.Group.Id.IsValid());
	
	const FVoxelData& Data = FoliageSubsystem.GetSubsystemChecked<FVoxelData>();
	
	if (SpawnSettings.SpawnType == EVoxelFoliageSpawnType::Height && !Data.Generator->GetOutputsPtrMap<v_flt>().Contains(SpawnSettings.HeightGraphOutputName_HeightOnly))
	{
		FVoxelMessages::Warning(FVoxelUtilities::GetMissingGeneratorOutputErrorString<v_flt>(SpawnSettings.HeightGraphOutputName_HeightOnly, *Data.Generator), Foliage.GetClass());
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelFoliageProxy::NeedCanSpawn() const
{
	return bEnableHeightRestriction || bEnableSlopeRestriction;
}

bool FVoxelFoliageProxy::CanSpawn(
	const FVoxelFoliageRandomGenerator& RandomGenerator,
	const FVoxelVector& Position,
	const FVector& Normal,
	const FVector& WorldUp) const
{
	ensure(NeedCanSpawn());
	
	if (bEnableHeightRestriction)
	{
		if (!HeightRestriction.Contains(Position.Z))
		{
			return false;
		}

		const float Center = (HeightRestriction.Min + HeightRestriction.Max) / 2.f;
		const float Radius = HeightRestriction.Size() / 2.f;
		const float Distance = FMath::Abs(Center - Position.Z);
		const float Falloff = FMath::Min(HeightRestrictionFalloff, Radius);

		const float Alpha = FVoxelUtilities::SmoothFalloff(Distance, Radius - Falloff, Falloff);

		if (RandomGenerator.GetHeightRestrictionFraction() >= Alpha)
		{
			return false;
		}
	}

	if (bEnableSlopeRestriction)
	{
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Normal, WorldUp)));
		if (!GroundSlopeAngle.Contains(Angle))
		{
			return false;
		}
	}

	return true;
}

FMatrix FVoxelFoliageProxy::GetTransform(
	const FVoxelFoliageRandomGenerator& RandomGenerator,
	const FVector& Normal,
	const FVector& WorldUp,
	const FVector& Position) const
{
	FMatrix Matrix = FRotationTranslationMatrix(LocalRotationOffset, LocalPositionOffset);
	
	const FVector Scale = GetScale(RandomGenerator);

	const float Yaw = bRandomYaw ? RandomGenerator.GetYawFraction() * 360.0f : 0.0f;
	const float Pitch = RandomGenerator.GetPitchFraction() * RandomPitchAngle;
	Matrix *= FScaleRotationTranslationMatrix(Scale, FRotator(Pitch, Yaw, 0.0f), FVector::ZeroVector);

	switch (RotationAlignment)
	{
	default: check(false);
	case EVoxelFoliageRotation::AlignToSurface:
		Matrix *= FRotationMatrix::MakeFromZ(Normal);
		break;
	case EVoxelFoliageRotation::AlignToWorldUp:
		Matrix *= FRotationMatrix::MakeFromZ(WorldUp);
		break;
	case EVoxelFoliageRotation::RandomAlign:
		Matrix *= FRotationMatrix::MakeFromZ(RandomGenerator.GetRandomAlignVector());
		break;
	}

	Matrix *= FTranslationMatrix(Position + GlobalPositionOffset);

	return Matrix;
}

FVector FVoxelFoliageProxy::GetScale(const FVoxelFoliageRandomGenerator& RandomGenerator) const
{
	FVector Scale;
	switch (Scaling.Scaling)
	{
	case EVoxelFoliageScaling::Uniform:
		Scale.X = Scaling.ScaleX.Interpolate(RandomGenerator.GetScaleFraction());
		Scale.Y = Scale.X;
		Scale.Z = Scale.X;
		break;
	case EVoxelFoliageScaling::Free:
		Scale.X = Scaling.ScaleX.Interpolate(RandomGenerator.GetScaleFraction());
		Scale.Y = Scaling.ScaleY.Interpolate(RandomGenerator.GetScaleFraction());
		Scale.Z = Scaling.ScaleZ.Interpolate(RandomGenerator.GetScaleFraction());
		break;
	case EVoxelFoliageScaling::LockXY:
		Scale.X = Scaling.ScaleX.Interpolate(RandomGenerator.GetScaleFraction());
		Scale.Y = Scale.X;
		Scale.Z = Scaling.ScaleZ.Interpolate(RandomGenerator.GetScaleFraction());
		break;
	default:
	check(false);
	}
	return Scale;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelFoliageResults);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Voxel Foliage Num Results"), STAT_VoxelFoliage_NumResults, STATGROUP_VoxelCounters);

FVoxelFoliageProxyResult::FVoxelFoliageProxyResult(
	const FVoxelFoliageProxy& Proxy,
	const FVoxelIntBox& Bounds,
	const TVoxelSharedRef<FVoxelFoliageResultData>& Data)
	: Proxy(Proxy)
	, Bounds(Bounds)
	, Data(Data)
	, WeakProxy(Proxy.AsShared())
{
	INC_DWORD_STAT(STAT_VoxelFoliage_NumResults);
	UpdateStats();
}

FVoxelFoliageProxyResult::~FVoxelFoliageProxyResult()
{
	DEC_DWORD_STAT(STAT_VoxelFoliage_NumResults);
	DEC_VOXEL_MEMORY_STAT_BY(STAT_VoxelFoliageResults, AllocatedSize);

	check(WeakProxy.Pin().Get() == &Proxy);
}

void FVoxelFoliageProxyResult::Create(const FVoxelFoliageSubsystem& Manager)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	ensure(!bCreated);
	bCreated = true;

	auto& MeshManager = Manager.GetSubsystemChecked<FVoxelInstancedMeshManager>();
	if (Data->Transforms.Matrices.Num() > 0)
	{
		// Matrices can be empty if all instances were removed
		ensure(!InstancesRef.IsValid());
		const auto Ref = MeshManager.AddInstances(Proxy.MeshKey, Data->Transforms, Data->CustomData, Bounds);
		if (Ref.IsValid())
		{
			// Ref might be invalid if we reached instances limit
			InstancesRef = MakeUnique<FVoxelInstancedMeshInstancesRef>(Ref);
		}
	}
	
	UpdateStats();
}

void FVoxelFoliageProxyResult::Destroy(const FVoxelFoliageSubsystem& Manager)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	ensure(bCreated);
	bCreated = false;
	
	ApplyRemovedIndices(Manager);
		
	auto& MeshManager = Manager.GetSubsystemChecked<FVoxelInstancedMeshManager>();
	if (InstancesRef.IsValid())
	{
		MeshManager.RemoveInstances(*InstancesRef);
		InstancesRef.Reset();
	}
	
	UpdateStats();
}

void FVoxelFoliageProxyResult::ApplyRemovedIndices(const FVoxelFoliageSubsystem& Manager)
{
	if (!InstancesRef.IsValid())
	{
		return;
	}
	
	auto& MeshManager = Manager.GetSubsystemChecked<FVoxelInstancedMeshManager>();

	TArray<int32> RemovedIndices = MeshManager.GetRemovedIndices(*InstancesRef);
	RemovedIndices.Sort([](int32 A, int32 B) { return A > B; }); // Need to sort in decreasing order for the RemoveAtSwap
	
	for (int32 RemovedIndex : RemovedIndices)
	{
		Data->Transforms.Matrices.RemoveAtSwap(RemovedIndex, 1, false);
		Data->CustomData.RemoveAtSwap(RemovedIndex, Proxy.CustomDatas.Num(), false);
	}
	Data->Transforms.Matrices.Shrink();
	Data->CustomData.Shrink();

	UpdateStats();
}

void FVoxelFoliageProxyResult::UpdateStats()
{
	DEC_VOXEL_MEMORY_STAT_BY(STAT_VoxelFoliageResults, AllocatedSize);
	AllocatedSize = GetAllocatedSize();
	INC_VOXEL_MEMORY_STAT_BY(STAT_VoxelFoliageResults, AllocatedSize);
}

uint32 FVoxelFoliageProxyResult::GetAllocatedSize() const
{
	return sizeof(*this) + Data->Transforms.Matrices.GetAllocatedSize() + Data->CustomData.GetAllocatedSize();
}