// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "VoxelStatHelpers.h"
#include "VoxelFoliage.h"
#include "VoxelFoliageMatrix.h"
#include "VoxelInstancedMeshSettings.h"

class FVoxelCancelTracker;
class FVoxelFoliageSubsystem;
class FVoxelFoliageRayHandler;
class FVoxelConstDataAccelerator;
class FVoxelFoliageRandomGenerator;

struct FVoxelFoliageHit;
struct FVoxelInstancedMeshInstancesRef;

DECLARE_UNIQUE_VOXEL_ID(FVoxelFoliageProxyId);
DECLARE_UNIQUE_VOXEL_ID(FVoxelFoliageGroupId);
DECLARE_UNIQUE_VOXEL_ID(FVoxelFoliageBiomeId);

struct FVoxelFoliageStats
{
	FVoxelStatEntry TotalShared;
	FVoxelStatEntry TotalSelf;
	
	FVoxelStatEntry GenerateMesh_Ray;
	FVoxelStatEntry SetupEmbree_Ray;
	
	FVoxelStatEntry FindHits;
	
	FVoxelStatEntry FindBiome;
	FVoxelStatEntry SplitGroup;
	FVoxelStatEntry ApplyDensity;
	FVoxelStatEntry CheckFloating_Height;
	FVoxelStatEntry ComputeGradient_Height;
	FVoxelStatEntry GetWorldUp;
	FVoxelStatEntry CanSpawn;
	FVoxelStatEntry CreateTransforms;
	FVoxelStatEntry ComputeCustomData;

	FVoxelStatEntry GetValue;
	FVoxelStatEntry GetMaterial;
	FVoxelStatEntry TraceRay_Ray;
	FVoxelStatEntry GetHeight_Height;
	FVoxelStatEntry GetHeightRange_Height;
	
	int64 NumRays = 0;
	int64 NumHits = 0;
	int64 NumHitsAfterBiomes = 0;
	int64 NumHitsAfterDensity = 0;
	int64 NumHitsAfterFloating_Height = 0;
	int64 NumHitsAfterCanSpawn = 0;
	
	int64 NumConsecutiveRays_Ray = 0;
	int64 NumHitsFromConsecutiveRays_Ray = 0;

	FVoxelFoliageStats& operator+=(const FVoxelFoliageStats& Other)
	{
		static_assert(sizeof(FVoxelFoliageStats) == 19 * sizeof(FVoxelStatEntry) + 8 * sizeof(int64), "Need update");

#define ADD(Name) Name += Other.Name

		ADD(TotalShared);
		ADD(TotalSelf);
		
		ADD(GenerateMesh_Ray);
		ADD(SetupEmbree_Ray);

		ADD(FindHits);

		ADD(FindBiome);
		ADD(SplitGroup);
		ADD(ApplyDensity);
		ADD(CheckFloating_Height);
		ADD(ComputeGradient_Height);
		ADD(GetWorldUp);
		ADD(CanSpawn);
		ADD(CreateTransforms);
		ADD(ComputeCustomData);

		ADD(GetValue);
		ADD(GetMaterial);
		ADD(TraceRay_Ray);
		ADD(GetHeight_Height);
		ADD(GetHeightRange_Height);

		ADD(NumRays);
		ADD(NumHits);
		ADD(NumHitsAfterBiomes);
		ADD(NumHitsAfterDensity);
		ADD(NumHitsAfterFloating_Height);
		ADD(NumHitsAfterCanSpawn);

		ADD(NumConsecutiveRays_Ray);
		ADD(NumHitsFromConsecutiveRays_Ray);

#undef ADD

		return *this;
	}
};

class VOXELFOLIAGE_API FVoxelFoliageProxy : public TVoxelSharedFromThis<FVoxelFoliageProxy>
{
public:
	const FVoxelFoliageProxyId UniqueId = FVoxelFoliageProxyId::New();
	const TWeakObjectPtr<const UVoxelFoliage> Foliage;
	const FString FoliageName;

public:
	struct FBiome
	{
		// Should always be valid - even for foliage with no valid biome
		// This simplifies the task merging logic
		FVoxelFoliageBiomeId Id;

		// If true, will not check the output name/value and will assume this is the only biome foliage
		bool bDummyBiome = false;
		
		FName OutputName;
		// Will spawn this foliage if OutputValue = GetCustomOutput(OutputName)
		int32 OutputValue = -1;
	};
	const FBiome Biome;

	// Foliage in a same group shares hits
	struct FGroup
	{
		// Should always be valid - even for single foliage groups
		FVoxelFoliageGroupId Id;
		float Strength = 0.f;
	};
	const FGroup Group;
	
public:
	const FGuid Guid;
	const uint32 Seed;
	const FVoxelInstancedMeshWeakKey MeshKey;
	
	const FVoxelFoliageSpawnSettings SpawnSettings;
	const TArray<FVoxelFoliageDensity> Densities;
	const TArray<FVoxelFoliageCustomData> CustomDatas;
	
	const bool bEnableSlopeRestriction;
	const FFloatInterval GroundSlopeAngle;
	const bool bEnableHeightRestriction;
	const FFloatInterval HeightRestriction;
	const float HeightRestrictionFalloff;
	const FVoxelFoliageScale Scaling;
	const EVoxelFoliageRotation RotationAlignment;
	const bool bRandomYaw;
	const float RandomPitchAngle;
	const FVector LocalPositionOffset;
	const FRotator LocalRotationOffset;
	const FVector GlobalPositionOffset;
	const FVector FloatingDetectionOffset;

	const bool bSave;
	const bool bDoNotDespawn;

public:
	struct FInit
	{
		FString Name;
		FVoxelInstancedMeshKey MeshKey;

		TOptional<int32> SeedOverride;
		TOptional<FVoxelFoliageSpawnSettings> SpawnSettingsOverride;
		
		FBiome Biome;
		FGroup Group;
	};
	FVoxelFoliageProxy(
		const UVoxelFoliage& Foliage,
		const FVoxelFoliageSubsystem& FoliageSubsystem,
		const FInit& Init);

public:
	FVoxelFoliageStats GetTotalStats() const
	{
		FScopeLock Lock(&TotalStatsSection);
		return TotalStats;
	}
	void AddToTotalStats(const FVoxelFoliageStats& Stats) const
	{
		FScopeLock Lock(&TotalStatsSection);
		TotalStats += Stats;
	}
	
private:
	mutable FCriticalSection TotalStatsSection;
	mutable FVoxelFoliageStats TotalStats;

public:
	bool NeedCanSpawn() const;
	bool CanSpawn(
		const FVoxelFoliageRandomGenerator& RandomGenerator,
		const FVoxelVector& Position,
		const FVector& Normal,
		const FVector& WorldUp) const;

	FMatrix GetTransform(
		const FVoxelFoliageRandomGenerator& RandomGenerator,
		const FVector& Normal,
		const FVector& WorldUp,
		const FVector& Position) const;

	FVector GetScale(const FVoxelFoliageRandomGenerator& RandomGenerator) const;
};

struct FVoxelScopedFoliageStats : FVoxelFoliageStats
{
	const FVoxelFoliageProxy& Foliage;

	explicit FVoxelScopedFoliageStats(const FVoxelFoliageProxy& Foliage)
		: Foliage(Foliage)
	{
	}
	~FVoxelScopedFoliageStats()
	{
		Foliage.AddToTotalStats(*this);
	}
};

DECLARE_VOXEL_MEMORY_STAT(TEXT("Voxel Foliage Results Memory"), STAT_VoxelFoliageResults, STATGROUP_VoxelMemory, VOXELFOLIAGE_API);

struct FVoxelFoliageResultData
{
	FVoxelFoliageTransforms Transforms;
	TArray<float> CustomData;
};

class VOXELFOLIAGE_API FVoxelFoliageProxyResult
{
public:
	const FVoxelFoliageProxy& Proxy;
	const FVoxelIntBox Bounds;
	const TVoxelSharedRef<FVoxelFoliageResultData> Data;
	
private:
	const TVoxelWeakPtr<const FVoxelFoliageProxy> WeakProxy;
	uint32 AllocatedSize = 0;
	bool bCreated = false;
	bool bDirty = false;

	TUniquePtr<FVoxelInstancedMeshInstancesRef> InstancesRef;

public:
	FVoxelFoliageProxyResult(
		const FVoxelFoliageProxy& Proxy,
		const FVoxelIntBox& Bounds,
		const TVoxelSharedRef<FVoxelFoliageResultData>& Data);
	virtual ~FVoxelFoliageProxyResult();

	FVoxelFoliageProxyResult(const FVoxelFoliageProxyResult&) = delete;
	FVoxelFoliageProxyResult& operator=(const FVoxelFoliageProxyResult&) = delete;

public:
	void Create(const FVoxelFoliageSubsystem& Manager);
	void Destroy(const FVoxelFoliageSubsystem& Manager);
	void ApplyRemovedIndices(const FVoxelFoliageSubsystem& Manager);
	void UpdateStats();

	bool IsCreated() const
	{
		return bCreated;
	}
	
	bool IsDirty() const
	{
		return bDirty;
	}
	void MarkDirty()
	{
		bDirty = true;
	}
	
	bool NeedsToBeSaved() const
	{
		return IsDirty() && Proxy.bSave;
	}

private:
	uint32 GetAllocatedSize() const;
};